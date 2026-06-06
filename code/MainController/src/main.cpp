#include <Arduino.h>
#include <Wire.h>

#define I2C_SCL_MAIN 9
#define I2C_SDA_MAIN 8
#define G_SENSITIVITY = 2

#define STEPS_PER_REVOLUTION 200

TwoWire mainI2C = TwoWire(0);

//kalman filter stuff
double Q_angle = 0.001;  // Process noise variance for the accelerometer
double Q_bias = 0.003;   // Process noise variance for the gyroscope bias
double R_measure = 0.03; // Measurement noise variance
unsigned long time = 0;

const int8_t X_IMU = 1;
const int8_t Y_IMU = 2;
const int8_t Z_IMU = 3;

#define LIMB_TYPE 1
#define BASE_TYPE 2
#define HAND_TYPE 3

struct IMUOrientation { 
    /* assuming positive X is facing left, positive Y is facing front, and Z is facing down
        negative values indicate the opposite direction. The orientation of the BMI323
        is assumed to be facing the white dot from above at the top left, with the left 
        of the dot being left (+X) and the forward from the dot being front (+Y). */
    int8_t xDirection, yDirection, zDirection; 
};
class Motor {
    public:
        uint8_t address;
        uint8_t angleAddress;
        IMUOrientation angleCarrierOrientation; /* rearranges sensor values to be relative to
                                                the arm as a whole (front is mostly arbitrary, yaw is not measured)
                                                uses the angles tracked from angleAddress*/
        uint8_t type = LIMB_TYPE;
        double pitch, roll = 0; // radians
        double bias, rate = 0;
        double P[2][2] = {{0,0}, {0, 0}};
        Motor(
            uint8_t address,
            uint8_t angleAddress, // address to query for current angle of actuator
            IMUOrientation angleCarrierOrientation,
            uint8_t type = LIMB_TYPE
        ){
            this->address = address;
            this->angleCarrierOrientation = angleCarrierOrientation;
            this->type = type;
        }
        // returns a bool of whether message was successfully sent, function does not work if the class represents the angle tracker for the hand
        bool moveAngle(
            double angle, // radians to rotate, negative will indicate counterclockwise
            int16_t stepOffset = 0,
            double gearRatio = 1.0/49 // 1:gearRatio -- a gearRatio above one means outputting less torque and more speed, vice cersa (for this project always equals 1/49)
        ){
            if(this->type == HAND_TYPE){ // preventing the angle sensor for the hand to be tracked
                return false;
            };
            int16_t numSteps = STEPS_PER_REVOLUTION*angle/(2*PI*gearRatio);
            //sends a int16_t num of steps to take, positive & negative included
            mainI2C.beginTransmission(this->address);
            mainI2C.write(numSteps + stepOffset);
            mainI2C.endTransmission();
            return true;
        }
};

// The base (the one marked BASE_TYPE) tracks its own angle, and Limb1's angle is tracked by Motor2's angle and Limb2 by Motor3.
// The hand is tracked by a motorless board designated HAND_TYPE
// However, Motor1 controls Limb1's angle, Motor2 controls Limb2's angle and Motor3 controls the Hand's angle. The base motor 
// only alters the yaw of the arm.
const uint8_t numLimbs = 5;
Motor motorList[numLimbs] = { // in order of connection from base to hand
    Motor(0x50, 0x50, {-Y_IMU, -X_IMU, -Z_IMU}, BASE_TYPE), //Base Motor
    Motor(0x51, 0x50, {-Y_IMU, -X_IMU, -Z_IMU}), //Motor 1
    Motor(0x52, 0x52, {-Z_IMU, X_IMU, -Y_IMU}), //Motor 2
    Motor(0x53, 0x53, {-Z_IMU, X_IMU, -Y_IMU}), //Motor 3
    Motor(0x54, 0x54, {-X_IMU, Z_IMU, Y_IMU}, HAND_TYPE) //Hand
};

//------

//all angles are radians from the horizontal up; 0 or pi radians means arm is flat 

void sendCommand(
    double baseAngle,
    double limb1Angle,
    double limb2Angle,
    double handAngle
){
    for(int i = 0; i < 4; i++){
        double selectedAngle = 
            i == 0 ? baseAngle :
            i == 1 ? limb1Angle :
            i == 2 ? limb2Angle :
            i == 3 ? handAngle :
            PI/2;
        if(selectedAngle < 0 || selectedAngle > PI) continue;
        double offset = i == 2 ? -limb1Angle :
                        i == 3 ? limb2Angle : 0;
        motorList[i].moveAngle(selectedAngle - motorList[i].pitch + offset);
    }
}
void rearrangeIMUOutput(
    double *directionVector, // intended to be in order of X, Y and Z values
    uint8_t numRead,
    int16_t value, 
    IMUOrientation imuOrient 
    /* IMUOrientation tells us which of the sensor's directions correspond to the arm's direction, 
        eg. if xDirection = -Z_IMU that means that perceived from the perspective of the arm,
        the X (left) acceleration/gyroscope sensor value would be the negative of the acceleration 
        value read from the Z axis of the IMU
    */
    /* the numRead corresponds to whether the X, Y or Z sensor was read
    */
){
    if(abs(imuOrient.xDirection) == numRead){
        directionVector[0] = value * (imuOrient.xDirection < 0 ? -1 : 1); 
    } else if(abs(imuOrient.yDirection) == numRead) {
        directionVector[1] = value * (imuOrient.yDirection < 0 ? -1 : 1); 
    } else if(abs(imuOrient.zDirection) == numRead) {
        directionVector[2] = value * (imuOrient.zDirection < 0 ? -1 : 1); 
    }
}
void fetchAngles(){
    for(int i = 0; i < numLimbs; i++){
        double accel[3]; //xyz format
        double gyro[3]; //xyz format
        Motor motor = motorList[i];
        mainI2C.requestFrom(motor.angleAddress, 16*6); // order sent: accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z
        uint8_t integersRead = 0;                         // since 6 signed 16 bit integers are sent, we expect to read 12 bytes
        while(mainI2C.available() >= 2 && integersRead < 6 ){
            integersRead++;
            byte msb = mainI2C.read();
            byte lsb = mainI2C.read();
            int16_t num = (msb << 8) | lsb;
            rearrangeIMUOutput(
                integersRead <= 3 ? accel : gyro,
                integersRead,
                num,
                motor.angleCarrierOrientation
            );
        }
        double dt = (millis() - time)/1000.0;
        dt = dt <= 0 ? 0.001 : dt;
        time = millis();

        // original function:
        // pitch = Kalman_filter(pitch, gx, atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI);
        // roll = Kalman_filter(roll, gy, atan2(ay, az) * 180.0 / PI);
        // in the blog X is front and Y is left, therefore here pitch and roll are switched
 
        motor.roll = Kalman_filter(&motor, motor.roll, gyro[0], atan2(-accel[0], sqrt(accel[1] * accel[1] + accel[2] * accel[2])) * 180.0 / PI, dt);
        motor.pitch = Kalman_filter(&motor, motor.pitch, gyro[1], atan2(accel[1], accel[2]) * 180.0 / PI, dt);
    }
}

void setup() {
    mainI2C.begin(I2C_SDA_MAIN, I2C_SCL_MAIN, 100000);
    time = millis();
}

bool hasRunTest = false;
void loop() {
    // all funcs below
    // ...
    if(!hasRunTest && millis() > 8000){
        sendCommand(
            PI/8,
            PI/2,
            0,
            0
        );
        hasRunTest = true;
    }
    fetchAngles();
    delay(10);
}

// borrowed code from https://how2electronics.com/measure-pitch-roll-yaw-with-mpu6050-hmc5883l-esp32/
double Kalman_filter(Motor *limb, double angle, double gyroRate, double accelAngle, double dt) {
  limb->rate = gyroRate - limb->bias;
  angle += dt * limb->rate;
 
  limb->P[0][0] += dt * (dt * limb->P[1][1] - limb->P[0][1] - limb->P[1][0] + Q_angle);
  limb->P[0][1] -= dt * limb->P[1][1];
  limb->P[1][0] -= dt * limb->P[1][1];
  limb->P[1][1] += Q_bias * dt;
 
  // Update
  double S = limb->P[0][0] + R_measure; // Estimate error
  double K[2];                    // Kalman gain
  K[0] = limb->P[0][0] / S;
  K[1] = limb->P[1][0] / S;
 
  double y = accelAngle - angle; // Angle difference
  angle += K[0] * y;
  limb->bias += K[1] * y;
 
  double P00_temp = limb->P[0][0];
  double P01_temp = limb->P[0][1];
 
  limb->P[0][0] -= K[0] * P00_temp;
  limb->P[0][1] -= K[0] * P01_temp;
  limb->P[1][0] -= K[1] * P00_temp;
  limb->P[1][1] -= K[1] * P01_temp;
 
  return angle;
}
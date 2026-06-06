#include <Arduino.h>
#include <Wire.h>

/* Alter the two parameters below based on which device is being flashed.
* Only set "isHand" to true if flashing to the board meant for tracking the hand's angle (the one not attached to any motor)
* Note that an A4988 motor stepper driver is needed only for non-hand controllers
*/
/* UNIQUE address for each limb, change 
    in each limb and main controller accordingly 
    Addresses for each limb:
    * BaseMotor : 0x50
    * Motor1 : 0x51
    * Motor2 : 0x52
    * Motor3 : 0x53
    * Hand : 0x54
*/
/* --------------------------- */
const uint8_t LIMB_I2C_ADDRESS = 0x50; 
const bool isHand = false;
/* --------------------------- */

#define STEP_PIN 39
#define DIR_PIN 40
#define I2C_SCL_SUB 42
#define I2C_SDA_SUB 43
#define I2C_SCL_MAIN 21
#define I2C_SDA_MAIN 22
#define MS1_PIN 26
#define MS2_PIN 27
#define MS3_PIN 28

#define INC_ADDRESS 0x68
#define ACC_CONF  0x20  //Page 91
#define GYR_CONF  0x21  //Page 93
#define CMD       0x7E  //Page 65

#define ACC_DATA_X_ADDR 0x03 
#define ACC_DATA_Y_ADDR 0x04
#define ACC_DATA_Z_ADDR 0x05

const int COMMAND_BUFFER_MAX = 20;
const double gearRatio = 1/49;
const int stepsPerRevolution = 200;

TwoWire mainI2C(I2C_SDA_MAIN, I2C_SCL_MAIN);
TwoWire subI2C(I2C_SDA_SUB, I2C_SCL_SUB);
I2C_HandleTypeDef hi2c1; //I2C main     
I2C_HandleTypeDef hi2c2; //I2C sub

// angle data
int16_t commandBuffer[COMMAND_BUFFER_MAX];
int16_t  x, y, z;
int16_t  gyro_x, gyro_y, gyro_z;
bool isTurning = false;

void handleMainCommand(int numBytes){
    shiftCommands();
    uint8_t buffer[numBytes];
    mainI2C.readBytes(buffer, numBytes);
    int16_t steps = (buffer[0] << 8) | buffer[1];
    for(int i = 0; i < COMMAND_BUFFER_MAX; i++){
        if(commandBuffer[i] == 0){
            commandBuffer[i] = steps;
            break;
        }
    }
}
void handleData(){
    mainI2C.write(x);
    mainI2C.write(y);
    mainI2C.write(z);
    mainI2C.write(gyro_x);
    mainI2C.write(gyro_y);
    mainI2C.write(gyro_z);
}

// below is borrowed code from https://forum.arduino.cc/t/bmi323-sensor-with-i2c-kalman-filter/1225900 for the BMI323
//Read all axis
void readAllAccel() {
  Wire.beginTransmission(INC_ADDRESS);
  Wire.write(0x03);
  Wire.endTransmission();
  Wire.requestFrom(INC_ADDRESS, 20);
  uint16_t data[20];
  int i = 0;
  while(Wire.available()){
    data[i] = Wire.read();
    i++;
  }
  int offset = 0;  
  x =             (data[offset + 0]   | (uint16_t)data[offset + 1] << 8);  //0x03
  y =             (data[offset + 2]   | (uint16_t)data[offset + 3] << 8);  //0x04
  z =             (data[offset + 4]   | (uint16_t)data[offset + 5] << 8);  //0x05
  gyro_x =         (data[offset + 6]   | (uint16_t)data[offset + 7] << 8);  //0x06
  gyro_y =         (data[offset + 8]   | (uint16_t)data[offset + 9] << 8);  //0x07
  gyro_z =         (data[offset + 10]  | (uint16_t)data[offset + 11] << 8); //0x08
//   temperature =   (data[offset + 12]  | (uint16_t)data[offset + 13] << 8); //0x09
//   temperatureInDegree = (temperature/512.f) + 23.0f;  
}
void softReset(){  
  writeRegister16(&subI2C, CMD, 0xDEAF);
  delay(50);    
}

void writeRegister16(TwoWire *wire, uint16_t reg, uint16_t value) {
  wire->beginTransmission(INC_ADDRESS);
  wire->write(reg);
  //Low
  wire->write((uint16_t)value & 0xff);
  //High
  wire->write((uint16_t)value >> 8);
  wire->endTransmission();
}

uint16_t readRegister16(TwoWire *wire, uint8_t reg) {
  wire->beginTransmission(INC_ADDRESS);
  wire->write(reg);
  wire->endTransmission(false);
  int n = wire->requestFrom(INC_ADDRESS, 4); 
  uint16_t data[4];
  int i =0;
  while(wire->available()){
    data[i] = Wire.read();
    i++;
  } 
  return (data[3] | data[2] << 8);
}
// ------------------------------------

void setup(){
    mainI2C.begin(LIMB_I2C_ADDRESS);
    mainI2C.setClock(100000);
    
    if(!isHand){
        subI2C.begin();
        subI2C.setClock(100000);
        pinMode(STEP_PIN, OUTPUT);
        pinMode(DIR_PIN, OUTPUT);
    }
    
    writeRegister16(&subI2C, 0x20, 0x4007); //accelerometer init, the N in 0x__N_ determines sensitivity
    writeRegister16(&subI2C, 0x21, 0x404B); //gyroscope init

    mainI2C.onReceive(handleMainCommand);
    mainI2C.onRequest(handleData);
}
void shiftCommands(){
    int offset = 0;
    for(int i = 0; i < COMMAND_BUFFER_MAX; i++){
        if(commandBuffer[i] == 0){
            offset++;
        } else {
            break;
        }
    }
    if(offset >= COMMAND_BUFFER_MAX) return;
    for(int i = 0; i < COMMAND_BUFFER_MAX-offset; i++){
        commandBuffer[i] = commandBuffer[i+offset];
    }
}

const int maxRPM = 200;
const double maxStepVelocity = ((double)maxRPM*stepsPerRevolution)/(60.0*1000); // units: steps per millisecond
const double stepAcceleration = 0.0001; // step per millisecond^2, 
const double timeToMaxVelocity = maxStepVelocity/stepAcceleration; //milliseconds
double stepVelocity = 0; // step per millisecond, is always positive
bool isDirectionHigh = false; // direction
bool isDeaccelerating = false;
uint32_t time = millis(); 

int stepDeaccelerationCutoff() {
    return (int)(stepVelocity*stepVelocity)/(2*stepAcceleration);
}
void loop(){
    if(readRegister16(&subI2C, 0x02) == 0x00){
        readAllAccel();
    }
    if(!isHand){
        if(commandBuffer[0] != 0){
            if(commandBuffer[0] < 0 == !isDirectionHigh){ // check if direction is set correctly, if not set the correct one (negative steps are CCW)
                isDirectionHigh == !isDirectionHigh;
                digitalWrite(DIR_PIN, isDirectionHigh ? HIGH : LOW);
            }
            if(!isDeaccelerating && commandBuffer[0] < stepDeaccelerationCutoff()) { //deacceleration as limb approaches position
                isDeaccelerating = true;
            } else if(stepVelocity < maxStepVelocity){ //check if velocity is max, if not speed up w/ constant acceleration
                stepVelocity += stepAcceleration*(millis()-time);
                if(stepVelocity > maxStepVelocity) stepVelocity = maxStepVelocity;
            }
            if(isDeaccelerating) stepVelocity -= stepAcceleration;
        } else {
            stepVelocity = 0;
            isDeaccelerating = false;
            if(commandBuffer[1] != 0){
                shiftCommands();
            }
        }
        if(stepVelocity > 0 && millis() - time >= 1.0/stepVelocity && commandBuffer[0] > 0){ //stepping the motor if velocity and command exists
            digitalWrite(STEP_PIN, HIGH);
            commandBuffer[0]--;
        } else {
            digitalWrite(STEP_PIN, LOW);
        }
        if(stepVelocity < 0) stepVelocity = 0;
    }
    time = millis();
    delayMicroseconds(100);
}
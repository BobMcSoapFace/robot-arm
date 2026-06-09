# robot-arm
![zine picture](/images/zine.png)
robot-arm is a 4-axis robot arm designed with modularity in mind. Depending on the maximum torque output of the motors and maximum range of the I2C wires, this project can support numerous limbs with only minor changes in code. Currently, the code is fixed to allowing two limbs and a hand on the robot.

I built this project mostly to challenge myself to tackle more complicated electronic and mechanical designs, but also because most hobbyist robot arms focus on using claws or grippers to pick up objects. For this project, the minor change of a claw to a hand that can sense its own orientation as well as high gear ratios allows for supporting heavier objects such as displays and light fixtures. 

All materials can be found within BOM.csv.
# Electronics
## Limb Controller PCB
![limb controller pcb 3d model](/images/limbcontroller_model.png)
![limb controller pcb footprint](/images/limbcontroller_pcb.png)
![limb controller schematic](/images/limbcontroller_schematic.png)
## Main Controller PCB
![main controller pcb 3d model](/images/maincontroller_model.png)
![main controller pcb footprint](/images/maincontroller_pcb.png)
![main controller schematic](/images/maincontroller_schematic.png)
# Setup Instructions
## Electronics
First solder the components onto each board based on the diagrams above. For this project only 1 of the main controller boards are required, however 5 limb controller boards are required as well (with 1 not requiring a corresponding stepper motor nor driver).

On the main controller board, use PlatformIO to flash the main controller board using the code in /code/MainController. Since the board runs on an ESP32, flashing using other tools is likely be supported. Do not connect the XT60 while simultaneously flashing.

On the limb controller boards, use PlatformIO to flash via USB-C. While USB-C is connected, hold down the BOOT0 button and press the NRST button to enter DFU mode. Adjust the two parameters listed in the program based on which gearBox the controller is intended to be used for. Then, flash the firmware in /code/LimbController. 

## Assembly
Unless specified otherwise, all screws mentioned in the steps below refer to M4 1.6cm long screws meant for M4x6x6 inserts.
### Step 1 - baseSunGear
![baseSunGear diagram](/images/baseSunGear_diagram.png)
| Object | Print Quantity |
| --- | --- |
| 1 | 1x1 |
| 2 | 1x1 |
\* The "xN" represents the total number of gearboxes that require this part (meaning that a 3x4 quantity will need to be printed 12 times total)
1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert. 
   * In this part, only 1 threaded insert is required (the attachment between part 1 and part 2)
2. Screw part 1 to part 2.
3. Repeat this process 3 more times, for a total baseSunGear quantity of 4.
### Step 2 - gearReducer
![gearReducer diagram](/images/gearReducer_diagram.png)
<ins>**Note: There is a baseSunGear supposed to be between parts 1 and 2 and the three gears, but is hidden for better clarity.**</ins>
| Object | Print Quantity |
| --- | --- |
| 1 | 1x4 |
| 2 | 1x4 |
| 3 | 1x4 |
| 4 | 1x4 |
| Gear (27 teeth) | 3x4 |
\* The "xN" represents the total number of gearboxes that require this part (meaning that a 3x4 quantity will need to be printed 12 times total)

1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert. 
2. Using a hammer or other means, press fit a 6907ZZ bearing into part 1.
3. For each of the three 27 tooth gears, press fit a 6805 bearing into its cavity.
4. With the smaller hole side facing upwards, place each of the 3 gears on the poles of part 1.
5. Press fit a 6907ZZ bearing into the center hole of part 2.
6. With the baseSunGear's circular side of its part 2, press fit the baseSunGear into the 6907ZZ bearing of part 1 (the baseSunGear protruding upwards and aligning its teeth with the three gears).
7. Press fit part 2's 6907ZZ bearing on the baseSunGear's part 1.
7. Screw part 2 to part 1.
8. Screw part 3 to part 2.
9. Screw part 4 to part 3.
10. Repeat this process 3 more times, for a total gearReducer quantity of 4.
### Step 3 - gearReducerOutput
![gearReducerOutput digram](/images/gearReducerOutput_diagram.png)
| Object | Print Quantity |
| --- | --- |
| 1 | 1x4 |
| 2 | 1x4 |
| 3 | 1x4 |
| Gear (27 teeth) | 3x4 |
\* The "xN" represents the total number of gearboxes that require this part (meaning that a 3x4 quantity will need to be printed 12 times total)
1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert. 
2. Using a hammer or other means, press fit a 6907ZZ bearing into part 1.
3. For each of the three 27 tooth gears, press fit a 6805 bearing into its cavity.
4. With the smaller hole side facing upwards, place each of the 3 gears on the poles of part 1.
5. Press fit a 6907ZZ bearing into the center hole of part 2.
6. Screw part 2 to part 1.
7. Screw part 3 to part 2.
9. Repeat this process 3 more times, for a total gearReducerOutput quantity of 4.
### Step 4 - gearBox
![gearBox diagram 1](/images/gearBox_diagram1.png)
![gearBox diagram 2](/images/gearBox_diagram2.png)
<ins>**Note: There is a 6020 bearing supposed to be between parts 1 and 2, but is hidden for better clarity.**</ins>
| Object | Print Quantity |
| --- | --- |
| 1 | 1x4 |
| 2 | 1x4 |
| 3 | 1x4 |
| 4 | 1x4 |
| 5 | 1x4 |
| 6 | 2x4 |
| gearReducerOutput | 1x4 |
| gearReducer | 1x4 |
| baseSunGear | 1x4 |
| Nema17 Stepper Motor | 1x4 |

1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert. 
2. Screw the two part 6's together using the outer brackets.
3. Screw the joined part 6's to part 7.
4. Screw part 4 to part 7.
5. Screw part 5 to part 7, with the screw terminals being accessible from the provided holes.
6. Align the asymmetrical shaft of the Nema17 stepper motor to the gearReducer's baseSunGear corresponding hole of the same shape, attaching the two objects.
7. Lower the gearReducer with the Nema17 into the ring gears, with the Nema17 aligning with the cavity provided by part 4.
8. Once the Nema17 is inside the cavity, screw part 3 to part 4, blocking the Nema17 from falling through.
9. Unscrew part 4 from part 3 for the gearReducer, and unscrew parts 2 and 3 from gearReducerOutput's part 1.
10. Lower the gearReducerOutput into the ring gear, aligning the 27 teeth gears with the base gear in the middle.
11. Screw back part 4 to part 3 for the gearReducer
12. Press fit the gearReducer's part 4 with the 6907ZZ bearing of the deattached gearReducerOutput's part 2, then screw parts 2 and 3 back to gearReducerOutput's part 1.
13. Press fit a 6020 bearing onto the gearbox's part 1, then press fit the bearing onto the gearReducerOutput's output cylinder (marked part 2)
14. Repeat 3 more times, for a total of 4 gearboxes.
### Step 5 - limb
![limb diagram 1](/images/limb_diagram1.png)
![limb diagram 2](/images/limb_diagram2.png)
| Object | Print Quantity |
| --- | --- |
| 1 | 1x2 |
| 3 | 2x2 |
| 4 | 1x2 |
| 5 | 1x2 |
| 7 | 1x2 |
| 8 | 1x2 |
| 9 | 1x2 |
| 10 | 1x2 |
| 11 | 1x2 |
| 12 | 1x2 |
| 13 | 1x2 |
| 14 | 1x2 |
\*Note that parts 15 and 6 are the same parts as 14 and 5 respectively.
1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert. 
2. On a gearbox, screw part 8 to the side with the controller.
3. Press fit a 6020 bearing to part 7, then screw part 7 to part 8.
4. Press fit part 1 to the bearing.
5. On the side of the gearbox outputting rotation, screw part 11 to the gearbox's output.
6. Screw part 12 to part 11.
7. Screw part 13 to parts 12 and 2.
8. Screw the two part 3's to part 13.
9. Screw part 5 to the two part 3's.
10. Screw part 4 to the part 3 on the side of the gearbox's controller.
11. Screw part 14 to the part 3 on the side of the gearbox's output.
12. Screw parts 14 and 4 to a second gearbox.
13. Repeat the steps above for a second limb, using the newly attached gearbox as the base (used in steps 2 and 5).
### Step 6 - base
![base diagram 1](/images/base_diagram1.png)
![base diagram 2](/images/base_diagram2.png)
![base diagram 3](/images/base_diagram3.png)
| Object | Print Quantity |
| --- | --- |
| 1 | 1x1 |
| 2 | 7x1 |
| 4 | 1x1 |
| 5 | 7x1 |
| 6 | 8x1 |
| 7 | 1x1 |
| 8 | 1x1 |
| 9 | 1x1 |

\*Note that parts 15 and 6 are the same parts as 14 and 5 respectively.
1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert.
2. Using the inserts indicated by the region labeled #3, screw in 7 part 2's, avoiding the area covered by the gearbox controller.
3. To increase arm stability during movement, insert a heavy filler (e.g sand or gravel) or other similar heavy objects in equal distribution among the part 2's.
4. Screw a part 5 to each of the part 2's (7 total).
5. Screw a main controller PCB to part 1
6. Screw part 1 to the gearbox, and then screw part 7 to part 1.
7. Screw 8 part 6's to connect each of the part 7's together. 
7. With a gearbox in the middle and output side facing upwards, screw part 4 to the gearbox using the three holes.
### Step 7 - hand
![hand diagram 1](/images/hand_diagram1.png)
![hand diagram 2](/images/hand_diagram2.png)
![hand diagram 3](/images/hand_diagram3.png)

| Object | Print Quantity |
| --- | --- |
| 1 | 1x1 |
| 2 | 1x1 |
| 3 | 1x1 |

1. For every hole about 0.7cm in diameter in every printable part, insert a M4x6x6 heat set threaded insert.
2. Screw part 3 to the output of the last limb's gearbox output.
3. Press fit a 6020 bearing to the last limb's gearbox controller side on the attached part, then press fit part 1 into the bearing.
4. Screw parts 1 and 3 together with part 2.
5. Screw the limb controller (part 4) to part 2.
   * Note that on this controller, an A4988 motor driver is not required.
### Step 8 - Connecting Limbs to Base
1. Using the screw terminals on each board, connect wires from the main controller to the limb controller in the base on one side. On the other side of the limb controller's screw terminals, connect more wires to the next gearbox's limb controllers, repeating the process for each controller until the hand's controller is connected.
   * You may have to unscrew some parts connecting to the gearbox to be able to reach the screw terminals.
   * Low capacitance wires are preferred, ex. solid core
2. Lower the side of the first limb's initial gearbox with the output side facing the base's part 4 side with two holes for screws.
3. Deattach the limb's part 11 from the gearbox and part 12, then secure the gearbox in the base's part 4 by screwing in the two holes. Reattach part 11 afterwards.
4. On the other side of the gearbox, secure the gearbox with the three screw holes on base 4.

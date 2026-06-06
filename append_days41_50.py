import os

content_day41 = """
## 🧠 Code Explanation

Let's break down how we read an IMU sensor directly via I2C Registers:

### 1. Waking up the Sensor
```cpp
Wire.beginTransmission(MPU_ADDRESS);
Wire.write(REG_PWR_MGMT_1);
Wire.write(0x00); 
Wire.endTransmission();
```
- By default, the MPU6050 boots up in "Sleep Mode" to save battery.
- We must manually write a `0x00` (All Zeros) to the Power Management 1 Register (`0x6B`) to flip the sleep bit OFF and turn the internal clock ON.

### 2. Multi-byte Burst Reads
```cpp
Wire.write(REG_ACCEL_XOUT_H); 
Wire.requestFrom(MPU_ADDRESS, 6);

*ax = (Wire.read() << 8) | Wire.read();
*ay = (Wire.read() << 8) | Wire.read();
*az = (Wire.read() << 8) | Wire.read();
```
- The Acceleration data is stored across 6 registers (High and Low bytes for X, Y, and Z).
- We point the sensor to the start register (`0x3B`), and then `requestFrom` 6 bytes in one giant gulp! This ensures that all 3 axes are from the exact same moment in time.
- **Bitwise Math:** We take the first 8 bits (High Byte), shift them 8 spaces to the left (`<< 8`), and merge them (`|`) with the next 8 bits (Low Byte) to magically reconstruct the 16-bit integer!
"""
with open("Day_41_MPU6050_IMU/README.md", "a", encoding="utf-8") as f:
    f.write(content_day41)

content_day42 = """
## 🧠 Code Explanation

Let's break down the magic of Sensor Fusion math:

### 1. Gyroscope Integration (Short-Term Accuracy)
```cpp
gyroIntegratedRoll += gx * dt;
```
- A gyroscope measures *speed* of rotation (Degrees per Second), not angle.
- To find the angle, we have to integrate (multiply speed by time). If we are spinning at 10 deg/sec for 0.02 seconds (`dt`), we have moved 0.2 degrees. We add this to our total angle.
- **The Problem:** Gyroscopes have microscopic static biases. Over time, that 0.02 degree error adds up into a massive, unstoppable drift.

### 2. The Complementary Filter (Sensor Fusion)
```cpp
fusedRoll = ALPHA * (fusedRoll + gx * dt) + (1.0 - ALPHA) * rollAccel;
```
- We calculate the absolute tilt angle using Accelerometer gravity vectors (`rollAccel`). Accelerometers never drift, but they are incredibly noisy when the robot moves or vibrates.
- **The Filter:** We take 98% (`ALPHA = 0.98`) of our smooth, fast Gyroscope angle, and mix in just 2% (`1.0 - ALPHA`) of our noisy, but absolutely-anchored Accelerometer gravity angle!
- This completely cancels out the gyro drift, while perfectly smoothing out the accelerometer vibrations!
"""
with open("Day_42_Complementary_Filter/README.md", "a", encoding="utf-8") as f:
    f.write(content_day42)

content_day43 = """
## 🧠 Code Explanation

Let's break down how we built a Non-Blocking Robotic State Machine:

### 1. The Switch-Case State Router
```cpp
enum DriveState { STATE_DRIVE_FORWARD, STATE_STOP_ROBOT, STATE_SCAN_RIGHT ... };

switch (currentDriveState) {
    case STATE_DRIVE_FORWARD:
        driveForward(180);
        if (frontDist < CRITICAL_DIST_CM) currentDriveState = STATE_STOP_ROBOT;
        break;
}
```
- Beginners often use `delay()` to run sequences. E.g., `Scan Right -> Delay(500) -> Ping -> Scan Left`. But `delay()` freezes the Arduino, meaning you can't read a stop button or update a screen while waiting!
- We use a State Machine. The code instantly checks the current "State", executes a tiny piece of logic, and loops back around thousands of times a second.
- When an obstacle is detected, we simply change the `currentDriveState` variable, and the next time the loop comes around, it instantly begins executing the next behavior!

### 2. Servo Settle Timing
```cpp
if (currentMillis - stateTimerStart >= 300) {
    sensorServo.write(SERVO_RIGHT);
    // ...
}
```
- Because we aren't using `delay()`, we use `millis()` math. We record the time we commanded the servo to move, and keep bypassing the `if` statement until 300ms have passed. This gives the physical servo motor time to physically rotate and stop vibrating before we trigger the acoustic sonar ping!
"""
with open("Day_43_Obstacle_Avoidance_Robot/README.md", "a", encoding="utf-8") as f:
    f.write(content_day43)

content_day44 = """
## 🧠 Code Explanation

Let's break down how to program analog hysteresis logic for line tracking:

### 1. Analog Hysteresis Gating
```cpp
int rawLeft = analogRead(LEFT_SENSOR_PIN);
bool leftOnLine = (rawLeft > LINE_THRESHOLD);
```
- Many cheap IR sensors have a little blue potentiometer you turn with a screwdriver to adjust sensitivity. Vibrations often knock these out of calibration.
- By using `analogRead()` (0-1023), we bring the raw voltage directly into the code.
- We set a software `LINE_THRESHOLD` (e.g. 500). Anything above is converted to a simple `True` (Black Line) or `False` (White Floor). This makes the robot infinitely more reliable!

### 2. Differential Drive Steer Logic
```cpp
else if (leftOnLine && !rightOnLine) {
    steerLeft(TURN_SPEED);
} 
```
- If the left sensor suddenly sees black, it means the robot has drifted too far to the right!
- `steerLeft()` corrects this by cutting power to the left wheel and running the right wheel forward. This creates a pivot point on the left wheel, swinging the front of the robot back over the center of the line.
"""
with open("Day_44_Line_Follower_2Sensor/README.md", "a", encoding="utf-8") as f:
    f.write(content_day44)

content_day45 = """
## 🧠 Code Explanation

Let's break down how to find the exact Center of Mass of a line:

### 1. Min-Max Auto-Normalization
```cpp
int normVal = map(rawVal, sensorMinValues[i], sensorMaxValues[i], 0, 1000);
```
- Every sensor is slightly different due to manufacturing, LED brightness, or distance from the floor. Sensor 1 might read 800 on black, while Sensor 3 reads 950.
- Our boot calibration recorded the absolute minimums and maximums for *each individual sensor*.
- We use `map()` to stretch those unique ranges into a perfect, uniform 0 to 1000 scale. Now all 5 sensors behave absolutely identically!

### 2. Weighted Centroid Math
```cpp
coordinateSum += ((float)normVal * SENSOR_COORDINATES[i]);
lineCentroid = coordinateSum / weightSum;
```
- We assign physical X-coordinates to our sensors (-2.0, -1.0, 0, 1.0, 2.0).
- We multiply each sensor's normalized "weight" (0-1000) by its coordinate, and divide the total by the sum of all weights.
- This outputs a continuous decimal float! If the line is slightly off-center, we get `+0.35`, rather than a hard digital "RIGHT" or "LEFT". We can feed this float directly into our motors for buttery-smooth steering!
"""
with open("Day_45_Line_Array_Calibrated/README.md", "a", encoding="utf-8") as f:
    f.write(content_day45)

content_day46 = """
## 🧠 Code Explanation

Let's break down the steering math of our high-speed PD Controller:

### 1. The Proportional Term (P)
```cpp
double pTerm = Kp * error;
```
- `error` is our Line Centroid (how far off center we are, from -2.0 to +2.0).
- If we multiply that error by a huge number (`Kp`), we get an aggressive steering correction. If the error is small, the correction is small. This smoothly proportionally steers the robot back to the center!

### 2. The Derivative Term (D)
```cpp
double dTerm = Kd * ((error - lastError) / dt);
lastError = error;
```
- If we only use `P`, the robot will steer back toward the line so fast that it overshoots the center, wobbling back and forth violently!
- `(error - lastError) / dt` calculates the *slope* of our movement. It answers the question: "How fast are we approaching the line?"
- If we are approaching the line extremely fast, the Derivative term becomes a massive negative number. It fights against the Proportional term, actively pulling the brakes before we cross the center, eliminating all wobbles!
"""
with open("Day_46_PD_Line_Follower/README.md", "a", encoding="utf-8") as f:
    f.write(content_day46)

content_day47 = """
## 🧠 Code Explanation

Let's break down how we keep an inverted pendulum from falling over:

### 1. PID Angle Correction
```cpp
double error = fusedPitch - targetPitch;
double pTerm = Kp * error;
errorSum += error * dt;
double iTerm = Ki * errorSum;
double dTerm = Kd * ((error - lastError) / dt);
double pidOutput = pTerm + iTerm + dTerm;
```
- Our Complementary Filter (Day 42) gives us a lightning-fast `fusedPitch` angle.
- We feed this into a PID controller. If the robot falls forward (positive error), the PID output becomes a large positive number.
- We map this output directly to Motor PWM, causing the wheels to drive forward underneath the falling chassis, catching the center of gravity!

### 2. The Motor Deadband
```cpp
motorPWM = map(motorPWM, 0, 255, MIN_MOTOR_PWM, 255);
```
- DC motors have static friction. If you give a motor a PWM of 20, it just whines and doesn't move. It might need a minimum PWM of 45 just to start spinning.
- If we don't account for this, the PID will output tiny corrections (e.g., PWM 15) when the robot is nearly perfectly balanced, but the motors won't move! The robot will fall over before the PID ramps up high enough to overcome friction.
- We use `map()` to instantly bump any output greater than 0 up past the friction threshold (`MIN_MOTOR_PWM`), making the robot incredibly responsive to micro-balance adjustments!
"""
with open("Day_47_Self_Balancing_Robot/README.md", "a", encoding="utf-8") as f:
    f.write(content_day47)

content_day48 = """
## 🧠 Code Explanation

Let's break down the logic of solving mazes automatically:

### 1. The Left-Hand Rule Priority
```cpp
if (pathLeft) {
    logDecision('L');
    drivePivot(TURN_LEFT);
} else if (pathAhead) {
    logDecision('S');
} else if (pathRight) {
    // ...
```
- The "Left-Hand-On-Wall" algorithm states that if you put your left hand on the wall of a maze and never take it off, you will eventually find the exit.
- In code, this translates to an `if / else-if` priority hierarchy: ALWAYS take a Left turn if it exists. Only go Straight if you can't go Left. Only go Right if you can't go Left or Straight!
- Every time we make a decision, we push a character (`'L'`, `'S'`, `'R'`, `'U'`) into our `pathLog` array memory.

### 2. Path Optimization
```cpp
if (prev == 'L' && next == 'L') replacement = 'S';
else if (prev == 'L' && next == 'S') replacement = 'R';
```
- Once we reach the end, our memory array is full of dead-end U-turns (e.g. `['L', 'L', 'U', 'S', 'R']`).
- If you turn Left, hit a dead-end, U-Turn, and then go Straight back past the intersection, that entire detour was completely equivalent to just turning Right in the first place! (`L U S` -> `R`).
- Our algorithm loops through the array, finding any `U`, looking at the turns before and after it, and replacing all three letters with the simplified shortcut!
"""
with open("Day_48_Maze_Solver/README.md", "a", encoding="utf-8") as f:
    f.write(content_day48)

content_day49 = """
## 🧠 Code Explanation

Let's break down how to decode raw satellite telemetry without libraries:

### 1. The Byte-Level State Machine
```cpp
if (c == '$') {
    isCapturing = true;
} else if (isCapturing) {
    if (c == '\\n') { processCompleteSentence(); }
    else { sentenceBuffer[bufferIndex++] = c; }
}
```
- The GPS module screams raw text at the Arduino continuously at 9600 baud.
- A standard NMEA sentence looks like: `$GPRMC,225446,A,4916.45,N,12311.12,W...*68`
- Our parser ignores all garbage characters until it sees the Start Marker (`$`).
- It then saves every character into a buffer array until it hits the End Marker (`\\n` Newline). It then instantly fires off the processing function!

### 2. Checksum Verification
```cpp
long calculatedChecksum = 0;
for (int i = 0; i < asteriskIndex; i++) {
    calculatedChecksum ^= sentenceBuffer[i];
}
```
- Radio waves get corrupted by atmospheric noise. If a coordinate digit gets flipped, our robot might think it's in the ocean!
- The GPS module calculates an XOR Hash of all the characters in the sentence and attaches it to the end (e.g., `*68`).
- Our code manually performs an XOR (`^=`) on all the characters we received. If our calculated hash doesn't perfectly match the hash attached to the sentence, we throw the corrupted data in the trash!
"""
with open("Day_49_GPS_Decoder/README.md", "a", encoding="utf-8") as f:
    f.write(content_day49)

content_day50 = """
## 🧠 Code Explanation

Let's break down how we use a digital compass to hold an absolute magnetic heading:

### 1. Hard-Iron Bias Calibration
```cpp
xOffset = (float)(xMax + xMin) / 2.0;
float calX = (float)rawX - xOffset;
```
- Motors, batteries, and the chassis itself contain iron that warps the Earth's magnetic field, severely distorting the magnetometer's readings (Hard-Iron Distortion).
- During boot, we spin the robot in a circle and record the max and min magnetic flux seen on the X and Y axes.
- The true center of the magnetic field should be zero. By calculating the midpoint between our max and min readings, we find exactly how much the robot's metal chassis has shifted the center (the Offset). We subtract this offset from all future readings to completely eliminate the distortion!

### 2. Shortest-Path Error Normalization
```cpp
double error = targetHeading - currentHeading;
if (error > 180.0) error -= 360.0;
else if (error < -180.0) error += 360.0;
```
- Compasses wrap around at 360 degrees.
- If our Target is 10° and our Current Heading is 350°, `10 - 350 = -340°` of error. If we feed `-340` into our PID controller, the robot will violently spin left in a massive circle almost all the way around!
- By injecting our normalizer logic, we force the error to take the shortest path across the 0/360 boundary. The `-340` error instantly transforms into `+20°`, and the robot simply makes a tiny, gentle turn to the right!
"""
with open("Day_50_Compass_Heading_Lock/README.md", "a", encoding="utf-8") as f:
    f.write(content_day50)

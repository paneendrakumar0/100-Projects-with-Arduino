import os

content_day91 = """
## 🧠 Code Explanation

Let's break down how we decode a physical dial into perfect digital signals:

### 1. The Physics of Quadrature Encoders
- Unlike a standard potentiometer that gives an absolute analog voltage, a Rotary Encoder gives relative digital steps. 
- Inside the dial are two physical metal contact switches, mechanically offset from one another. 
- As you turn the dial, they generate two square waves (Channel A and Channel B) that are exactly 90 degrees out of phase.

### 2. State-Machine Decoding
```cpp
if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
  if (digitalRead(ENCODER_PIN_B) != currentStateCLK) {
    volumeUp();
  } else {
    volumeDown();
  }
}
```
- By monitoring Channel A (CLK) for a falling edge, we know the dial just clicked one physical "detent".
- In that exact microsecond, we look at the state of Channel B (DT). If Channel A transitioned *before* Channel B, the dial is turning Clockwise. If Channel B transitioned *before* Channel A, it's turning Counter-Clockwise. It's a mathematically flawless way to detect infinite rotation!

### 3. Native USB HID Consumer Control
```cpp
Consumer.write(MEDIA_VOLUME_UP);
```
- The ATmega32U4 chip (Leonardo/Micro) features a hardware USB transceiver. 
- Instead of relying on buggy PC-side software to read Serial strings, the Arduino physically enumerates to the operating system as a native USB Multimedia Keyboard. 
- Sending the `MEDIA_VOLUME_UP` hex code triggers the operating system's built-in volume control directly, bypassing all custom drivers!
"""
with open("Day_91_HID_Volume_Knob/README.md", "a", encoding="utf-8") as f:
    f.write(content_day91)

content_day92 = """
## 🧠 Code Explanation

Let's break down the industrial technique used inside every mechanical keyboard:

### 1. Matrix Scanning (Saving I/O Pins)
- A 104-key mechanical keyboard doesn't use 104 pins on a microcontroller. 
- Switches are wired into a grid of Rows and Columns. For our 6-key macro pad, a 2x3 matrix means we only need 5 microcontroller pins!
- The Arduino sets all columns to `INPUT_PULLUP`. It then pulls Row 0 `LOW`. It reads the 3 columns. If Col 2 reads `LOW`, we instantly know the key at Matrix(0, 2) is pressed! It then pulls Row 1 `LOW` and repeats. This happens thousands of times a second.

### 2. The Ghosting Problem and Signal Diodes
- **Ghosting** happens when 3 keys sharing rows/columns are pressed simultaneously. Current flows backwards through the closed switches, creating a "sneak path" that tricks the microcontroller into thinking a 4th key is pressed.
- By placing a tiny signal diode (like a 1N4148) on one leg of every single switch, we force the electricity to act as a one-way valve. The sneak path is blocked, granting the keyboard **N-Key Rollover (NKRO)**—the ability to press every key at once perfectly!

### 3. Firing Complex System Hotkeys
```cpp
Keyboard.press(KEY_LEFT_CTRL);
Keyboard.press('c');
delay(20);
Keyboard.releaseAll();
```
- A macro is just a rapid sequence of keypresses. We instruct the USB HID controller to hold down the modifier (`CTRL`), tap the payload key (`C`), and critically, we call `releaseAll()`. 
- If you forget `releaseAll()`, the computer thinks you are permanently holding down the `CTRL` key, which makes using the PC impossible until you reboot!
"""
with open("Day_92_HID_Macro_Pad/README.md", "a", encoding="utf-8") as f:
    f.write(content_day92)

content_day93 = """
## 🧠 Code Explanation

Let's break down how we write robust drivers for analog human-interface devices:

### 1. The Necessity of Deadzones
```cpp
if (val <= lowerThreshold) return 0;
if (val >= upperThreshold) return 1023;
```
- **Inner Deadzone:** Humans cannot hold their feet perfectly still. Resting your foot on the brake pedal might cause the sensor to read 2% pressure, causing your sim racing car to drag its brakes constantly. The inner deadzone ignores all inputs below a certain threshold (e.g., 8%).
- **Outer Deadzone:** Metal springs compress, potentiometers drift, and rubber bumpers wear down. If physical wear prevents the sensor from hitting 100%, you will never reach full throttle. The outer deadzone artificially guarantees 100% output at, say, 95% physical pedal travel.

### 2. Dynamic Auto-Calibration
```cpp
if (rawT > throttleCal.maxRaw) throttleCal.maxRaw = rawT;
```
- Potentiometers differ wildly from factory to factory. We don't want to hardcode analog limits (`0` to `1023`) because a pedal might physically only sweep between `200` and `800`.
- In Calibration Mode, the code aggressively tracks the absolute minimum and maximum voltages it has ever seen. The driver pushes the pedal to the floor, the Arduino saves that max voltage, and sets it as the new 100% ceiling dynamically!

### 3. Linear Scaling (Interpolation)
```cpp
map(val, lowerThreshold, upperThreshold, 0, 1023);
```
- Once we know the true physical lower and upper limits of the sensor, we use the `map()` function to stretch that active analog span across the high-resolution 10-bit range (`0` to `1023`) expected by the USB Game Controller API.
"""
with open("Day_93_Sim_Racing_Pedals/README.md", "a", encoding="utf-8") as f:
    f.write(content_day93)

content_day94 = """
## 🧠 Code Explanation

Let's break down how a Finite State Machine guarantees perfect data transmission:

### 1. The Problem with String Parsing
- Using `Serial.parseInt()` or `Serial.readStringUntil('\\n')` is incredibly slow and CPU intensive. Worse, if a single byte is lost due to electrical noise, the entire string shifts, causing catastrophic robotic failure.

### 2. Binary Framing Protocol
- We construct a highly rigid data packet:
  `[Start Byte (0x02)] [Length] [Command ID] [Payload Data] [Checksum] [End Byte (0x03)]`
- The `0x02` tells the Arduino to wake up and expect a packet. The `Length` tells it exactly how many payload bytes to read.

### 3. The Finite State Machine (FSM)
```cpp
switch (currentState) {
  case STATE_WAIT_SOF:
    if (val == 0x02) currentState = STATE_READ_LEN;
    break;
```
- The FSM evaluates one single byte at a time in a completely non-blocking loop. 
- If the Arduino is in the `STATE_WAIT_SOF` state, it ignores all incoming bytes until it sees `0x02`. Then it cleanly transitions to the next state.

### 4. Checksum Data Validation
```cpp
calculatedChecksum ^= val; // Cumulative XOR
```
- To ensure no payload bytes were corrupted over the wire, the sender XORs all the data bytes together and attaches the result as a "Checksum" byte at the end of the packet.
- The Arduino recalculates the XOR sum as it receives the bytes. If the Arduino's sum does not perfectly match the sender's Checksum byte, it means electrical noise flipped a bit in transit. The packet is instantly discarded to prevent the robot from executing a corrupted, dangerous command!
"""
with open("Day_94_Serial_Parser/README.md", "a", encoding="utf-8") as f:
    f.write(content_day94)

content_day95 = """
## 🧠 Code Explanation

Let's break down how the Geometric Inverse Kinematics solver works:

### 1. Base Yaw (XY Plane Projection)
```cpp
float theta1_rad = atan2(y, x);
```
- The first degree of freedom is the base rotating left and right. This is easily solved using `atan2()` to find the angle to the target coordinate on the 2D floor plane.

### 2. 2D Slice and Distance Calculation
```cpp
float r = sqrt(x*x + y*y);
float S = sqrt(r*r + z_prime*z_prime);
```
- We collapse the 3D problem into a 2D vertical slice. `r` is the horizontal distance from the base, and `z_prime` is the vertical height. 
- `S` calculates the direct diagonal distance from the shoulder joint to the target end-effector location using the Pythagorean theorem.

### 3. The Law of Cosines (Elbow & Shoulder)
```cpp
float cosBeta = (L1*L1 + L2*L2 - S*S) / (2 * L1 * L2);
float beta = acos(cosBeta);
```
- We now have a triangle formed by the upper arm (L1), the forearm (L2), and the diagonal distance (S).
- We apply the **Law of Cosines**, a fundamental trigonometric principle, to solve for the interior angles of this triangle!
- The angle `beta` directly dictates how far the elbow servo needs to bend to connect L1 to L2 at the exact target distance!

### 4. Workspace Singularities
- The solver checks if `S > (L1 + L2)`. If the target is physically further away than the two arm links stretched out straight, the code rejects the command. This prevents the mathematical `acos()` function from returning `NaN` (Not a Number) and crashing the robot!
"""
with open("Day_95_Robotic_Arm_IK/README.md", "a", encoding="utf-8") as f:
    f.write(content_day95)

content_day96 = """
## 🧠 Code Explanation

Let's break down how a robot tracks its exact coordinates without GPS:

### 1. Reading Quadrature Ticks
```cpp
float dL = (float)deltaL * DISTANCE_PER_TICK;
float dR = (float)deltaR * DISTANCE_PER_TICK;
```
- As the wheels spin, hardware interrupts count encoder ticks. We multiply those ticks by the physical circumference of the wheel. 
- If the left wheel rolled 5.2 cm (`dL`) and the right wheel rolled 5.8 cm (`dR`), the robot has moved in an arc.

### 2. Linear and Angular Displacement
```cpp
float dC = (dL + dR) / 2.0f;
float dTheta = (dR - dL) / WHEEL_TRACK;
```
- The robot's center point moved the exact average of both wheels (`dC`).
- The robot's heading (rotation) changed based on the difference between the wheels divided by the physical width of the robot (`WHEEL_TRACK`). If the right wheel moves more than the left, the robot is curving left!

### 3. Runge-Kutta Midpoint Integration
```cpp
float midTheta = robotTheta + (dTheta / 2.0f);
robotX += dC * cos(midTheta);
robotY += dC * sin(midTheta);
```
- To update our 2D global coordinates (X, Y), we use basic trigonometry (SOH CAH TOA).
- We use the **Midpoint Method**: instead of projecting our movement using our old heading, or our new heading, we calculate the exact middle heading of the arc. This massively reduces integration errors and keeps our X, Y coordinates highly accurate over long distances!
"""
with open("Day_96_Differential_Drive_Odometry/README.md", "a", encoding="utf-8") as f:
    f.write(content_day96)

content_day97 = """
## 🧠 Code Explanation

Let's break down how S-Curve mathematics prevent stepper motors from stalling:

### 1. The Problem with Instant Acceleration
- Stepper motors move a heavy physical rotor using magnetic fields. If you command a stepper to jump instantly from 0 to 1000 RPM, the magnetic field spins too fast for the rotor's inertia to catch up. The magnetic lock breaks, the motor squeals horribly, and misses steps.

### 2. Raised Cosine (S-Curve) Math
```cpp
float sCurveFraction = (1.0f - cos(PI * fraction)) / 2.0f;
float sCurveVel = vStart + (vMax - vStart) * sCurveFraction;
```
- While a linear (Trapezoidal) ramp increases speed at a constant rate, an S-Curve ramp uses a cosine wave.
- It starts accelerating very gently, ramps up aggressively in the middle, and then smoothly tapers off as it reaches peak velocity. 
- Because the rate of change of acceleration (Jerk) is minimized, the motor doesn't resonate, allowing it to reach much higher top speeds without stalling!

### 3. Precomputed Lookup Tables
```cpp
sCurveIntervals[i] = (uint16_t)(1000000.0f / sCurveVel);
```
- Calculating floating-point trigonometry (`cos()`) is very slow on an 8-bit Arduino.
- To ensure we can fire stepper pulses at microsecond accuracy without lag, we pre-calculate the delay interval for all 100 steps of the ramp during `setup()`, and store them in an array in memory.

### 4. Non-Blocking Step Execution
```cpp
if (currentMicros - lastStepMicros >= currentIntervalUs) {
    digitalWrite(STEP_PIN, HIGH);
```
- Instead of using `delayMicroseconds()`, which freezes the entire CPU, we use a `micros()` tracking system. The CPU is free to parse Serial commands or run calculations, and fires the physical STEP pulse the exact microsecond the precomputed interval expires!
"""
with open("Day_97_Stepper_S_Curve/README.md", "a", encoding="utf-8") as f:
    f.write(content_day97)

content_day98 = """
## 🧠 Code Explanation

Let's break down the legendary algorithm used in the Apollo moon landings:

### 1. The Sensor Problem
- **Accelerometers** measure gravity. They give a mathematically perfect absolute tilt angle, but they are incredibly noisy. Any tiny vibration or bump makes the reading spike violently.
- **Gyroscopes** measure rotational speed. They are incredibly smooth and immune to vibration, but their physical zero-point slowly drifts over time. If you integrate gyro data, your angle will slowly drift until it's completely wrong.

### 2. Step 1: Predict (Trusting the Gyro)
```cpp
kalmanAngle += dt * (newRate - gyroBias);
P[0][0] += dt * (dt * P[1][1] - P[0][1] - P[1][0] + Q_angle);
```
- The Kalman Filter first calculates where it *thinks* the robot is, by integrating the smooth Gyroscope data minus the estimated bias drift.
- It also updates its Error Covariance Matrix (`P`), which tracks how "uncertain" or "unconfident" the algorithm is about its current prediction.

### 3. Step 2: Update (Trusting the Accelerometer)
```cpp
float S = P[0][0] + R_measure;
float K0 = P[0][0] / S; // Kalman Gain
```
- The filter compares its prediction to the noisy Accelerometer reading.
- It calculates the **Kalman Gain**. This is the magic ratio! 
  - If the Accelerometer noise (`R_measure`) is high, the Kalman Gain drops near 0, meaning it ignores the accelerometer and trusts the smooth gyro.
  - If the Gyro uncertainty (`P`) grows too large over time due to drift, the Kalman Gain increases toward 1, meaning it snaps the angle back to the absolute accelerometer reading!

### 4. Optimal Bias Estimation
```cpp
gyroBias += K[1] * y;
```
- The true brilliance of the Kalman filter: it doesn't just estimate the angle. It continuously tracks the error mathematically to reverse-engineer and estimate the exact bias drift of the gyroscope in real-time, subtracting it out dynamically!
"""
with open("Day_98_Kalman_Filter/README.md", "a", encoding="utf-8") as f:
    f.write(content_day98)

content_day99 = """
## 🧠 Code Explanation

Let's break down the industry standard path-tracking algorithm used by AGVs and self-driving cars:

### 1. Global to Local Frame Transformation
```cpp
float lx =  dx * cos(robotTheta) + dy * sin(robotTheta);
float ly = -dx * sin(robotTheta) + dy * cos(robotTheta);
```
- GPS or Odometry gives us the target waypoint in Global coordinates (e.g., X: 40, Y: 25).
- The robot doesn't know what to do with that. We use matrix rotation mathematics to transform the target into the Robot's *Local Coordinate Frame*. 
- `lx` is now "how far forward the target is", and `ly` is "how far to the left the target is" relative to the robot's hood!

### 2. The Look-Ahead Distance (Ld)
- A human driving a car doesn't stare at the bumper; they look 20 meters down the road.
- The algorithm searches the path array for the first waypoint that is further away than our designated "Look-Ahead Distance" (e.g., 12cm) and aims for that.

### 3. Calculating Steering Curvature (Gamma)
```cpp
float gamma = (2.0f * ly) / (Ld * Ld);
```
- The core of Pure Pursuit: we want to draw a perfect circular arc connecting the robot's rear axle to the Look-Ahead target.
- Using basic geometry, the curvature of that arc (`gamma`) is mathematically derived directly from the lateral offset (`ly`) and the look-ahead distance (`Ld`). 

### 4. Differential Drive Motor Mapping
```cpp
float vL = vTarget * (1.0f - (gamma * WHEEL_TRACK / 2.0f));
float vR = vTarget * (1.0f + (gamma * WHEEL_TRACK / 2.0f));
```
- Once we know the required curvature, we map it to the physical wheels.
- If the curve goes left (positive `gamma`), the math automatically slows down the left wheel (`vL`) and speeds up the right wheel (`vR`) perfectly, forcing the robot onto the circular arc intersecting the target!
"""
with open("Day_99_Pure_Pursuit_Path_Tracking/README.md", "a", encoding="utf-8") as f:
    f.write(content_day99)

content_day100 = """
## 🧠 Code Explanation

Let's break down the capstone architecture that allows Arduinos to run under the Robot Operating System (ROS):

### 1. Robust Serial Protocol Framing
```cpp
uint8_t packet[9] = { SOF, len, msgType, val1, val2, val3, val4, CS, EOF };
```
- ROS cannot tolerate corrupted Serial strings. We use a binary protocol utilizing a Start of Frame (`0x02`), an End of Frame (`0x03`), and a mathematical XOR Checksum byte. 
- If a single bit flips due to electrical noise, the checksum fails, and the Arduino safely drops the packet instead of causing a robotic collision.

### 2. The Subscriber: `/cmd_vel`
```cpp
int16_t rawLinearX  = (rxBuffer[1] << 8) | rxBuffer[2];
float linearX  = (float)rawLinearX / 1000.0f;
```
- When the ROS PC wants the robot to move, it sends a `Twist` message over the `/cmd_vel` topic.
- Because sending raw 32-bit floating point numbers over Serial is messy, the PC multiplies the float by 1000 and sends it as a 16-bit integer. The Arduino shifts the bytes together, divides by 1000, and perfectly reconstructs the `linearX` (m/s) and `angularZ` (rad/s) velocities!
- The Arduino then runs differential kinematics to convert `linearX/angularZ` into target RPMs for the left and right wheel PID controllers.

### 3. The Publisher: `/odom`
```cpp
int16_t scaleX = (int16_t)(odomX * 100.0f);
```
- For ROS to navigate (SLAM), it needs to know exactly where the robot is.
- At 10 Hz, the Arduino runs dead-reckoning kinematics to track its `X`, `Y`, and `Theta` heading. It scales these floats into 16-bit integers, packs them into a binary frame with a `0x20` message type, and blasts it up the USB cable to the PC!
- The PC ROS Node decodes this binary packet, converts it into an `nav_msgs/Odometry` message, and publishes it to the global ROS network, closing the autonomous loop!
"""
with open("Day_100_ROS_Serial_Bridge/README.md", "a", encoding="utf-8") as f:
    f.write(content_day100)

import os

content_day71 = """
## 🧠 Code Explanation

Let's break down how we securely write to physical media:

### 1. The SPI Level-Shifting Interface
```cpp
// Check if card is present and can be initialized
if (!SD.begin(CS_PIN)) {
  // Initialization failed!
}
```
- SD Cards use logic levels of 3.3V, but the Arduino operates at 5.0V. We use a module with an onboard regulator and a 74LVC125A level-shifter chip to translate the 5V signals to safe 3.3V signals.
- The `SD.begin()` function sends a raw SPI "wake up" command to the card. If the card isn't physically inserted, or the FAT file system is corrupted, it returns false, letting us know it's unsafe to proceed.

### 2. Physical Writes and File Flushing
```cpp
myFile.print(timestamp);
myFile.flush();
myFile.close();
```
- Writing data to an SD card is physically very slow compared to the Arduino's CPU. The `SD.h` library uses a 512-byte SRAM cache. When we call `.print()`, it actually just writes to this fast RAM cache.
- If power is pulled before the cache is written to the physical SD card, data is permanently lost.
- By explicitly calling `flush()` and `close()`, we force the Arduino to halt and physically commit the SRAM cache to the non-volatile SD flash memory, ensuring perfect data integrity!
"""
with open("Day_71_SD_Card_Writing/README.md", "a", encoding="utf-8") as f:
    f.write(content_day71)

content_day72 = """
## 🧠 Code Explanation

Let's break down how we built a robust CSV datalogging engine:

### 1. Formatting for Data Science (CSV)
```cpp
if (!SD.exists(CSV_FILENAME)) {
  myFile.println(F("Timestamp_ms,LDR_Raw,Pot_Raw,Voltage_V"));
}
```
- CSV (Comma-Separated Values) is the universal language for data analysis. Every spreadsheet program and Python data library natively understands it.
- On boot, we check if the file exists. If it's brand new, we automatically inject a "Header Row" containing the names of our variables separated by commas.
- During logging, we separate every reading with `,` and end the line with `\n` (`println`), creating a perfectly structured grid of data over time!

### 2. State-Driven Safe Ejection
```cpp
if (isLogging) {
  myFile.println(F("# LOGGER: Logging session started."));
} else {
  myFile.println(F("# LOGGER: Logging session terminated gracefully."));
}
```
- Yanking an SD card out while the Arduino is actively writing to the FAT filesystem will corrupt the entire card.
- We implemented a debounced toggle button. When pressed to STOP, the Arduino finalizes the file by writing a termination comment and closing all hardware handles. The status LED turns off, signaling to the human that it is 100% safe to eject the card.
"""
with open("Day_72_CSV_Datalogger/README.md", "a", encoding="utf-8") as f:
    f.write(content_day72)

content_day73 = """
## 🧠 Code Explanation

Let's break down how we communicate directly with the MPU6050 hardware:

### 1. I2C Initialization and Waking the Sensor
```cpp
Wire.beginTransmission(MPU6050_ADDR);
Wire.write(REG_PWR_MGMT_1); // 0x6B
Wire.write(0x00);           // Wake up
```
- By default, the MPU6050 boots into Sleep Mode to save power. 
- We use the I2C bus (`Wire.h`) to open a line to address `0x68`, point the hardware register to `0x6B` (Power Management 1), and overwrite it with `0x00`. The internal MEMS oscillator wakes up, and the sensor begins continuously sampling physics data!

### 2. Atomic Burst Reading and Reassembly
```cpp
Wire.requestFrom(MPU6050_ADDR, (uint8_t)6);
x = (Wire.read() << 8) | Wire.read();
```
- If we read the X, Y, and Z registers one at a time with delays in between, the robot might have moved, causing the X and Z data to represent different points in time!
- We execute an Atomic Burst Read: We request 6 bytes simultaneously. The MPU6050 hardware freezes its data buffer and streams all 6 bytes (High and Low for each axis) instantly over I2C.
- Because the data is 16-bit but I2C only sends 8-bit bytes, we shift the High byte left by 8 bits (`<< 8`) and use bitwise OR (`|`) to stitch the Low byte to it, reconstructing the true 16-bit integer!
"""
with open("Day_73_MPU6050_Raw/README.md", "a", encoding="utf-8") as f:
    f.write(content_day73)

content_day74 = """
## 🧠 Code Explanation

Let's break down how we calculate rotational angles using Integral Calculus:

### 1. Calibration and Bias Removal
```cpp
float gx = (rawX - gyroOffsetX) / GYRO_SCALE_FACTOR;
```
- A gyroscope doesn't measure angles; it measures the *speed of rotation* (°/second).
- Because of microscopic manufacturing imperfections and temperature, the sensor will report a tiny rotation speed (e.g., 2 °/s) even when sitting perfectly still on a desk! This is the "Zero-Rate Offset".
- On boot, we take 200 samples while perfectly still and average them to find this offset. By subtracting this offset from all future readings, we virtually eliminate false rotation!

### 2. Numerical Integration (Area Under the Curve)
```cpp
float dt = (currentMicros - lastLoopTime) / 1000000.0f;
angleX += gx * dt;
```
- To find the angle, we must multiply the speed of rotation by the time elapsed ($Distance = Speed \times Time$).
- We use `micros()` to find exactly how much time (`dt`) has passed since the last loop.
- We multiply our current rotation speed (`gx`) by `dt` to find how many degrees we moved in that tiny fraction of a second, and add (`+=`) it to our running total. This mathematical process is called Numerical Integration!
"""
with open("Day_74_MPU6050_Gyro/README.md", "a", encoding="utf-8") as f:
    f.write(content_day74)

content_day75 = """
## 🧠 Code Explanation

Let's break down how we achieve perfect stability with Sensor Fusion:

### 1. The Flaws of Individual Sensors
- **Accelerometers** are noisy. Any vibration from motors causes the reading to spike wildly. However, in the long term, they always point straight down toward Earth's gravity.
- **Gyroscopes** are clean and immune to vibration. However, due to the integration math, any tiny microscopic error accumulates over time, causing the angle to drift away into infinity!

### 2. The Complementary Filter Algorithm
```cpp
fusedRoll = ALPHA * (fusedRoll + gx * dt) + (1.0f - ALPHA) * accRoll;
```
- The Complementary Filter mathematically fuses both sensors to get the best of both worlds. 
- `ALPHA` is our weight (e.g., 0.96). 
- We trust the Gyroscope's integrated angle for 96% of our calculation. This acts as a High-Pass Filter, reacting instantly and smoothly to rapid movement while ignoring vibrations.
- We trust the Accelerometer's gravity angle for the remaining 4%. This acts as a Low-Pass Filter. Over a period of a few seconds, it gently tugs the total calculation back toward absolute zero, completely eliminating the gyroscope's drift!
"""
with open("Day_75_MPU6050_Pitch_Roll/README.md", "a", encoding="utf-8") as f:
    f.write(content_day75)

content_day76 = """
## 🧠 Code Explanation

Let's break down how we built a life-saving Fall Detection algorithm:

### 1. Vector Magnitude and Free-Fall Physics
```cpp
float aTot = sqrt(ax*ax + ay*ay + az*az);
if (aTot < THRES_FREE_FALL) { // aTot < 0.4g
```
- When you stand still, gravity exerts a 1.0g force on you. When you jump out of a plane (or fall), you are in "free-fall", and the sensors read close to 0.0g!
- We use the 3D Pythagorean Theorem to calculate the total magnitude of acceleration across all axes. If this total drops below 0.4g, the state machine triggers Phase 1: Free-Fall detected!

### 2. The Post-Fall Inactivity Check
```cpp
if (now - inactivityStartTime >= TIME_INACTIVITY_REQ) {
  if (tilt > THRES_TILT) {
     // TRIGGER ALARM
```
- A person jumping off a chair experiences free-fall and a massive impact spike upon landing, but this isn't a medical emergency!
- To prevent false alarms, our Finite State Machine (FSM) implements a Post-Fall phase. It waits 2 seconds. If the person stood up (movement detected), the alarm cancels. 
- If the sensor remains perfectly still *and* the `tilt` angle calculation shows the sensor is lying flat (not upright), we assume the person is incapacitated and trigger the siren!
"""
with open("Day_76_Fall_Detection/README.md", "a", encoding="utf-8") as f:
    f.write(content_day76)

content_day77 = """
## 🧠 Code Explanation

Let's break down the control logic for an inverted pendulum (Self-Balancing Robot):

### 1. The PID Control Loop
```cpp
float error = targetAngle - currentAngle;
float pTerm = Kp * error;
float iTerm = Ki * errorIntegral;
float dTerm = Kd * ((error - lastError) / dt);
return pTerm + iTerm + dTerm;
```
- A self-balancing robot is inherently unstable. If it tilts forward, it must drive its wheels forward to "catch" itself.
- We use a **Proportional-Integral-Derivative (PID)** algorithm to calculate the perfect motor speed.
- **P (Proportional)** pushes harder the further it tilts.
- **I (Integral)** accumulates error over time to overcome steady-state friction (e.g. if the floor is slightly slanted).
- **D (Derivative)** predicts the future. If the robot is tilting back toward zero very fast, D pushes *backwards* to act like a brake, preventing it from overshooting and falling the other way!

### 2. Anti-Windup Clamping
```cpp
errorIntegral = constrain(errorIntegral, -MAX_INTEGRAL, MAX_INTEGRAL);
```
- If a human holds the robot and forces it to stay tilted, the `errorIntegral` variable will add up to infinity! When the human lets go, the robot will fly across the room at max speed until the integral slowly unwinds.
- We "clamp" the accumulator using `constrain()`. This "Anti-Windup" technique guarantees the integral term can never exceed a mathematically safe limit.
"""
with open("Day_77_Self_Balancing/README.md", "a", encoding="utf-8") as f:
    f.write(content_day77)

content_day78 = """
## 🧠 Code Explanation

Let's break down how we parse raw satellite data from the cosmos:

### 1. Serial Stream Buffering and Sentence Extraction
```cpp
if (c == '$') {
  isRecording = true;
} else if (isRecording && (c == '\r' || c == '\n')) {
  // Parse complete sentence
}
```
- GPS modules blindly blast text data out of their TX pin at 9600 baud.
- Instead of using a library, we built a Ring Buffer. We ignore everything until we see the start character (`$`). We then record every character into a character array (`sentenceBuffer`) until we see a newline (`\n`). 
- We now have a perfectly isolated NMEA sentence string ready for analysis!

### 2. Tokenizing and Coordinate Translation
```cpp
int degrees = (int)(rawValue / 100.0f);
float minutes = rawValue - (degrees * 100.0f);
float decimalDegrees = degrees + (minutes / 60.0f);
```
- The `$GPRMC` sentence gives us coordinates in "Degrees-Minutes" format (e.g., `4807.038`).
- Google Maps and modern APIs require "Decimal Degrees".
- Because the first two (or three) digits are Degrees and the remaining digits are Minutes, we divide by 100 and cast to an `int` to cleanly slice off the Degrees. We then divide the remaining Minutes by 60 to convert them to a decimal, yielding the exact Decimal Degree format!
"""
with open("Day_78_GPS_Parser/README.md", "a", encoding="utf-8") as f:
    f.write(content_day78)

content_day79 = """
## 🧠 Code Explanation

Let's break down how we navigate the globe using spherical geometry:

### 1. The Haversine Formula (Great-Circle Distance)
```cpp
float a = sin(dLat/2)*sin(dLat/2) + cos(lat1)*cos(lat2) * sin(dLon/2)*sin(dLon/2);
float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
float distance = EARTH_RADIUS_M * c;
```
- You cannot use standard algebra (Pythagorean theorem) to measure distance on a global scale because the Earth is a sphere, not flat!
- We implemented the Haversine Formula, a robust spherical trigonometry algorithm used by aircraft and ships. It calculates the shortest path over the curved surface of the Earth between two points.
- By multiplying the resulting angle by the radius of the Earth ($6,371,000$ meters), we get an incredibly accurate distance measurement in meters!

### 2. Forward Bearing (Compass Heading)
```cpp
float y = sin(dLon) * cos(lat2);
float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
float bearing = atan2(y, x) * RAD_TO_DEG;
```
- Distance alone isn't enough; an autonomous robot needs to know *which direction* to drive to get Home.
- This formula computes the initial bearing angle. We use `atan2()`, which intelligently handles all four quadrants of the coordinate plane.
- The result is an angle from 0° to 360°, pointing directly True North (0°), East (90°), South (180°), or West (270°)!
"""
with open("Day_79_GPS_Distance/README.md", "a", encoding="utf-8") as f:
    f.write(content_day79)

content_day80 = """
## 🧠 Code Explanation

Let's break down how we built a Master/Slave hardware network using I2C:

### 1. The Master's Role: Orchestration
```cpp
Wire.requestFrom(SLAVE_I2C_ADDR, 2);
byte high = Wire.read();
byte low  = Wire.read();
uint16_t val = (high << 8) | low;
```
- In an I2C network, the Slave is completely passive. The Master (Mode 1) dictates the rhythm of the entire bus.
- Every 1 second, the Master asserts the SCL clock line and requests exactly 2 bytes of data from address `0x08`. It blocks, waits for the Slave to push the bytes across the SDA wire, and then mathematically reassembles the 16-bit sensor data!

### 2. The Slave's Role: Interrupt-Driven Callbacks
```cpp
Wire.onRequest(handleMasterRequest);
// ...
void handleMasterRequest() {
  Wire.write(highByte);
  Wire.write(lowByte);
}
```
- The Slave (Mode 0) runs its own code in `loop()`, reading sensors. It does not pause to poll for network traffic.
- When the Master initiates communication, the hardware I2C peripheral inside the Slave's ATmega328P instantly halts the CPU and triggers an Interrupt Service Routine (ISR).
- The `handleMasterRequest` function executes instantaneously, blasting the data onto the bus, and then the Slave seamlessly returns to its `loop()` as if nothing happened!
"""
with open("Day_80_I2C_Network/README.md", "a", encoding="utf-8") as f:
    f.write(content_day80)

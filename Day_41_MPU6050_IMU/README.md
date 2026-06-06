# Day 41: MPU6050 6-Axis IMU (Direct I2C Register Reading)

Welcome to Day 41 of the 100-Day Arduino Masterclass! Today, we explore inertial navigation and study how to program an **Inertial Measurement Unit (IMU)** at the register level. We will interface the standard **MPU6050 6-Axis IMU** over the **I2C bus** using raw register transactions with `Wire.h`, configure device power registers, record gyroscope calibration biases, and translate binary sensor data into physical units.

---

## 🎯 The "Why" and "What"

An IMU is the sensory core of any self-balancing or flight controller. Examples include:
1. **Drones & Quadcopters:** Measuring pitch, roll, and yaw angles at high speed to maintain flight stability.
2. **Self-Balancing Robots:** Detecting tilt angles so the robot can drive its wheels to stay upright.
3. **VR/AR Headsets:** Tracking user head rotation in 3D space.
4. **Autonomous Vehicles:** Integrating acceleration and angular rates to estimate vehicle position (Dead Reckoning).

The MPU6050 is a low-cost, high-precision MEMS sensor containing a **3-axis accelerometer** and a **3-axis gyroscope** in a single package.

---

## 🔬 Physics & Sensor Register Theory

### 1. MEMS Sensor Chemistry & Physics
* **Capacitive MEMS Accelerometer:** Inside the chip are micro-machined silicon structures suspended by spring-like beams. When the chip accelerates (or tilts under gravity), the structures shift position, changing the electrical capacitance between fixed fingers and the moving masses. The chip converts this change in capacitance into a digital voltage.
* **Coriolis-Force MEMS Gyroscope:** Contains micro-machined masses kept in constant vibration. When the chip rotates, the Coriolis effect forces the vibrating mass to shift sideways. The chip measures this lateral displacement capacitively to determine angular rate (degrees per second).

---

### 2. Register Pointer Addressing
We access the MPU6050 registers by:
1. Establishing I2C connection to device address `0x68`.
2. Writing the starting register pointer (e.g. `0x3B` for Accelerometer data start).
3. Sending a "Repeated Start" command and requesting the target number of bytes. The MPU6050's internal memory pointer auto-increments after each read, allowing us to read X, Y, and Z axes in a single burst:

```cpp
Wire.beginTransmission(0x68);
Wire.write(0x3B); // Point to Accel X High Register
Wire.endTransmission(false); // Repeated start
Wire.requestFrom(0x68, 6); // Read 6 sequential bytes (X_H, X_L, Y_H, Y_L, Z_H, Z_L)
```

---

### 3. Signed 16-bit Two's Complement Reassembly
The ADC converts physical readings into signed 16-bit integers stored across two separate bytes (High Byte and Low Byte). We reassemble these bytes using bitwise shift and OR operations:
```cpp
int16_t rawValue = (highByte << 8) | lowByte;
```

---

### 4. Scaling Factors & Sensor Calibrations
* **Accelerometer Sensitivity:** By default, the accelerometer is configured for the $\pm 2g$ scale. This maps the 16-bit ADC steps to a resolution of **$16384\text{ LSB}/g$**:
  $$\text{Acceleration }(g) = \frac{\text{Raw Reading}}{16,384}$$
* **Gyroscope Sensitivity:** By default, the gyroscope is configured for $\pm 250^{\circ}/\text{s}$. This maps to a resolution of **$131\text{ LSB}/(^{\circ}/\text{s})$**:
  $$\text{Angular Velocity }(^{\circ}/\text{s}) = \frac{\text{Raw Reading}}{131}$$
* **Gyro Offset Calibration:** Gyroscopes suffer from stationary bias (constant non-zero readings even when completely still). On setup, the sketch captures 50 samples while stationary to calculate these bias values, subtracting them from active loops.

---

## 🔄 Alternatives Comparison

When selecting IMUs for autonomous projects:

| Sensor Chip | Degrees of Freedom (DOF) | Bus Interface | Onboard Fusion? | Calibration Complexity | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **MPU6050** | **6-Axis (Accel + Gyro)** | **I2C ($400\text{ kHz}$)** | **DMP (Digital Motion Processor)**| **Medium (Needs software filtering)** | **Self-balancing bots, camera gimbals (Our choice)** |
| **MPU9250** | **9-Axis (Accel + Gyro + Mag)**| **SPI ($20\text{ MHz}$) / I2C** | **DMP** | **High (Compass requires hard/soft iron calibration)** | **Autonomous drones, heading reference systems (AHRS)** |
| **LSM6DS3** | **6-Axis (Accel + Gyro)** | **SPI / I2C** | **Interrupt Engine** | **Medium** | **Wearable devices, low-power step counting** |
| **BNO055** | **9-Axis (Accel + Gyro + Mag)**| **I2C** | **Yes (Internal ARM Cortex-M0)**| **Very Low (Calculated on-chip)** | **High-level robotics, rapid prototyping (Expensive)** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x MPU6050 Breakout Board (GY-521)
* 1x Breadboard
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

| MPU6050 Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **5V** or **3.3V** (Most breakout boards have regulator) | Red | Power input |
| **GND** | **GND** | Black | Ground reference |
| **SCL** | **A5** (or dedicated SCL) | Yellow | I2C Serial Clock |
| **SDA** | **A4** (or dedicated SDA) | Green | I2C Serial Data |
| **AD0** | **Not Connected** | - | Address Select (Default: Address 0x68) |

---

## 💻 How to Test & Validate

1. Connect the MPU6050 module to the Arduino using the wiring table. Place the sensor flat and perfectly still on a desk.
2. Upload `Day_41_MPU6050_IMU.ino` to your Arduino.
3. Open the **Serial Monitor** at **9600 Baud**.
4. You will see calibration logs start:
   `[IMU] Calibrating Gyroscope. DO NOT MOVE SENSOR...`
   Once complete, it prints offset biases (e.g. `X: -1.24 Y: 0.85 Z: -0.12`).
5. Observe the live data readout:
   * **Gravity Check:** If the sensor lies flat, the Z-axis acceleration should read $\approx 1.00g$, while X and Y read close to $0.00g$.
   * **Accelerometer Test:** Tilt the sensor forward. The X-axis acceleration will change towards $+1.00g$ or $-1.00g$ depending on tilt direction, as gravity acts on the axis.
   * **Gyroscope Test:** Rotate the sensor quickly. You will see the gyroscope readings spike to hundreds of degrees per second, returning back to zero as rotation stops.
   * **Temperature Test:** Place your finger on the sensor chip. The temperature reading will begin rising.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **Serial prints `[ERROR] Failed to communicate with MPU6050 over I2C! Check wiring.`:**
  * Ensure the SDA pin connects to **A4** and SCL connects to **A5**.
  * Check power connections. A red power LED on the MPU6050 board should be lit.
  * Verify I2C address. Pulling the AD0 pin to 5V changes the I2C address to `0x69`. Ensure AD0 is disconnected to maintain `0x68`.
* **The Gyroscope readings drift slowly (incrementing or decrementing continuously at rest):**
  * Gyroscopes measure *angular rate* (speed of rotation), not absolute angle. Static drift is normal. Ensure the sensor did not move during the calibration sweep in `setup()`.
* **The Z-axis acceleration does not read $1.00g$ at rest:**
  * Ensure the sensor is lying flat on a level surface. If it is tilted, gravity is distributed across multiple axes.
  * MEMS sensors have manufacturing offset errors. You can calibrate the accelerometer by recording the Z-axis offset and subtracting it.

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

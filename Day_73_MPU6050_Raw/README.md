# Day 73: MPU6050 Accelerometer Raw Data (Direct I2C Register Programming)

Welcome to Day 73! Today we interface the high-performance **MPU6050 6-Axis Inertial Measurement Unit (IMU)**. Rather than relying on heavy pre-written libraries, we will communicate directly with the sensor registers over the **I2C bus** using `Wire.h`. We will study the physics of **MEMS accelerometers**, handle signed 16-bit register parsing, and convert raw data into standard physical units of Gravity ($g$).

---

## 🎯 The "Why" and "What"

An **Inertial Measurement Unit (IMU)** is the sensory organ of movement. In robotics, self-balancing systems, and drones, an IMU provides state estimation:
1. **Tilt & Orientation**: Measuring angles relative to gravity to balance a two-wheeled robot or stabilize a quadcopter.
2. **Impact & Vibration**: Detecting collisions, falls, or rough terrain.
3. **Dead Reckoning**: Estimating velocity and relative displacement by double-integrating acceleration values over time.

Interfacing at the register level allows us to configure full-scale sensitivity ranges directly, perform efficient block I2C reads, and minimize microcontroller memory footprint.

---

## 🔬 Physics & Hardware Theory

### 1. MEMS Accelerometer Physics
The MPU6050 contains a Micro-Electro-Mechanical Systems (MEMS) accelerometer on a single silicon die.
- **Differential Capacitance**: The sensor consists of a tiny silicon proof mass suspended by silicon springs between fixed plates. When acceleration is applied (due to gravity or motion), the proof mass deflects, altering the distance between the plates. This change in distance changes the electrical capacitance:
  $$C = \epsilon \cdot \frac{A}{d}$$
- **Voltage Output**: On-chip circuitry translates this differential capacitance change into a voltage, which is then digitized by an internal 16-bit Analog-to-Digital Converter (ADC) per axis.

```
       Fixed Plate      Proof Mass      Fixed Plate
         ├───┐             ┌───┐             ┌───┤
         │   │  ◄──Spring──┤   ├──Spring──►  │   │
         │   │             │   │             │   │
         └───┘             └───┘             └───┘
         ◄───── d1 ───────► ◄────── d2 ─────►
```

### 2. Full-Scale Range & LSB Sensitivity
The internal ADC yields values between `-32768` and `+32767` (signed 16-bit integer). We map these raw numbers to physical units of Gravity ($g$, where $1\,g \approx 9.81\,\text{m/s}^2$).
The scaling factor depends on the **Full Scale Range** configured in register `0x1C` (`ACCEL_CONFIG`):

| Config Value | Full Scale Range | Sensitivity Scale Factor |
| :--- | :--- | :--- |
| **`0x00` (Our choice)** | **±2g** | **16384 LSB / g** |
| `0x08` | ±4g | 8192 LSB / g |
| `0x10` | ±8g | 4096 LSB / g |
| `0x18` | ±16g | 2048 LSB / g |

To get the physical acceleration ($a_x$), we divide the raw integer ($raw_x$) by the scale factor:
$$a_x = \frac{raw_x}{16384.0}$$

### 3. Registers & atomic I2C reads
The accelerometer stores its measurements in 6 consecutive registers starting at `0x3B`:
- `0x3B` / `0x3C`: `ACCEL_XOUT_H` and `ACCEL_XOUT_L`
- `0x3D` / `0x3E`: `ACCEL_YOUT_H` and `ACCEL_YOUT_L`
- `0x3F` / `0x40`: `ACCEL_ZOUT_H` and `ACCEL_ZOUT_L`

If we read these bytes individually, the sensor might update its readings mid-process, leading to skewed vectors. To prevent this, we request **6 bytes in a single transmission** (burst read). The MPU6050 freezes its output registers during the transaction to ensure all axes are sampled at the exact same instant.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Master Controller |
| MPU6050 IMU Module (GY-521) | 1 | 3-axis Accelerometer & Gyroscope |
| Breadboard & Jumper Wires | 1 | Connections |

---

## 🔌 Pin-to-Pin Wiring

| MPU6050 Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** (or 3.3V) | Power Supply (MPU6050 GY-521 has an onboard 3.3V LDO regulator) |
| **GND** | **GND** | Ground |
| **SCL** | **A5** | I2C Serial Clock |
| **SDA** | **A4** | I2C Serial Data |
| **AD0** | **GND** | Configures I2C address to `0x68` (floating/GND = 0x68, VCC = 0x69) |

---

## 💾 Alternatives to MPU6050 Accelerometer

| Sensor | Resolution | Interface | Max Range | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **MPU6050** | 16-bit | I2C | ±16g | Cheap, includes high-performance gyro. |
| **ADXL345** | 13-bit | I2C / SPI | ±16g | Lower power, standard industrial accelerometer. |
| **LIS3DH** | 12-bit | I2C / SPI | ±16g | Ultra-low power, has tap/double-tap interrupt. |
| **MMA8452Q** | 12-bit | I2C | ±8g | High performance, smart low-power features. |

---

## 💻 How to Test & Validate

1. Wire the MPU6050 to the Arduino Uno using the layout table.
2. Load [Day_73_MPU6050_Raw.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_73_MPU6050_Raw/Day_73_MPU6050_Raw.ino) in the Arduino IDE and upload it.
3. Open the **Serial Monitor** at **9600 Baud**.
4. You should see `[SYSTEM] MPU6050 detected and initialized` and lines of raw and scaled readings:
   `RAW -> X: 245	Y: -110	Z: 16402 | ACCEL -> X: 0.015g	Y: -0.007g	Z: 1.001g`
5. Place the sensor flat on your desk:
   - $X$ and $Y$ should read close to $0.0g$.
   - $Z$ should read close to $+1.0g$ (offset by calibration limits).
6. Flip the sensor upside down:
   - $Z$ should swing to $-1.0g$.
7. Shake the sensor in different directions; observe the corresponding axes spike dynamically.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `Failed to communicate with MPU6050!` | Wrong I2C wiring (SDA/SCL swapped) | SDA goes to A4, SCL goes to A5. Double check connections. |
| `Failed to communicate with MPU6050!` | Address mismatch (AD0 Pin) | Tie AD0 pin to GND to force `0x68` address. |
| Readings are static and do not change | MPU6050 remained in Sleep mode | Ensure `PWR_MGMT_1` (0x6B) was set to `0x00` during setup. |
| Z-axis reading fluctuates significantly | Mechanical vibrations | Mount sensor securely. Use a low-pass filter (software) or activate MPU6050 Digital Low Pass Filter (DLPF) register `0x1A`. |

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

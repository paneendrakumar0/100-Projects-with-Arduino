# Day 86: Persistent Sensor Calibration Manager (MPU6050 Boot-trigger Calibration & EEPROM Storage)

Welcome to Day 86! Today we build a production-grade **Persistent Sensor Calibration Manager** for the MPU6050 6-Axis Inertial Measurement Unit (IMU). We will address a common problem in robotics: how to calibrate an IMU without forcing a long calibration cycle on every power cycle, which requires the robot to remain perfectly still. We will save calibration offsets in the internal **EEPROM**, protect them with a **checksum**, and implement a boot-trigger pin logic to force recalibration.

---

## 🎯 The "Why" and "What"

Robotics and aerospace control loops (like PID controllers for balance or quadcopters) rely on clean, zero-biased measurements from an Inertial Measurement Unit (IMU). 
When an IMU is flat and stationary, its sensor readings should theoretically read:
- **Gyroscope X, Y, Z**: $0^\circ/\text{s}$ (no rotation)
- **Accelerometer X, Y**: $0g$ (no lateral acceleration)
- **Accelerometer Z**: $1g$ or $9.81\,\text{m/s}^2$ (standard gravity vector)

However, due to chip manufacturing variations, soldering stresses, and mounting angles, every IMU has a **static bias (offset)**. Without calibration, your robot might think it is drifting or tilting even when sitting flat.

Instead of running a 2-second calibration loop *every single time* the system boots (which would fail if the robot is moving during power-up), this manager:
1. **Reads pre-computed offsets** from the EEPROM on boot.
2. **Applies them instantly** to the raw readings.
3. **Offers a physical override**: holding a button during boot triggers a fresh, 500-sample calibration cycle and writes the new offsets back to EEPROM.
4. **Protects memory integrity**: a magic configuration byte and an arithmetic checksum ensure we never load corrupt or uninitialized data.

---

## 🔬 Physics & Hardware Theory

### 1. IMU Static Bias (Sensor Offset)
Micro-Electro-Mechanical Systems (MEMS) sensors contain microscopic silicon structures suspended in a cavity. These structures flex under acceleration or rotation, changing capacitance. Small structural defects or stresses from soldering warp these plates slightly at rest, creating a constant voltage offset (bias). 
We model this offset simply as:
$$\text{Reading}_{\text{calibrated}} = \text{Reading}_{\text{raw}} - \text{Offset}$$

To calculate the offset, we take $N$ samples ($N=500$ in our code) while the sensor is completely stationary:
$$\text{Offset} = \frac{1}{N} \sum_{i=1}^{N} \text{Reading}_{\text{raw}, i}$$

*Note*: For the Accelerometer Z axis, standard gravity is always acting on it. If the IMU is flat, the raw Z reading should be $1g$ (which corresponds to $16384\,\text{LSB}$ at the default $\pm2g$ sensitivity). Thus:
$$\text{Offset}_Z = \left( \frac{1}{N} \sum_{i=1}^{N} \text{Raw}_Z \right) - 16384$$

### 2. EEPROM Structure & Checksum Protection
The Arduino Uno has 1 KB of internal EEPROM (Electrically Erasable Programmable Read-Only Memory) that survives power cycles. To store our calibration data safely, we pack it into a structured byte block:
- **Magic Byte (1 byte)**: A unique identifier (`0xC4`). If this byte doesn't match on boot, the Arduino knows the EEPROM has not been initialized.
- **6 Floats (24 bytes)**: Offsets for Accel X, Y, Z and Gyro X, Y, Z.
- **Checksum (2 bytes)**: A simple additive 16-bit checksum of the preceding bytes. 

Before loading the offsets, the Arduino recalculates the checksum of the bytes in EEPROM and compares it to the stored checksum. If they match, the data is verified.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| MPU6050 IMU | 1 | 6-Axis Accelerometer & Gyroscope |
| Pushbutton | 1 | Factory calibration trigger |
| LED | 1 | Built-in pin 13 (Calibration state indicator) |
| Breadboard & Jumper Wires | 1 | Prototyping and wiring |

---

## 🔌 Pin-to-Pin Wiring

| MPU6050 Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** | 5V Power Supply |
| **GND** | **GND** | Ground |
| **SDA** | **A4** | I2C Data line (needs no library) |
| **SCL** | **A5** | I2C Clock line |
| **Button Pin** | **D2** | Calibration switch (uses internal pull-up) |

---

## 💾 Alternatives to EEPROM

| Medium | Storage Type | Capacity | Write Endurance | Access Speed | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Internal EEPROM** | Non-volatile | 1 KB | 100,000 writes | Moderate | Zero external wiring. Perfect for calibration parameters. |
| **External I2C EEPROM** (e.g., AT24C256) | Non-volatile | 32 KB | 1,000,000 writes | Slow | Requires extra bus lines, excellent for larger parameter sets. |
| **Flash Memory** (Internal PROGMEM) | Non-volatile | 32 KB | 10,000 writes | Fast | Read-only at runtime. Cannot save new calibrations without reflashing. |
| **FRAM** (Ferroelectric RAM) | Non-volatile | 8 KB - 256 KB | $10^{14}$ writes | Extremely Fast | Highly durable, but expensive and requires external IC. |

---

## 💻 How to Test & Validate

1. Connect the MPU6050 and the button to the Arduino Uno according to the wiring diagram.
2. Open the Arduino IDE, load [Day_86_Calibration_EEPROM.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_86_Calibration_EEPROM/Day_86_Calibration_EEPROM.ino), and select the COM port.
3. Open the **Serial Monitor** at **9600 Baud**.
4. **First Run (Uncalibrated)**:
   - On the first power-up, you will see a warning: `[WARNING] No valid calibration data found in EEPROM. Sensor readings will use 0.0 default offsets.`
5. **Run Calibration**:
   - Place the IMU perfectly flat on a stable, level surface.
   - Send `c` through the Serial Monitor (or hold Pin 2 to Ground and press the Reset button).
   - The indicator LED on Pin 13 will light up. The IMU will collect 500 samples over ~2 seconds.
   - The console will output the computed offsets and confirm they have been saved.
6. **Reboot Test**:
   - Press the Reset button on the Arduino (without holding Pin 2).
   - You should see `[SYSTEM] Pre-stored offsets loaded successfully. Ready.` showing that it loaded the valid calibration from EEPROM instantly!
7. **CLI Commands**:
   - Send `s` to print active calibration parameters in memory.
   - Send `d` to invalidate the EEPROM (simulates an uncalibrated board).

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `[ERROR] Failed to communicate with MPU6050!` | Wrong I2C address or bad wiring | Check SDA (A4) and SCL (A5) connections. Ensure AD0 pin on MPU6050 is grounded (0x68 address). |
| `[WARNING] No valid calibration data found` | First run or checksum mismatch | Trigger a manual calibration sequence using the Serial CLI (`c`) or boot-button pin. |
| Accelerometer Z-axis is calibrated near $-16384$ instead of $0$ | IMU was not held flat during calibration | Lay the IMU perfectly flat on a table so the Z-axis aligns with the gravity vector. |
| Calibration triggers constantly on boot | Button wire is loose or floating | Ensure Pin 2 is connected correctly to the button, and the other button terminal goes to GND. |

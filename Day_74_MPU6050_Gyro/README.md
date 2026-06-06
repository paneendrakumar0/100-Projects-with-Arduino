# Day 74: MPU6050 Gyroscope Angles & Integration (Direct I2C Registers)

Welcome to Day 74! Today we explore **gyroscope sensor physics** and **numerical integration**. Using the **MPU6050 IMU**, we will read angular velocities (degrees per second) over the **I2C bus** and track absolute rotational angles (Roll, Pitch, and Yaw). We will implement a startup calibration routine to calculate sensor bias offsets and study why gyroscopes suffer from **drift** over time.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/MPU6050.jpg" alt="MPU6050" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

An accelerometer measures linear acceleration, but it cannot easily measure rotational speed. A **gyroscope** detects rotation rates. In robotics:
1. **Drones**: Gyroscopes measure angular roll, pitch, and yaw velocities to adjust motor speeds at hundreds of Hertz, keeping the drone stable.
2. **Self-Balancing Robots**: Detecting the angular velocity of falling is the primary input to PID loops to calculate restoring motor torque.
3. **Stabilization Gimbals**: Keeping a camera level by rotating motors in the opposite direction of the sensed angular velocity.

Integrating rotational speed yields position (angles), which is the foundation of navigation.

---

## 🔬 Physics & Hardware Theory

### 1. MEMS Gyroscope Physics (Coriolis Effect)
Inside the MPU6050, the gyroscope utilizes the **Coriolis Effect** to measure rotation.
- **Vibrating Mass**: The sensor contains a silicon proof mass driven into a continuous electrostatic vibration.
- **Coriolis Acceleration**: When the sensor is rotated around an axis, the vibrating mass experiences a Coriolis force perpendicular to both the velocity of vibration and the axis of rotation:
  $$\vec{F}_c = -2m \cdot (\vec{\omega} \times \vec{v})$$
- **Capacitance Shift**: The Coriolis force pushes the mass sideways, altering the gap between differential capacitors. This capacitance shift is processed by internal filters, digitized, and output as a signed 16-bit word.

```
                  ┌───────────────┐
                  │ Vibrating Mass│ (Moves Up/Down)
                  └──────┬┬───────┘
                         ││
       Rotation ◄────────┼┼────────► Coriolis Force
       (Roll/Pitch/Yaw)  ││          (Moves Left/Right)
                  ┌──────┴┴───────┐
                  │Capacitor Plate│ (Senses shift)
                  └───────────────┘
```

### 2. Numerical Integration & Time Deltas
A gyroscope output is angular velocity ($\omega$, in degrees per second). To find the rotation angle ($\theta$, in degrees), we must integrate:
$$\theta = \int \omega \, dt$$

In software, we perform discrete numerical integration at each loop iteration:
$$\theta_t = \theta_{t-1} + \omega_t \cdot dt$$
- **Time Delta ($dt$)**: The elapsed time between loops, measured in seconds. If we run a loop at 100 Hz, $dt \approx 0.01$ seconds. We measure this dynamically using `micros()` to account for loop timing jitter.

### 3. Gyroscope Drift & Calibration
No sensor is perfect. A stationary gyroscope will output a small, non-zero reading (e.g. $+0.15\,\text{°/s}$) due to manufacturing tolerances and thermal changes. This is the **Gyroscope Bias**.
If we integrate this raw bias, the calculated angle will grow continuously:
$$\theta_{\text{drift}} = 0.15\,\text{°/s} \times 60\,\text{s} = 9.0^\circ \text{ of drift per minute}$$

To solve this, we calibrate the sensor during `setup()` by sampling the gyroscope 200 times at rest. We average these values to find the bias offset, which is then subtracted from all subsequent readings.

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
| **VCC** | **5V** | Power Supply |
| **GND** | **GND** | Ground |
| **SCL** | **A5** | I2C Serial Clock |
| **SDA** | **A4** | I2C Serial Data |
| **AD0** | **GND** | Configures I2C address to `0x68` |

---

## 💾 Alternatives to MPU6050 Gyroscope

| Sensor | Channels | Interface | Full-Scale Range | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **MPU6050** | 3 (Gyro) + 3 (Accel) | I2C | ±2000°/s | Most popular hobbyist IMU. Very cost-effective. |
| **L3GD20** | 3 (Gyro only) | I2C / SPI | ±2000°/s | High-performance standalone gyroscope, very low noise. |
| **ITG3200** | 3 (Gyro only) | I2C | ±2000°/s | Legacy digital gyro, 16-bit ADCs. |
| **BMG160** | 3 (Gyro only) | I2C / SPI | ±2000°/s | Bosch high-performance gyro, excellent temperature stability. |

---

## 💻 How to Test & Validate

1. Wire the MPU6050 to the Arduino. Place the sensor flat and completely still on a flat desk.
2. Load [Day_74_MPU6050_Gyro.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_74_MPU6050_Gyro/Day_74_MPU6050_Gyro.ino) in the Arduino IDE and upload.
3. Open the **Serial Monitor** at **9600 Baud**.
4. The system will display:
   `[SYSTEM] MPU6050 detected. Starting calibration...`
   `[CALIBRATION] Calibrating gyroscope. Keep sensor perfectly still!`
5. After 2 seconds, the calibration offset parameters are shown, and angle printouts begin:
   `RATE -> X: 0.0 d/s	Y: 0.0 d/s	Z: 0.0 d/s | ANGLE -> X: 0.00 deg	Y: 0.00 deg	Z: 0.00 deg`
6. Pick up the sensor and rotate it exactly 90 degrees about the Z-axis (Yaw).
   - Observe Yaw ($Z$ angle) increase dynamically during rotation, and settle close to $90.0^\circ$ when stopped.
7. Return the sensor to its starting position. Yaw should return to $0.0^\circ$.
8. Observe the drift: leave the sensor completely still for 30 seconds. You will see the angles slowly drift by a fraction of a degree. This is normal and shows why we need sensor fusion (which we implement on Day 75!).

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Sensor fails to calibrate / freezes on boot | Sensor was moved during calibration | Keep the module completely still on a stable surface during the first 2 seconds of boot. |
| Calculated angles drift rapidly (e.g. 10 deg/sec) | Calibration failed or offset values corrupted | Reset the Arduino to run the calibration cycle again. |
| Angles jump wildly in raw console | Integration time calculation overflow | Ensure `dt` divides by `1000000.0f` to convert microsecond deltas correctly to seconds. |
| SDA/SCL wire noise causes freeze | Missing I2C pull-up resistors | GY-521 modules usually have 4.7kΩ pull-up resistors built-in. If using long wires, add external 4.7kΩ pull-up resistors between SDA-5V and SCL-5V. |

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
- To find the angle, we must multiply the speed of rotation by the time elapsed ($Distance = Speed 	imes Time$).
- We use `micros()` to find exactly how much time (`dt`) has passed since the last loop.
- We multiply our current rotation speed (`gx`) by `dt` to find how many degrees we moved in that tiny fraction of a second, and add (`+=`) it to our running total. This mathematical process is called Numerical Integration!

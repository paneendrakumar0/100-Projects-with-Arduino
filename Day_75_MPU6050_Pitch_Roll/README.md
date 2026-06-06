# Day 75: MPU6050 Pitch & Roll Sensor Fusion (Complementary Filter)

Welcome to Day 75! Today we master **Sensor Fusion**, one of the most critical concepts in modern robotics and aerospace engineering. Using the **MPU6050 IMU**, we will read both accelerometer and gyroscope data concurrently using an atomic **14-byte burst read**. We will then implement a **Complementary Filter** in software to compute stable, noise-free, and drift-free Pitch and Roll angles.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Kalman_Filter.jpg" alt="Kalman Filter" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/MPU6050.jpg" alt="MPU6050" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

If you try to control a self-balancing robot or stabilizer gimbal using only an accelerometer, it will jitter uncontrollably whenever the motor moves because the sensor cannot distinguish between gravitational acceleration and linear kinetic acceleration.
If you use only a gyroscope, the robot will slowly tip over because the integrated angle drifts over time due to sensor bias.

**Sensor Fusion** solves this by combining both sensors to get the "best of both worlds":
- **Short-term accuracy**: Provided by the gyroscope, which responds instantly to quick movements and ignores linear vibrations.
- **Long-term stability**: Provided by the accelerometer, which always references the true gravitational vector and does not drift.

A **Complementary Filter** is a simple, computationally efficient, and robust way to achieve this on 8-bit microcontrollers.

---

## 🔬 Physics & Hardware Theory

### 1. The Sensor Limits
- **Accelerometer Noise**: Accelerometers measure the direction of gravity. If the sensor is still, this vector is clean. However, if the robot vibrates, the vector oscillates wildly:
  $$\vec{a}_{\text{measured}} = \vec{a}_{\text{gravity}} + \vec{a}_{\text{vibration}}$$
- **Gyroscope Drift**: Gyroscopes measure angular velocity ($\omega$). Integrating this velocity over time yields position. But any small offset (bias) is integrated too, leading to linear drift over time:
  $$\theta_{\text{gyro}}(t) = \theta_{\text{true}}(t) + \text{bias} \cdot t$$

### 2. Complementary Filter Mathematics
The filter combines a **Low-Pass Filter** on the accelerometer and a **High-Pass Filter** on the gyroscope:

$$\theta_{\text{fused}} = \alpha \cdot (\theta_{\text{fused}} + \omega_{\text{gyro}} \cdot dt) + (1 - \alpha) \cdot \theta_{\text{accel}}$$

- **$\alpha$ (Filter Weight)**: Typically set between $0.95$ and $0.98$. If $\alpha = 0.96$:
  - $96\%$ of the angle is calculated by integrating the gyroscope (high-frequency response).
  - $4\%$ of the angle is corrected using the accelerometer gravity vector (low-frequency reference).
- **Time Constant ($\tau$)**: Represents the crossover frequency of the filter:
  $$\tau = \frac{\alpha \cdot dt}{1 - \alpha}$$
  For $\alpha = 0.96$ at $100\,\text{Hz}$ ($dt = 0.01\,\text{s}$), $\tau \approx 0.24\,\text{seconds}$. This means the gyroscope handles movements faster than $0.24$ seconds, and the accelerometer stabilizes anything slower.

```
Gyro Rate ────► [ Gyro Integration ] ──► [ High-Pass Filter ] ──┐
                                                                 ├──(+)──► Fused Angle
Accel Data ───► [ Euler Trigonometry ] ─► [ Low-Pass Filter ] ───┘
```

### 3. Trigonometric Euler Angle Solving
To find the orientation from the accelerometer, we project the gravity vector onto the sensor axes:
- **Roll ($\phi$, rotation around X-axis)**:
  $$\phi_{\text{acc}} = \text{atan2}(a_y, a_z) \cdot \frac{180}{\pi}$$
- **Pitch ($\theta$, rotation around Y-axis)**:
  $$\theta_{\text{acc}} = \text{atan2}(-a_x, \sqrt{a_y^2 + a_z^2}) \cdot \frac{180}{\pi}$$

### 4. 14-Byte Burst I2C Read
The MPU6050 maps all sensor data to 14 consecutive registers starting at `0x3B`:
- `0x3B – 0x40` : Accelerometer (6 bytes)
- `0x41 – 0x42` : Temperature (2 bytes)
- `0x43 – 0x48` : Gyroscope (6 bytes)

Reading all 14 bytes in one single I2C request ensures that the acceleration and angular rate vectors are sampled at the exact same instant, preventing calculation errors.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Master Controller |
| MPU6050 IMU Module (GY-521) | 1 | 6-DOF Sensor |
| Breadboard & Jumper Wires | 1 | Connections |

---

## 🔌 Pin-to-Pin Wiring

| MPU6050 Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** | Power Supply |
| **GND** | **GND** | Ground |
| **SCL** | **A5** | I2C Clock |
| **SDA** | **A4** | I2C Data |
| **AD0** | **GND** | Force I2C address `0x68` |

---

## 💾 Alternatives to Complementary Filters

| Fusion Method | Accuracy | Computational Cost | RAM/Flash Cost | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Complementary Filter** | Good | Extremely Low | Minimal | Best choice for 8-bit microcontrollers (Arduino Uno). |
| **Kalman Filter (Linear)** | Excellent | High | Moderate | Ideal for linear systems. Requires matrix operations. |
| **Extended Kalman (EKF)** | Outstanding | Extremely High | Very High | Standard on flight controllers, too heavy for Atmega328P. |
| **Madgwick Filter** | Outstanding | Moderate | Moderate | Uses quaternions, avoids gimbal lock. Needs high CPU speed. |

---

## 💻 How to Test & Validate

1. Wire the MPU6050 flat on a breadboard.
2. Upload [Day_75_MPU6050_Pitch_Roll.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_75_MPU6050_Pitch_Roll/Day_75_MPU6050_Pitch_Roll.ino).
3. Open the **Serial Monitor** at **9600 Baud**. Keep the board flat and still during the 2-second calibration.
4. You will see three sets of Pitch & Roll outputs side-by-side:
   - `ACCEL ONLY`: Jitters when you tap the table.
   - `GYRO ONLY`: Slowly drifts away even when still.
   - `FUSED`: Completely still, but responds instantly to rotations.
5. Tilt the sensor:
   - Tilt forward/backward: Pitch changes.
   - Tilt left/right: Roll changes.
6. **Vibration Test**: Tap the breadboard rapidly. You will see `ACCEL ONLY` values fluctuate wildly, while `FUSED` values remain steady, demonstrating successful high-frequency filtering.
7. **Drift Test**: Leave the sensor flat on the table for 1 minute. The `GYRO ONLY` angles will drift to several degrees, while the `FUSED` angles remain at exactly $0.0^\circ$, proving the low-frequency accelerometer correction.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Console output is frozen or garbage | Baud rate mismatch | Ensure the Serial Monitor is set to **9600 Baud**. |
| Pitch/Roll values drift indefinitely | Gyro calibration missed or board moved on boot | Press reset button on the Arduino and keep the sensor perfectly still during calibration. |
| Fused angle lags behind actual movements | Alpha constant ($\alpha$) too high or loop rate too slow | Ensure loop runs at 100 Hz (10ms). Reduce $\alpha$ to $0.95$ to give more weight to the accelerometer. |
| Gimbal lock (angles jump wildly at 90°) | Mathematical limit of Euler angles | Pitch/Roll angles suffer from mathematical singularities at $\pm 90^\circ$. For full 3D rotation ($360^\circ$), upgrade to a **quaternion-based filter** (e.g. Madgwick filter). |

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

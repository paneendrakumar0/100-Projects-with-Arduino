# Day 98: Kalman Filter (1D Sensor Fusion & Bias Estimation)

Welcome to Day 98! Today we tackle a legendary algorithm in guidance, navigation, and control: the **Kalman Filter**. We will construct a 1D Kalman filter to perform optimal sensor fusion between an accelerometer and a gyroscope, track and estimate the gyroscope's static drift bias in real-time, and run a 100 Hz simulation comparing the filter's performance with a complementary filter.

---

## 🎯 The "Why" and "What"

Measuring pitch and roll angles of a moving robot is a fundamental challenge:
1. **Accelerometer**: Calculates tilt by measuring the gravity vector relative to the Earth.
   - *Pros*: No long-term drift.
   - *Cons*: Extremely noisy. When the robot accelerates or hits a bump, the lateral force is added to gravity, warping the calculated angle.
2. **Gyroscope**: Measures angular velocity ($\dot{\theta}$). Integrating this velocity gives the angle.
   - *Pros*: Very clean, highly responsive, immune to lateral shocks.
   - *Cons*: Drifts over time. Because we integrate values on every step, even a tiny constant sensor bias (e.g. $+0.05^\circ/\text{s}$) accumulates into degrees of error, eventually drifting to infinity.

### What is a Kalman Filter?
The **Kalman Filter** is an optimal mathematical estimator. It models the physics of the system and the statistical properties of sensor noise (modeled as Gaussian white noise).
Rather than using static weights (like a complementary filter), the Kalman filter **calculates weights dynamically** (the Kalman Gain) based on how confident it is in the prediction (gyroscope integration) versus the measurement (accelerometer). 
Crucially, it includes **Bias Estimation**: by observing how the integrated angle diverges from the accelerometer angle over time, it calculates the gyroscope's offset and subtracts it from future readings!

---

## 🔬 Physics & Mathematics of the Kalman Filter

We model the system using two states: Angle ($\theta$) and Gyroscope Bias ($b$).
Our state vector is:
$$x = \begin{bmatrix} \theta \\ b \end{bmatrix}$$

### Phase 1: Prediction (Time Update)
At each time step $\Delta t$, we use the gyroscope angular velocity reading $\dot{\theta}$ to project the state forward:
$$\theta_k = \theta_{k-1} + \Delta t \cdot (\dot{\theta}_k - b_{k-1})$$
$$b_k = b_{k-1}$$

Next, we project the state error covariance matrix $P$ (which represents the filter's uncertainty in its estimates):
$$P_{00} = P_{00} - \Delta t \cdot (P_{01} + P_{10}) + Q_{\theta} \cdot \Delta t$$
$$P_{01} = P_{01} - \Delta t \cdot P_{11}$$
$$P_{10} = P_{10} - \Delta t \cdot P_{11}$$
$$P_{11} = P_{11} + Q_{\text{bias}} \cdot \Delta t$$
where $Q_{\theta}$ and $Q_{\text{bias}}$ are our process noise covariance parameters (tuning values).

### Phase 2: Update (Measurement Update)
When a new accelerometer angle measurement ($z$) arrives:
1. **Calculate the measurement innovation (residual)**:
   $$y = z - \theta_k$$
2. **Calculate the innovation covariance ($S$)**:
   $$S = P_{00} + R_{\text{measure}}$$
   where $R_{\text{measure}}$ is the accelerometer measurement noise covariance.
3. **Calculate the Kalman Gain ($K$)**:
   $$K_0 = \frac{P_{00}}{S}$$
   $$K_1 = \frac{P_{10}}{S}$$
4. **Update the state estimates**:
   $$\theta_k = \theta_k + K_0 \cdot y$$
   $$b_k = b_k + K_1 \cdot y$$
5. **Update the error covariance matrix $P$**:
   $$P_{00} = P_{00} - K_0 \cdot P_{00}$$
   $$P_{01} = P_{01} - K_0 \cdot P_{01}$$
   $$P_{10} = P_{10} - K_1 \cdot P_{00}$$
   $$P_{11} = P_{11} - K_1 \cdot P_{01}$$

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| MPU6050 6-Axis IMU | 1 | Optional. To test on real physical tilts. |
| Breadboard & Jumper Wires | 1 | Connections |

---

## 🔌 Pin-to-Pin Wiring

If testing on a physical MPU6050 IMU:
| MPU6050 Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** | Power supply |
| **GND** | **GND** | Common ground |
| **SDA** | **A4** | I2C Data line |
| **SCL** | **A5** | I2C Clock line |

*If no MPU6050 is available, the code runs in Simulation Mode, generating simulated IMU signals over the Serial line.*

---

## 💾 Alternatives to 1D Kalman Filters

| Filter Type | CPU Overhead | Memory Overhead | Tracking Accuracy | Shock Rejection | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1D Kalman Filter** | **Medium** | **O(1) - Low** | **Excellent** | **Very High** | Estimates gyro bias dynamically. Optimal for single-axis balancing. |
| **Complementary Filter** | Low | Low | Good | Low | Simple weighted average. Prone to passing sudden shock spikes. |
| **Low-Pass Filter** | Very Low | Low | Poor | High | Introduces severe phase lag, making it useless for fast balancing loops. |
| **Madgwick Filter** | High | Medium | Excellent (3D) | High | Quaternion-based 3D orientation estimator. Heavy math overhead. |

---

## 💻 How to Test & Validate

1. Open the Arduino IDE, load [Day_98_Kalman_Filter.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_98_Kalman_Filter/Day_98_Kalman_Filter.ino), and select the COM port.
2. Select your Arduino Uno board and upload the code.
3. Open **Tools > Serial Plotter** at **115200 Baud**. (Ensure you change the plotter speed to 115200; the loop runs at 100 Hz!).
4. **Analyzing the Plots**:
   - You will see five lines:
     - `TrueAngle` (Blue): The actual sinusoidal movement of the robot.
     - `RawAccel` (Orange): The noisy accelerometer readings.
     - `CompFilter` (Green): The complementary filter output.
     - `Kalman` (Red): The Kalman filter output.
     - `EstBias` (Purple): The Kalman filter's estimate of gyroscope bias.
   - Observe how the **Kalman** line tracks the **TrueAngle** much closer than the `CompFilter` and smoothly rejects the high-frequency accelerometer noise.
5. **Shock Rejection Test**:
   - Close the plotter and open the **Serial Monitor** at **115200 Baud**.
   - Send `b`. This injects a sudden $+30^\circ$ shock spike into the accelerometer measurement.
   - Observe the outputs:
     - The raw accelerometer jumps.
     - The complementary filter jumps by several degrees due to its fixed weighting.
     - The **Kalman Filter** barely twitches, recognizing the bump as high-frequency noise and filtering it out!
6. **Gyro Bias Tracking Test**:
   - Send `z`. This toggles the gyroscope constant drift bias from $+2^\circ/\text{sec}$ to $+0^\circ/\text{sec}$.
   - Observe `EstBias` dynamically decay back down to $0.0$, demonstrating that the filter constantly tracks and eliminates sensor bias!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Telemetry output displays garbage characters | Baud rate mismatch | Ensure the Serial Monitor / Serial Plotter baud rate is set strictly to **115200**. |
| Kalman filter output lags behind the real movement | Process noise $Q_{\text{angle}}$ is too small | Increase $Q_{\text{angle}}$ (using `3` in the CLI) or decrease $R_{\text{measure}}$ (using `2` in the CLI) to tell the filter to trust measurements more. |
| Kalman filter output is too noisy | Measurement noise $R_{\text{measure}}$ is too small | Increase $R_{\text{measure}}$ (using `1` in the CLI) to tell the filter to ignore accelerometer fluctuations. |
| Memory usage warnings on compile | float multiplication overhead | The math has been optimized to bypass matrix multiplication libraries by calculating the algebraic updates directly. It runs under 100 microseconds, well within the 10,000 microsecond limit of 100 Hz. |

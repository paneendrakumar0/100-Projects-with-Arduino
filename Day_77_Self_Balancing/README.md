# Day 77: Self-Balancing Robot Logic (Stabilization Loop & PID Tuning)

Welcome to Day 77! Today we step into control systems engineering by designing the stabilization controller for a **Self-Balancing Robot**. We combine our MPU6050 Complementary Filter tilt estimator with a high-speed **Proportional-Integral-Derivative (PID) Controller**, implement **anti-windup clamping**, map outputs to an **L298N H-Bridge driver**, and write a real-time Serial Tuning CLI to adjust PID gains dynamically.

---

## 🎯 The "Why" and "What"

Two-wheeled self-balancing robots are classic examples of the **Inverted Pendulum** problem, a foundational benchmark in control theory.
- **Inverted Pendulum**: Unlike a regular pendulum (where the center of mass hangs below the pivot and is stable), the center of mass of a self-balancing robot sits *above* the wheels and is inherently unstable. If left alone, it will fall.
- **Active Balancing**: To keep the robot upright, the wheels must continuously accelerate in the *direction* the robot is falling. By matching the tilt velocity, the wheel pivot remains positioned directly underneath the robot's center of mass, maintaining balance.

This loop must run at high frequency (100 Hz) to counteract physical gravity before mechanical inertia takes over.

---

## 🔬 Physics & Hardware Theory

### 1. Inverted Pendulum Dynamics & Restoring Force
Consider a robot tilted at an angle $\theta$ relative to the vertical axis:
- The force of gravity ($F_g = m \cdot g$) acts on the center of mass at a distance $L$ from the wheels, creating a destabilizing torque:
  $$\tau_{\text{gravity}} = m \cdot g \cdot L \cdot \sin(\theta)$$
- To counteract this, the motors must accelerate the wheels to create a horizontal inertial restoring force ($F_{\text{restore}}$), establishing a balancing torque:
  $$\tau_{\text{balancing}} = F_{\text{restore}} \cdot L \cdot \cos(\theta)$$
- The controller adjusts motor speed to make $\tau_{\text{balancing}} > \tau_{\text{gravity}}$, driving $\theta$ back to $0^\circ$.

```
           Center of Mass (m)
                 ●
                / \
               /   \
              /     \  Tilt Angle (θ)
             /       \  
            /         \  
           /     L     \ 
          /             \
         o               o
     [Wheel]           [Wheel]
      ◄──► Drive direction to stabilize
```

### 2. The PID Control Terms
The PID controller uses the error signal $e(t) = \theta_{\text{target}} - \theta_{\text{current}}$:
1. **Proportional ($K_p \cdot e(t)$)**: Generates a restoring force proportional to how far the robot has tilted. A higher $K_p$ makes the robot react faster but can cause it to oscillate back and forth.
2. **Integral ($K_i \cdot \int e(t) dt$)**: Accumulates the tilt error over time. This is essential to overcome mechanical friction, backlash in gears, or unequal wheel weight, pushing the robot exactly to $0.0^\circ$.
   - **Anti-Windup**: If the robot is held or blocked, the error continues to accumulate, causing the integral term to grow to infinity (windup). When released, the robot will drive wildly and crash. We restrict the integral accumulator using `constrain()` to prevent windup.
3. **Derivative ($K_d \cdot \frac{de(t)}{dt}$)**: Responds to the *speed* of tilting. It acts as a mechanical dampener, opposing rapid movements to prevent the overshoot caused by the $K_p$ term.

### 3. Safety Drop Lock
If the robot tilts beyond a threshold (e.g. $\pm 40^\circ$), it has fallen past the point of recovery. Allowing the motors to continue spinning at maximum speed will burn out the H-Bridge or damage the gearboxes. We implement a **safety drop lock** that immediately cuts motor PWM to $0$ if the tilt exceeds $40^\circ$.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Control Loop Processor |
| MPU6050 IMU | 1 | Tilt angle sensor |
| L298N Dual H-Bridge Driver | 1 | Drives DC motors |
| High-Torque Geared DC Motors | 2 | Balancing actuators (wheels attached) |
| 12V LiPo Battery | 1 | Motor power source |
| Breadboard & Wires | 1 | Circuit assembly |

---

## 🔌 Pin-to-Pin Wiring

| L298N Pin | MPU6050 Pin | Arduino Pin | Description |
| :--- | :--- | :--- | :--- |
| **GND** | **GND** | **GND** | Common Ground (Battery GND must also connect here!) |
| **5V** | **VCC** | **5V** | Logic Power |
| **ENA** | - | **D5** | Left Motor Speed (PWM) |
| **IN1** | - | **D3** | Left Motor Dir 1 |
| **IN2** | - | **D4** | Left Motor Dir 2 |
| **IN3** | - | **D7** | Right Motor Dir 1 |
| **IN4** | - | **D8** | Right Motor Dir 2 |
| **ENB** | - | **D6** | Right Motor Speed (PWM) |
| - | **SDA** | **A4** | I2C SDA |
| - | **SCL** | **A5** | I2C SCL |

---

## 💾 Alternatives to PID Control

| Method | Response Time | Calibration Complexity | CPU Load | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **PID (Our Choice)** | Fast | Moderate (heuristic tuning) | Very Low | Best suited for Arduino Uno. Very easy to implement. |
| **LQR (Linear Quadratic Regulator)** | Excellent | High (needs physical system model) | Moderate | Optimal control; requires state matrices. |
| **State-Space Control** | Outstanding | Very High (requires full math models) | Moderate | Great for multi-input multi-output (MIMO) systems. |
| **Fuzzy Logic** | Good | High (rule base design) | Moderate | Relies on linguistic rules instead of math equations. |

---

## 💻 How to Test & Validate

1. Connect the MPU6050 and the L298N driver to the Arduino Uno. **Securely mount the MPU6050 to the robot frame.**
2. Upload [Day_77_Self_Balancing.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_77_Self_Balancing/Day_77_Self_Balancing.ino).
3. Open the **Serial Monitor** at **9600 Baud**.
4. **Interactive Simulation Mode (No Hardware Needed)**:
   - Type `m` to toggle Simulation Mode.
   - Type `e 1` to release the motor safety lock.
   - Inject a tilt angle: Type `s 5.0` (sensor tilted 5 degrees).
   - Observe the telemetry outputs:
     `[TELEMETRY] Mode: SIM | Active: YES | Pitch: 5.00 deg | PID: [22.5, 1.8, 1.2] | Effort: -113.4`
     The motors spin backward to counter the fall.
   - Inject opposite tilt: Type `s -10.0`. The motors swing forward to counter.
   - Type `s 45.0` (fall past recovery limits). The safety drop lock fires and cuts motors instantly.
5. **Real-world Tuning CLI**:
   - In normal hardware mode, release the safety lock by typing `e 1`.
   - Hold the robot vertical.
   - Tune gains on the fly: Type `p 25.0` to set Kp, `d 1.5` to set Kd, and `i 2.0` to set Ki.
   - **Tuning Rule of Thumb**:
     1. Increase $K_p$ until the robot starts oscillating back and forth.
     2. Increase $K_d$ to damp out the oscillations.
     3. Add a small $K_i$ to push the robot to a stable hover.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Motors run at full speed away from direction of tilt | Reversed feedback sign | Swap motor wire polarities on BOTH motors, or flip the signs of IN1/IN2 and IN3/IN4 in code. |
| Robot shakes violently (buzzing) | $K_p$ or $K_d$ gains set too high | Lower $K_p$ and $K_d$ to reduce controller over-reaction. |
| Robot falls over slowly without recovering | $K_p$ gain too low | Increase $K_p$ to generate more restoring force. |
| Motors do not rotate | Battery disconnected or safety lock active | Ensure the motor power battery is connected to L298N. Release lock via Serial (`e 1`). |
| System freezes during I2C read | SCL/SDA wiring loose | Verify I2C connections and add decoupling capacitors to the motor power line to prevent EMF noise resetting the IMU. |

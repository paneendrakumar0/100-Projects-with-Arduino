# Day 37: Closed-Loop PID Motor Speed Control (Feedback Controller)

Welcome to Day 37 of the 100-Day Arduino Masterclass! Today, we combine our motor driver (Day 33) and encoder sensor feedback (Day 36) to build a **Closed-Loop Proportional-Integral-Derivative (PID) controller**. We will write our own discrete PID regulator from scratch to adjust PWM outputs dynamically, keeping a DC motor spinning at a precise target speed (RPM) even when mechanical load is added or voltage changes.

---

## 🎯 The "Why" and "What"

In robotics and automation, motors must maintain constant speeds under varying loads:
1. **Mobile Robots:** A robot must drive straight, which requires left and right wheels to rotate at the exact same RPM regardless of changes in carpet friction, slope, or battery level.
2. **Conveyor Belts:** Must transport loads at a constant speed, whether carrying a heavy box or running completely empty.
3. **Robotic Arms:** Joint actuators must lock onto precise trajectories under variable gravitational forces.

An **open-loop controller** (where we write static PWM outputs) cannot correct for external disturbances. A **closed-loop PID controller** constantly monitors system speed, calculates the offset error from the target setpoint, and adjusts the power automatically to keep the motor running on target.

---

## 🔬 Physics & Control Loop Theory

A PID controller is a feedback mechanism that calculates an **Error** $e(t)$ as the difference between a desired **Setpoint** (target RPM) and a measured **Process Variable** (actual measured RPM):
$$e(t) = \text{Setpoint} - \text{Measurement}(t)$$

### 1. PID Mathematical Block Diagram
The controller adjusts the control signal $u(t)$ (our PWM output) by summing three separate terms:

```
               ┌──────────────┐
               │ Proportional │ (Kp * e)
           ┌──>│     [Kp]     ├──────────┐
           │   └──────────────┘          │
           │   ┌──────────────┐          ▼
Setpoint ──┼──>│   Integral   │ (Ki * ∫e)  Sum  u(t)
   (+)     │   │  [Ki] + Anti-├─────────>(+)───► [Actuator] (Motor)
    │      │   │    Windup    │          ▲
    ▼      │   └──────────────┘          │
  (Error) ─┤   ┌──────────────┐          │
  (e = S-M)└──>│  Derivative  │ (Kd * de/├─┘
   (-)     │   │     [Kd]     │    dt)   │
    ▲      │   └──────────────┘          │
    │      │                             │
    └──────┴───────── Measurement ◄──────┴ Sensor (Encoder)
```

---

### 2. The PID Terms Explained
* **Proportional (P):** $K_p \cdot e(t)$. Generates an output proportional to the current error. If the motor is far below target speed, P drives the motor hard. If speed is close to target, P pushes gently.
  * *Tuning Effect:* Increasing $K_p$ speeds up rise time, but setting it too high causes the motor to overshoot the setpoint and oscillate wildly.
* **Integral (I):** $K_i \cdot \int e(t) dt$. Integrates (accumulates) error over time. If a constant error remains (steady-state error) due to friction, the integral sum will grow larger and larger, adding extra voltage to force the motor to the target.
  * *Tuning Effect:* Eliminates steady-state error, but can increase overshoot and system instability.
* **Derivative (D):** $K_d \cdot \frac{de(t)}{dt}$. Measures the rate of change (slope) of the error. It predicts future behavior, applying a "brake" to the controller to slow it down as the actual speed approaches the setpoint.
  * *Tuning Effect:* Dampens oscillations and reduces overshoot.

---

### 3. Integral Saturation (Anti-Windup)
If the motor is physically blocked (stalled), the speed remains at 0, and the error remains high. The integral term will continue summing the error, growing towards infinity. When the motor is released, this massive accumulated sum will drive the motor at maximum voltage for a long time, causing it to overshoot the target setpoint dangerously.
To prevent this, we implement **Integral Anti-Windup** by clamping the integral accumulator to a maximum threshold:
```cpp
integralAccumulator += error * dt;
integralAccumulator = constrain(integralAccumulator, -integralMaxLimit, integralMaxLimit);
```

---

## 🔄 Alternatives Comparison

When choosing control architectures for mechatronic feedback loops:

| Controller Type | Output Type | Stability | Rise Time | Steady-State Error | Complexity | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **PID Control** | **Analog / PWM** | **High (If tuned)** | **Fast** | **Zero** | **Medium** | **Motor speed regulation, temperature chambers, flight dynamics (Our choice)** |
| **Bang-Bang (On-Off)**| **Digital (On/Off)**| **Low (Constant oscillation)**| **Fast** | **High** | **Very Low** | **Home thermostats, simple fluid level systems** |
| **Feedforward** | **Calculated Estimate**| **High** | **Instant** | **High (No correction)** | **High** | **Predictive robotics, path planning combined with feedback** |
| **Fuzzy Logic** | **Rule-Based (IF/THEN)**| **High** | **Medium** | **Low** | **High** | **Automotive cruise control, complex non-linear thermal systems** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x Brushed DC Motor (with slot disk or magnetic encoder attached)
* 1x Optical Slot Encoder Module (LM393)
* 1x L298N Dual H-Bridge Driver
* 1x External Battery Pack for motor power ($6\text{V}-12\text{V}$)
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

| Component Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **Encoder VCC** | **5V** | Red | Sensor logic power |
| **Encoder GND** | **GND** | Black | Sensor ground |
| **Encoder OUT** | **D2** (Interrupt 0) | Green | Sensor speed pulse output |
| **L298N ENA** | **D9** (PWM pin) | Orange | Motor speed PWM output |
| **L298N IN1** | **D7** | Yellow | Motor direction forward control |
| **L298N IN2** | **D8** | Brown | Motor direction reverse control |
| **L298N GND** | **GND** | Black | Shared Ground (Mandatory!) |
| **L298N 12V** | **Battery (+)** | Red | External battery positive |
| **Battery (-)** | **GND** | Black | Shared Ground |

---

## 💻 How to Test & Validate

1. Wire up the system according to the pin connections table. Attach the external battery to the L298N driver.
2. Upload `Day_37_PID_Speed_Control.ino` to the Arduino.
3. **Open the Serial Plotter:**
   * Go to **Tools -> Serial Plotter** (or press `Ctrl+Shift+L` in Arduino IDE).
   * Ensure the baud rate is set to **9600 Baud**.
4. Observe the plot in real-time:
   * **Blue Line:** The Setpoint RPM (target speed). It will step: `0 -> 100 -> 220 -> 140 -> 0`.
   * **Red Line:** The Actual RPM measured from the encoder.
   * **Green Line:** The PWM control value written to the motor driver.
5. **Interactive Test (Disturbance Rejection):**
   * While the setpoint is steady at `140 RPM` or `220 RPM`, carefully pinch the spinning motor axle with your fingers to add friction (simulating mechanical load).
   * Watch the Plotter:
     * The actual speed (Red) will drop briefly.
     * The PID controller detects this error and immediately ramps up the PWM output (Green) to feed more voltage to the motor.
     * The actual speed (Red) rises back up, locking onto the Setpoint line even though you are applying braking friction!
   * Release the axle: The actual speed will rise briefly, the PID will drop the PWM, and the speed will quickly settle back onto the Setpoint.

---

## 🛠️ Tuning Guide (Trial & Error Heuristics)

If your motor speed oscillates wildly or fails to reach the setpoint, you must tune the gains ($K_p$, $K_i$, $K_d$):
1. **Set $K_i = 0$ and $K_d = 0$.**
2. Increase $K_p$ slowly (in increments of 0.1) from 0 until the motor reaches the setpoint quickly but starts to oscillate continuously back and forth around it.
3. Cut $K_p$ in half to stabilize the system.
4. Increase $K_i$ slowly (increments of 0.2) to eliminate the steady-state offset (so the speed reaches the target, rather than stalling slightly below it).
5. If the system overshoots during step changes, increase $K_d$ slowly (increments of 0.01) to add damping.

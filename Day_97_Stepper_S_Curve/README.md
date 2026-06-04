# Day 97: Stepper Motor S-Curve Speed Profile (Jerk Minimization & LUT Ramping)

Welcome to Day 97! Today we explore advanced **Motion Control** by designing and implementing a smooth **S-Curve (Raised Cosine) Acceleration Profile Generator** for stepper motors. We will study the physics of motor stalling, analyze the mathematical definition of **jerk**, precompute step intervals, and build a non-blocking step engine testable via the Serial Plotter.

---

## 🎯 The "Why" and "What"

In high-precision mechatronic systems (like 3D printers, CNC routers, and robot joints), speed is important, but **smoothness** is critical. 

### The Problem with Linear Ramps (Trapezoidal)
Most standard stepper libraries (like the basic `AccelStepper`) use a linear velocity ramp. While simple, linear ramping has a major flaw: at the boundary points (where acceleration starts or stops), there is an instantaneous jump in acceleration. 
The mathematical derivative of acceleration is **jerk**:
$$\text{Jerk} = \frac{da}{dt}$$

An instantaneous change in acceleration represents **infinite jerk**.
In the physical world, infinite jerk causes:
1. High mechanical vibration and resonance.
2. Audible noise and clunking.
3. **Motor Stall**: Stepper motor torque decreases rapidly with speed. If you jerk the motor during high speeds, it will slip poles, lose synchronization, and lock up (stall).

### The S-Curve Solution
An **S-Curve acceleration profile** smooths out the acceleration curve by gradually ramping it up at the start and ramping it down at the end. This bounds the jerk to a finite, safe value, producing an "S-shape" velocity curve. This allows the motor to reach much higher peak speeds without stalling!

```
Linear (Trapezoidal) Velocity:          S-Curve (Smooth) Velocity:
      Velocity                                Velocity
         ▲                                       ▲
  vMax ──┼───────/──────────\─            vMax ──┼───────.──────────.─
         │      /            \                   │     .              .
         │     /              \                  │    .                .
  vStart ┼────/                \─         vStart ┼───.                  .─
         └────────────────────────► Time         └────────────────────────► Time
        (Sharp corners = Infinite Jerk)         (Rounded corners = Finite Jerk)
```

---

## 🔬 Physics & Mathematics of S-Curves

### 1. The Raised Cosine Model
To compute the step delays in real-time on an 8-bit AVR microcontroller without slowing down the stepper pulses, we precompute a 100-step ramp lookup table (LUT) in memory.
We define the velocity $v(i)$ at step index $i$ ($0 \le i \le N-1$) using a raised cosine:
$$v(i) = v_{\text{start}} + (v_{\text{max}} - v_{\text{start}}) \cdot \left( \frac{1 - \cos\left(\pi \cdot \frac{i}{N-1}\right)}{2} \right)$$

Where:
- $v_{\text{start}}$: Starting frequency (e.g. $150\,\text{steps/sec}$).
- $v_{\text{max}}$: Peak frequency (e.g. $1200\,\text{steps/sec}$).
- $N$: Number of steps in the ramp ($100$).
- $i$: The current step index in the ramp.

### 2. Step Interval Math
To pulse the stepper motor, the microcontroller waits for a delay interval ($t_{\text{delay}}$) between steps. The delay in microseconds for step $i$ is:
$$t_{\text{delay}}(i) = \frac{1,000,000}{v(i)}\,\mu\text{s}$$

By calculating this array in `setup()`, the execution loop only needs to read a single integer from memory per step, keeping execution extremely fast and jitter-free!

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| Stepper Driver (A4988 / DRV8825) | 1 | Controls motor phases based on STEP/DIR pins |
| Bipolar Stepper Motor (e.g., NEMA 17)| 1 | High-precision position actuator |
| External Power Supply (12V/2A DC) | 1 | Motor power (Uno cannot drive motor coils) |
| Decoupling Capacitor (100 µF) | 1 | Protects driver from voltage spikes |
| Breadboard & Jumper Wires | 1 | Connections |

---

## 🔌 Pin-to-Pin Wiring

| Stepper Driver Pin | Connect To | Description |
| :--- | :--- | :--- |
| **STEP** | **Digital Pin 9** | Pulsing pin (Step trigger) |
| **DIR** | **Digital Pin 8** | Direction pin (HIGH = CW, LOW = CCW) |
| **GND** (Logic) | **Arduino GND** | Common Ground |
| **VDD** | **Arduino 5V** | Logic Power |
| **VMOT** | **Power Supply (+12V)** | Motor coil power |
| **GND** (Power) | **Power Supply GND** | Motor power ground |
| **1A, 1B, 2A, 2B** | **Stepper Motor Coils** | Phase outputs |

---

## 💾 Alternatives to Custom S-Curve Profiling

| Method | Speed Curve | Jerk Level | CPU Overhead | Max Speed | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **S-Curve LUT (Our project)** | **S-Shape** | **Low (Finite)** | **Low** | **High** | Precomputed in RAM. Best balance of speed and smoothness. |
| **Linear (Trapezoidal)** | Linear | Infinite | Very Low | Moderate | Standard implementation, high vibration. |
| **Trigonometric Real-time** | S-Shape | Low | Very High | Low | Calculates `cos()` on every step. CPU struggles above 500 steps/sec. |
| **External Motion IC** | Custom | Zero | Zero | Extremely High | Dedicated chips (e.g. TMC5160) handle ramps in hardware. |

---

## 💻 How to Test & Validate

1. Connect your stepper motor driver to pins 8 and 9 (or run the simulator with nothing connected to view telemetry).
2. Open the Arduino IDE, load [Day_97_Stepper_S_Curve.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_97_Stepper_S_Curve/Day_97_Stepper_S_Curve.ino), and upload it.
3. Open **Tools > Serial Plotter** at **9600 Baud**.
4. **Compare Ramping Telemetry**:
   - Send `1` to select **Linear Ramping**.
   - Send `g 500` to execute a 500-step move.
   - Observe the speed plot. You will see a sharp triangular wedge with hard points at the base and apex.
   - Send `2` to select **S-Curve Ramping**.
   - Send `g 500`.
   - Observe the speed plot. You will see a smooth, rounded bell-like shape (raised cosine).
5. **Speed Boundary Adjustments**:
   - Open the **Serial Monitor** at **9600 Baud**.
   - Send `u` to increase peak speed by 100 steps/sec.
   - Send `d` to decrease peak speed by 100 steps/sec.
   - Send `g 1000` to perform a longer move. The flat top segment represents the peak velocity phase.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Motor makes a high-pitched whine but does not rotate | Motor is stalling or current limit is too low | Increase the current limit potentiometer on the A4988 driver. If speed is too high, decrease `vMax` using `d` in the CLI. |
| Motor stutters at the very beginning of the motion | Starting velocity is too high | Decrease `vStart` in the code to `50.0f` or `100.0f` to allow the motor to begin turning from a lower torque threshold. |
| Motor rotates in the wrong direction | Coil phases wired backwards | Power down the system, and swap the wires of one coil (e.g. swap A1 and A2) or toggle `DIR_PIN` logic in the sketch. |
| Serial Plotter output is garbled | Baud rate mismatch | Ensure the Serial Monitor / Serial Plotter baud rate is set to **9600**. |

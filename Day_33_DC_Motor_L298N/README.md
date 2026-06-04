# Day 33: DC Motor Speed & Direction Control (L298N H-Bridge)

Welcome to Day 33 of the 100-Day Arduino Masterclass! Today, we transition to high-power actuators. We will interface a brushed DC motor using the industry-standard **L298N Dual H-Bridge Driver module**. We will implement a non-blocking speed ramp state machine and explore the physics of H-bridges, Pulse Width Modulation (PWM), and inductive back-EMF suppression.

---

## 🎯 The "Why" and "What"

Brushed DC motors are the workhorses of mobile robotics, driving wheels, tracks, and mechanical pulleys.
* **The Problem:** DC motors draw high currents ($100\text{ mA}$ to several Amps) and require bidirectional voltage to change direction. Microcontrollers pins can output at most $40\text{ mA}$ at $5\text{V}$, and reversing voltage polarity directly from digital pins is electronically impossible.
* **The Solution:** The **L298N H-Bridge Driver** acts as a high-power routing switch. It takes low-power control signals from the Arduino and routes high-current power from an external battery pack to the motor.

In this project, we write a non-blocking program that accelerates, cruises, decelerates, and electronically brakes a DC motor in both forward and reverse directions.

---

## 🔬 Physics & Hardware Theory

### 1. The H-Bridge Topography
To reverse a DC motor, we must swap the positive and negative terminals of the power supply. An H-Bridge achieves this using four electronic switches (transistors) arranged in an "H" layout:

```
          Power (+V)
         ┌────┴────┐
      [S1]       [S2]
         ├─ Motor ─┤
      [S3]       [S4]
         └────┬────┘
           GND (-)
```

* **Forward Rotation:** Close switches **S1** and **S4** (S2 and S3 open). Current flows left-to-right through the motor.
* **Reverse Rotation:** Close switches **S2** and **S3** (S1 and S4 open). Current flows right-to-left through the motor.
* **Active Braking:** Close **S3** and **S4** (or S1 and S2). This short-circuits the motor terminals together, converting the motor's kinetic energy into electrical current that opposes rotation, stopping it instantly.
* **Coast (Float):** Open all switches. The motor spin decays naturally due to friction.

> [!CAUTION]
> If **S1** and **S3** (or S2 and S4) are closed at the same time, it creates a direct short circuit between Power and Ground. This is called **Shoot-Through** and will immediately destroy the H-bridge transistors. The L298N internal logic gates protect against this.

---

### 2. Inductive Spikes & Flyback Diodes (Back-EMF)
A DC motor consists of coils of copper wire (inductors) rotating inside a magnetic field. Inductors store energy in a magnetic field. 
When the H-bridge switches open to stop the motor, the current cannot instantly drop to zero. The collapsing magnetic field forces current to continue flowing, creating a massive voltage spike:
$$V = L \cdot \frac{di}{dt}$$

These spikes can easily exceed $100\text{ V}$ and punch through the sensitive silicon junctions of the transistors. The L298N module contains **Flyback Diodes** (also called freewheeling diodes) connected in parallel with the transistors to route these high-voltage spikes safely back into the power rails.

---

### 3. PWM Speed Regulation
Instead of supplying a constant voltage (which would keep the motor running at full speed), we pulse the voltage at high frequency ($490\text{ Hz}$ on standard Arduino pins) using **Pulse Width Modulation (PWM)**. 
The average voltage delivered is proportional to the **Duty Cycle** (the ratio of ON time to total cycle time):
$$V_{\text{average}} = V_{\text{source}} \times \left( \frac{t_{\text{ON}}}{t_{\text{ON}} + t_{\text{OFF}}} \right)$$

By altering the duty cycle via `analogWrite(pin, value)` (from 0 to 255), we regulate the average current and magnetic field strength, controlling the motor speed.

---

## 🔄 Alternatives Comparison

When selecting motor drivers for mobile robots:

| Driver IC / Module | Transistor Type | Continuous Current | Peak Current | Efficiency / Heat | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **L298N H-Bridge** | **BJT (Bipolar Junction)**| **$2.0\text{ A}$** | **$3.0\text{ A}$** | **Low (Drains up to $2\text{V}$ internally; runs hot)** | **Low-cost hobby robots, medium DC motors (Our choice)** |
| **TB6612FNG** | **MOSFET** | **$1.2\text{ A}$** | **$3.2\text{ A}$** | **High (Minimal internal resistance; runs cool)** | **Small, high-efficiency micro-metal gearmotor robots** |
| **L293D** | **BJT** | **$600\text{ mA}$** | **$1.2\text{ A}$** | **Low** | **Tiny, low-current DC motors or solenoid valves** |
| **BTS7960** | **High-power MOSFET** | **$43\text{ A}$** | **$60\text{ A}$** | **Extremely High** | **Heavy-duty industrial motors, combat robots** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x L298N Dual H-Bridge Module
* 1x Brushed DC Motor (e.g., standard yellow hobby gearmotor, 3V-12V)
* 1x External DC Power Source (e.g. 2s LiPo battery, 6x AA pack, or DC Bench Power Supply)
* 1x Breadboard
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

> [!IMPORTANT]
> **Common Ground Rule:** You MUST connect the negative (-) terminal of your external motor battery to the Arduino's **GND** pin. Without this shared ground reference, the control signals from the Arduino will float erratically, and the motor driver will not function.

```
                  +--------------------------+
                  |       L298N Module       |
                  |                          |
    Arduino Pin 9 |[ENA]             [OUT1]  |====== Motor Terminal 1
    Arduino Pin 7 |[IN1]             [OUT2]  |====== Motor Terminal 2
    Arduino Pin 8 |[IN2]                     |
                  |                  [12V]   |====== Battery Positive (+)
    Arduino GND   |[GND]             [GND]   |====== Battery Negative (-)
                  +--------------------------+
```

| L298N Driver Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **ENA** | **D9** (PWM pin) | Green | Speed Control (PWM) |
| **IN1** | **D7** | Orange | Direction Control Input 1 |
| **IN2** | **D8** | Yellow | Direction Control Input 2 |
| **GND** | **GND** | Black | Common Ground connection |
| **12V Screw Terminal** | **Battery (+) Pos** | Red | External power input ($6\text{V} - 12\text{V}$) |
| **GND Screw Terminal** | **Battery (-) Neg** | Black | Power return ground |
| **OUT1 Terminal** | **Motor Lead 1** | Blue | Motor power output A |
| **OUT2 Terminal** | **Motor Lead 2** | Blue | Motor power output B |

---

## 💻 How to Test & Validate

1. Wire the Arduino, L298N module, DC motor, and external battery pack as shown in the wiring table. Keep the external battery switched off or unplugged until verification.
2. **Double check:** Verify that the onboard 5V regulator jumper on the L298N module is in place (this enables the module to generate its own logic power from the motor power input).
3. Upload `Day_33_DC_Motor_L298N.ino` to the Arduino.
4. Open the **Serial Monitor** at **9600 Baud**.
5. Connect/Switch ON the external battery power.
6. Observe the motor:
   * The motor will accelerate forward, cruise for 4 seconds, decelerate to a stop, and brake.
   * After a brief pause, it will duplicate this cycle in the reverse direction.
7. Inspect the Serial Monitor:
   * Telemetry prints will update at 2 Hz showing current action (e.g. `[DRIVE] Action: ACCEL FORWARD | Target Speed: 45% | Pins: IN1=1 IN2=0`).
   * When transition states occur, a header message triggers (e.g., `[STATE CHANGE] -> Transitioned to state: REVERSE ACCELERATION`).

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The motor hums or vibrates but does not spin at low speed percentages:**
  * Gearmotors have internal static friction (stiction). At low PWM values (e.g. $< 15\%$), the average voltage is insufficient to overcome this stiction. This is normal. You can increase the minimum start speed or use a higher voltage supply.
* **The motor spins in the opposite direction of the Serial Monitor's printed action:**
  * Swap the motor connections at the OUT1 and OUT2 screw terminals. This reverses the physical installation polarity.
* **The motor does not spin at all, and the Arduino resets when the cycle starts:**
  * You are trying to power the motor directly from the Arduino's 5V rail. Unplug it immediately and connect an external battery pack to the L298N power terminals.
* **The L298N power LED is ON, but the motor does not spin and nothing happens:**
  * Make sure you have connected the negative (-) terminal of the external battery to the Arduino's **GND** pin.
  * Check if the ENA jumper on the L298N board was removed; you must plug your jumper wire from Pin 9 directly into the pin marked "ENA" (remove the default black plastic shunt).

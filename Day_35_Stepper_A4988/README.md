# Day 35: High-Performance Bipolar Stepper Control (A4988 & Trapezoidal Ramping)

Welcome to Day 35 of the 100-Day Arduino Masterclass! Today, we transition to high-precision motion control by interfacing a **NEMA 17 Bipolar Stepper Motor** with the **A4988 microstepping driver**. We will write a custom step generator implementing **Trapezoidal Velocity Profiles** (linear acceleration and deceleration ramps) using microsecond-level non-blocking timers. We will also learn how to calibrate the driver's current-limit potentiometer ($V_{\text{ref}}$).

---

## 🎯 The "Why" and "What"

Bipolar stepper motors (like the NEMA 17) are the industry standard for high-performance positioning systems, including 3D printers, CNC routers, robotic joints, and linear actuators.
* **The Problem:** Stepper motors have significant rotor inertia. If you try to start a motor at maximum speed instantly, the magnetic poles of the stator will rotate faster than the rotor can follow, causing the motor to lock up, vibrate, and stall (slip steps).
* **The Solution:** We implement a **Trapezoidal Velocity Profile** that gradually ramps speed (steps per second) on a linear slope during starting (acceleration) and stopping (deceleration) phases. 

We will write a non-blocking program that sweeps a bipolar stepper back and forth, managing velocity slopes and utilizing the A4988 driver's enable state to save power.

---

## 🔬 Physics & Hardware Theory

### 1. Bipolar Motor Phase Drive
Unlike unipolar motors (Day 34) which use center-tapped coils to pull current in one direction, bipolar motors have **two independent coils (no center tap)**. Reversing direction requires reversing the current flow inside the entire coil. This is accomplished using a dual H-bridge driver.

---

### 2. A4988 Current Chopping & Microstepping
The A4988 is a dedicated microstepping driver with translator indexer control:
* **Current Chopping:** Stepper motor coils are inductive. When voltage is applied, current takes time to build up. To get current flowing quickly, we apply a high voltage (e.g. $12\text{V}-24\text{V}$) to a $3.3\text{V}$ coil. Once the current reaches a safe limit, the A4988 chops (switches OFF) the voltage, maintaining a constant current level.
* **Microstepping:** Instead of turning phases completely ON and OFF (full step), the A4988 feeds varying proportions of sinusoidal currents into both coils simultaneously. This creates intermediate magnetic steps, allowing the rotor to balance between poles. This divides a standard $1.8^{\circ}$ step into 1/2, 1/4, 1/8, or 1/16 microsteps, resolving vibrations and increasing step count:
$$\text{Steps per Revolution (1/16 mode)} = 200 \text{ full steps} \times 16 = 3200 \text{ steps}$$

---

### 3. Current-Limit Calibration ($V_{\text{ref}}$ Formula)
Before powering your motor, you must calibrate the onboard trimpot (potentiometer) on the A4988. If the current limit is set too high, the A4988 or the motor will burn out. If set too low, the motor will skip steps.

Measure the DC voltage between the metal wiper of the trimpot and Ground ($V_{\text{ref}}$). Calculate $V_{\text{ref}}$ using the formula:
$$V_{\text{ref}} = I_{\text{limit}} \times 8 \times R_{\text{sense}}$$

* $I_{\text{limit}}$: The rated current of your motor (e.g., $1.0\text{ A}$ per phase).
* $R_{\text{sense}}$: The value of the current sense resistors on your A4988 board (usually $0.10\ \Omega$ or $0.068\ \Omega$; marked as R100 or R068 on the chip board).
* For example, for a $1.0\text{ A}$ motor on a board with R100 ($0.1\ \Omega$) sense resistors:
  $$V_{\text{ref}} = 1.0\text{ A} \times 8 \times 0.1\ \Omega = 0.8\text{ V}$$

---

### 4. Trapezoidal Acceleration Profile
A trapezoidal speed profile consists of three distinct phases:

```
 Speed (SPS)
     ▲          Cruise Phase (constant speed)
     │            ┌──────────────┐
Max  │           /                \
Speed│          /                  \
     │         /                    \
     │        /                      \
Min  │       /                        \
Speed│______/                          \______ Standstill
     │   Accel Phase                Decel Phase
     └────────────────────────────────────────► Time
```

1. **Acceleration Phase:** Speed increases linearly over time: $v(t) = v_0 + a \cdot t$.
2. **Cruise Phase:** Speed clamps at maximum target velocity.
3. **Deceleration Phase:** Speed decreases linearly: $v(t) = v_{\text{cruise}} - a \cdot t$. The deceleration trigger point is calculated dynamically based on remaining distance.

---

## 🔄 Alternatives Comparison

When selecting stepper motor drivers:

| Driver Chip | Microstep Options | Max Current | Interface | Protection features | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **A4988** | **Full, 1/2, 1/4, 1/8, 1/16** | **$2.0\text{ A}$** | **Step / Dir** | **Thermal Shutdown** | **Standard 3D printers, low-cost CNCs (Our choice)** |
| **DRV8825** | **Down to 1/32** | **$2.5\text{ A}$** | **Step / Dir** | **Thermal & Overcurrent**| **Higher torque stepper applications ($24\text{V}$ systems)** |
| **TMC2209** | **Down to 1/256 (Interpolated)** | **$2.8\text{ A}$** | **UART / Step-Dir**| **SilentStepStick, StallGuard** | **Whisper-quiet operations, sensorless homing printers** |
| **L298N** | **Full step only (Manual)** | **$2.0\text{ A}$** | **4-wire interface**| **None** | **Basic educational unipolar/bipolar sweeps (Inefficient)** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x NEMA 17 Bipolar Stepper Motor
* 1x A4988 Driver Board (with heatsink attached)
* 1x External DC Power Source (e.g. $12\text{V}$ battery or power supply)
* 1x Breadboard
* 1x 100 µF decoupling electrolytic capacitor (critical to protect driver from voltage spikes)
* Jumper wires
* Multimeter and small screwdriver (for $V_{\text{ref}}$ tuning)

---

## 🔌 Pin-to-Pin Wiring

> [!CAUTION]
> 1. Always connect a **$100\ \mu\text{F}$ capacitor** directly across the VMOT and GND pins on the A4988 board to prevent LC voltage spikes from destroying the driver.
> 2. **Never connect or disconnect a stepper motor while the driver is energized.** Doing so will blow the A4988 output stages instantly.

| A4988 Driver Pin | Arduino / Power Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **STEP** | **D5** | Yellow | Step trigger pulses |
| **DIR** | **D6** | Green | Rotation direction state |
| **ENABLE** | **D7** | Orange | Enable/disable motor coils |
| **VDD** | **5V** | Red | Logic power supply |
| **GND (Logic)** | **GND** | Black | Logic ground reference |
| **RESET & SLEEP** | **Tied Together** | Jumper wire | Jumper connection to boot driver |
| **VMOT** | **12V Positive (+)** | Red (Heavy) | External motor power ($8\text{V}-35\text{V}$) |
| **GND (Power)** | **12V Negative (-)** | Black (Heavy) | External motor return ground |
| **1A & 1B** | **Coil A Leads** | Red / Blue | Connect to Stepper Coil A |
| **2A & 2B** | **Coil B Leads** | Black / Green | Connect to Stepper Coil B |

---

## 💻 How to Test & Validate

1. Wire up the components on the breadboard. Keep the $12\text{V}$ power supply unplugged. **Ensure the $100\ \mu\text{F}$ capacitor is connected across VMOT and GND!**
2. **Current-Limit Setup:**
   * Plug in the Arduino USB (5V logic active, VMOT inactive).
   * Put your multimeter in DC Voltage mode ($2\text{V}$ scale). Connect the black probe to Arduino GND. Touch the metal tip of the red probe to the metal screw of the trimpot on the A4988.
   * Adjust the screw slowly with a screwdriver until the voltage reads your calculated $V_{\text{ref}}$ (e.g. $0.6\text{V} - 0.8\text{V}$ depending on motor).
3. Upload `Day_35_Stepper_A4988.ino` to the Arduino.
4. Connect the external $12\text{V}$ power source.
5. Observe the motor:
   * The motor will start accelerating smoothly, cruise at $0.5$ rev/sec, and then decelerate to a stop.
   * The coils will de-energize, and the motor will enter a 2-second pause.
   * The cycle repeats in the reverse direction.
6. Open the **Serial Monitor** at **9600 Baud** to verify state transition logs:
   ```
   [MOTION] Commanded 6400 steps | Direction: CW
   [RAMP] Cruise Speed Reached: 1600 SPS (Steps taken to accel: 1045)
   [RAMP] Deceleration Phase Initiated.
   [MOTION] Target reached. Coils disabled. Entering 2s standstill...
   ```

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The motor makes a high-pitched whining noise but does not rotate:**
  * The current limit is set too low. Increase the $V_{\text{ref}}$ trimpot value slightly.
  * The step frequency (`MAX_SPEED_SPS`) is set too high for the motor's voltage. Try decreasing the speed or increasing the motor supply voltage to $24\text{V}$.
* **The motor rotates a few steps, makes a clicking sound, and stalls:**
  * The coil pairs are wired incorrectly. Identify the coil pairs using a multimeter (continuity mode: coils A and B will beep across their respective lead pairs). Swap leads to ensure Coil A is on 1A/1B and Coil B is on 2A/2B.
* **The A4988 driver board gets extremely hot and shuts down intermittently:**
  * The current limit is set too high. Check your $V_{\text{ref}}$ calculations and adjust the trimpot down. Make sure the aluminum heatsink is installed on the A4988 chip.

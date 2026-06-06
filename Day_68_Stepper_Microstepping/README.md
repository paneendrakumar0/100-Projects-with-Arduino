# Day 68: Stepper Motor Microstepping (A4988 Driver — All Step Resolutions)

Welcome to Day 68! Today we master **stepper motor microstepping** — the technique that makes 3D printers, CNC routers, and camera sliders move smoothly and silently. We configure all five microstepping modes on the **A4988 driver** (Full / Half / ¼ / ⅛ / 1/16 step) by toggling the MS1/MS2/MS3 pins, and implement a **trapezoidal velocity profile** to accelerate and decelerate without skipping steps.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Stepper_Motor.jpg" alt="Stepper Motor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

A NEMA 17 stepper motor has 200 full steps per revolution (1.8° per step). In **full-step mode**, the rotor physically jumps between positions — causing vibration, resonance at certain speeds, and loud operation.

**Microstepping** solves this by energizing both motor coils simultaneously with varying current levels (approximating a sine wave), creating intermediate virtual steps between the physical poles:

| Mode | Steps/Rev | Angular Resolution | Smoothness |
| :--- | :--- | :--- | :--- |
| **Full Step** | 200 | 1.800° | Vibrates, loud |
| **Half Step** | 400 | 0.900° | Smoother |
| **1/4 Step** | 800 | 0.450° | Good |
| **1/8 Step** | 1600 | 0.225° | Very smooth |
| **1/16 Step** | 3200 | 0.113° | Silky — used in 3D printers |

---

## 🔬 Physics & Mathematics

### 1. Stepper Motor Coil Physics
A two-phase stepper has two coil pairs (A and B). In full-step mode, one coil is fully on and the other is off:
$$\text{Phase A} = \pm I_{max}, \quad \text{Phase B} = 0 \text{ or } \pm I_{max}$$

In microstepping, both coils are energized simultaneously with sinusoidal amplitudes. For $N$ microsteps between full steps:
$$I_A(n) = I_{max} \cdot \cos\!\left(\frac{n \pi}{2N}\right), \quad I_B(n) = I_{max} \cdot \sin\!\left(\frac{n \pi}{2N}\right)$$

The rotor aligns to the combined magnetic field vector, stopping at $N$ intermediate positions between physical poles.

### 2. A4988 MS Pin Truth Table

| MS1 | MS2 | MS3 | Mode | Steps/Rev (1.8°) |
| :--- | :--- | :--- | :--- | :--- |
| LOW | LOW | LOW | Full | 200 |
| HIGH | LOW | LOW | Half | 400 |
| LOW | HIGH | LOW | 1/4 | 800 |
| HIGH | HIGH | LOW | 1/8 | 1600 |
| HIGH | HIGH | HIGH | 1/16 | 3200 |

### 3. Trapezoidal Velocity Profile
Accelerating instantly to full speed risks losing steps under load inertia. The trapezoidal profile ramps speed linearly:

```
Speed (steps/s)
  ┌──────────────────────────────────────┐
  │      ┌──────────CRUISE──────────┐    │
  │  ACC/                            \DEC │
  │ /                                  \  │
  └──────────────────────────────────────┘
       Time →
```

The step period (µs between steps) is linearly interpolated:
$$\text{period}(i) = \text{START\_DELAY} - \frac{i}{\text{ACCEL\_STEPS}} \times (\text{START\_DELAY} - \text{MIN\_DELAY})$$

### 4. Current Sensing (A4988 VREF)
The A4988 limits coil current via a sense resistor. The current limit is set by trimming the VREF potentiometer on the driver board:
$$I_{max} = \frac{V_{REF}}{8 \times R_{sense}}$$

For typical A4988 boards with $R_{sense} = 0.1\,\Omega$:
$$I_{max} = \frac{V_{REF}}{0.8}$$

Set $V_{REF} = 0.4\,\text{V}$ for $I_{max} = 0.5\,\text{A}$ (good for NEMA 17 at lower torque). Measure VREF between the trimmer and GND.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Controller |
| A4988 Stepper Driver | 1 | Current-controlled microstepping |
| NEMA 17 Stepper Motor | 1 | 1.8°, 200 steps/rev |
| 12V DC Power Supply (≥ 1A) | 1 | Motor voltage (VMOT) |
| 100 µF Electrolytic Capacitor | 1 | VMOT decoupling (prevents voltage spikes) |
| 10 kΩ Potentiometer | 1 | Speed control |
| Push Button | 1 | Mode cycling |

> ⚠️ **CRITICAL: Connect the 100 µF capacitor between VMOT and GND BEFORE powering on.** Without it, back-EMF from the motor can spike and destroy the A4988 permanently.

### Alternatives to A4988

| Driver | Max Current | Microstepping | Notes |
| :--- | :--- | :--- | :--- |
| **A4988** | 2A | 1/16 | Our choice |
| DRV8825 | 2.5A | 1/32 | Higher resolution |
| TMC2208 | 1.4A RMS | 1/256 | Virtually silent (StealthChop) |
| TB6600 | 4A | 1/32 | For larger NEMA 23/34 motors |

---

## 🔌 Pin-to-Pin Wiring

| A4988 Pin | Arduino / Supply | Description |
| :--- | :--- | :--- |
| **STEP** | **D3** | Step pulse |
| **DIR** | **D4** | Direction |
| **MS1** | **D5** | Microstep bit 0 |
| **MS2** | **D6** | Microstep bit 1 |
| **MS3** | **D7** | Microstep bit 2 |
| **ENABLE** | **D8** | Active-LOW enable |
| **RESET** | **5V** | Must be HIGH |
| **SLEEP** | **5V** | Must be HIGH |
| **VMOT** | **12V supply** | Motor power (with 100µF cap to GND) |
| **VDD** | **5V** | Logic power |
| **GND** | **GND** | Both GND pins |
| **1A/1B/2A/2B** | Motor coils | Refer to motor datasheet |

---

## 💻 How to Test & Validate

1. Set VREF on the A4988 to match your motor's rated current (measure with multimeter).
2. Upload [Day_68_Stepper_Microstepping.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_68_Stepper_Microstepping/Day_68_Stepper_Microstepping.ino).
3. Open **Serial Monitor** at **9600 Baud**.
4. The motor runs one revolution CW, then one CCW, repeating. Rotate the pot to change speed.
5. Press **D2 button** to cycle through step modes. Observe in serial: `[Mode] 1/16 Step (3200/rev)`.
6. **Listen:** Full step is loud and jittery; 1/16 step is nearly silent and buttery smooth.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Motor doesn't move | ENABLE pin not driven LOW | Check `digitalWrite(ENABLE_PIN, LOW)` before stepping |
| Motor vibrates but doesn't rotate | Wrong coil wiring (1A/1B/2A/2B) | Swap either 1A↔1B or 2A↔2B |
| Motor skips steps at high speed | MIN_DELAY_US too small or VREF too low | Increase `MIN_DELAY_US` or raise VREF |
| A4988 gets very hot | VREF too high | Lower VREF to match motor rated current |
| Smoke / burnt smell | No 100µF cap on VMOT | Power off immediately, add capacitor |

## 🧠 Code Explanation

Let's break down how we achieve silky smooth stepper motion:

### 1. Hardware Microstepping Control
```cpp
digitalWrite(MS1_PIN, HIGH);
digitalWrite(MS2_PIN, HIGH);
digitalWrite(MS3_PIN, HIGH); // 1/16th Step Mode
```
- A standard stepper motor has 200 physical "teeth" per revolution (1.8° per step). If you step them individually, the motor jumps and vibrates violently.
- By toggling the `MS1`, `MS2`, and `MS3` pins on the A4988 driver, we command its internal DACs (Digital-to-Analog Converters) to feed sinusoidal partial-currents to the coils.
- This creates magnetic "sub-steps" between the physical teeth. At 1/16th mode, our 200-step motor transforms into an ultra-smooth 3,200-step motor!

### 2. The Trapezoidal Velocity Profile
```cpp
if (i < accelSteps) {
  stepDelay = map(i, 0, accelSteps, START_DELAY_US, minDelayUs);
} else if (i > steps - accelSteps) {
  stepDelay = map(i, steps - accelSteps, steps, minDelayUs, START_DELAY_US);
}
```
- A stepper motor has zero torque at high speeds. If you instantly command it to spin at 1000 RPM, the magnetic field will spin faster than the physical rotor can accelerate, and the motor will screech and stall ("skipping steps").
- We use a Trapezoidal Profile: We start with a long `stepDelay` (slow speed), and linearly decrease it (accelerate) over the first 100 steps. 
- We cruise at maximum speed in the middle, and linearly increase the delay (decelerate) at the end. This prevents stalling and allows for massive top speeds!

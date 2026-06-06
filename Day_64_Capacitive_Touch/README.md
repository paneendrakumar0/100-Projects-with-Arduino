# Day 64: Capacitive Touch Sensor (RC Charge-Time Measurement — No Library)

Welcome to Day 64! Today we build a **capacitive touch sensor from scratch** — no dedicated touch IC, no library. Using just two Arduino pins and a resistor, we detect the electrical capacitance of the human body by measuring how long it takes to charge an RC circuit. This is exactly the principle behind every modern smartphone touchscreen, laptop trackpad, and capacitive button.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

* **Why not a mechanical button?** Capacitive touch has no moving parts (no wear), works through glass/plastic, can sense proximity, and enables gesture detection.
* **Why no library?** Understanding the RC charge-time method shows you exactly how the `CapacitiveSensor` library works internally — the same technique used by chips like AT42QT1010 and TTP223.
* **The body is a capacitor:** The human body has a capacitance of roughly 100–300 pF. When you touch a metal electrode, your body adds this capacitance to the circuit, lengthening the RC charge time — which our code measures.

---

## 🔬 Physics & Mathematics

### 1. RC Circuit Charging Equation
When SEND_PIN goes HIGH, current flows through the resistor R and charges the stray capacitance C of the electrode:
$$V(t) = V_{CC} \times \left(1 - e^{-t/(RC)}\right)$$

The time to reach the digital HIGH threshold $V_{IH} \approx 0.5 \times V_{CC}$:
$$t_{charge} = -R \times C \times \ln\!\left(1 - \frac{V_{IH}}{V_{CC}}\right) \approx 0.693 \times R \times C$$

### 2. Touch Effect on Charge Time

| State | Capacitance | Approximate Charge Time |
| :--- | :--- | :--- |
| **No touch** | $C_{stray} \approx 10{-}50\,\text{pF}$ | Baseline count (short) |
| **Touch** | $C_{stray} + C_{body} \approx 100{-}350\,\text{pF}$ | Baseline + Delta (longer) |

With $R = 1\,\text{M}\Omega$ and $C_{body} = 200\,\text{pF}$:
$$\Delta t = 0.693 \times 1\,\text{M}\Omega \times 200\,\text{pF} = 0.693 \times 200\,\mu\text{s} = 138.6\,\mu\text{s}$$

This is easily measurable by counting Arduino loop iterations.

### 3. Measurement Algorithm (Loop Counter Method)

```
DISCHARGE:  pinMode(SENSE, OUTPUT) → LOW for 10 µs  (discharge stray C)
CHARGE:     pinMode(SENSE, INPUT)
            digitalWrite(SEND, HIGH)
COUNT:      While SENSE == LOW: count++          (charge-time proxy)
RESET:      Drive SENSE LOW again

Repeat N=30 times, average count → one reading
```

The raw count is proportional to RC charge time. The absolute value varies by MCU clock speed and loop overhead — that's why we use a **delta from baseline** rather than absolute values.

### 4. Auto-Calibration
At startup, 50 samples are taken with no touch to establish the baseline. Touch detection uses:
$$\text{Delta} = \text{reading} - \text{baseline} > \text{TOUCH\_THRESHOLD}$$

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Controller + measurement |
| 1 MΩ Resistor | 1 | RC series resistor (D4 → D5) |
| Metal foil / wire / copper tape | 1 | Touch electrode (connects to D5) |
| 47 pF Capacitor | 1 | Optional: noise filter on D5 to GND |

> 💡 **Resistor Value Guide:** Use 1 MΩ for close-proximity detection (touch required), 10 MΩ for proximity (hover 1–2 cm away), 100 kΩ for very fast response (must touch firmly).

### Dedicated Touch IC Alternatives

| IC | Interface | Notes |
| :--- | :--- | :--- |
| **Our software method** | GPIO (2 pins + R) | Free, flexible, no extra IC |
| TTP223 | GPIO output | Single-channel, self-calibrating |
| AT42QT1010 | GPIO output | More robust, industrial |
| MPR121 | I2C | 12-channel, supports proximity |
| FT5336 | I2C | Capacitive multi-touch panel IC |

---

## 🔌 Pin-to-Pin Wiring

```
Arduino D4 ──── 1 MΩ ──── Arduino D5 ──── Touch Electrode (foil/wire)
                                │
                               [47 pF optional, to GND]
Arduino D13 ──── LED ──── 220Ω ──── GND  (touch indicator)
```

| Component | Arduino Pin |
| :--- | :--- |
| **SEND_PIN** (driver) | **D4** |
| **SENSE_PIN** (measurement) | **D5** |
| **Touch LED** | **D13** |
| Resistor | Between D4 and D5 |
| Electrode | Connected to D5 |

---

## 💻 How to Test & Validate

1. Wire the 1 MΩ resistor between D4 and D5. Connect a 10–20 cm bare wire or square of kitchen foil to D5.
2. Upload [Day_64_Capacitive_Touch.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_64_Capacitive_Touch/Day_64_Capacitive_Touch.ino).
3. Open **Serial Monitor** at **9600 Baud**.
4. **Do NOT touch the electrode during startup** — the baseline calibration runs for ~1 second.
5. After calibration, observe the baseline count printed.
6. Touch the electrode — `[CapTouch] *** TOUCH DETECTED ***` and LED D13 lights up.
7. Release — `[CapTouch] --- Released ---` and LED goes off.
8. The `Delta` column shows how much charge time increased due to your body capacitance.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Always shows TOUCHED | Baseline too low / resistor wrong | Use exactly 1 MΩ; run calibration without touching |
| Never shows TOUCHED | Threshold too high or electrode too small | Lower `TOUCH_THRESHOLD` to 50; make electrode larger |
| Erratic readings | Electrical noise / fluorescent lights | Add 47 pF cap D5 to GND; shield electrode wire |
| Reading = 5000 (MAX_CYCLES) | SENSE_PIN never goes HIGH | Check resistor connection and wiring |

## 🧠 Code Explanation

Let's break down how we measured physical capacitance without any special chips:

### 1. The RC Time Constant
```cpp
digitalWrite(SEND_PIN, HIGH);
int count = 0;
while (digitalRead(SENSE_PIN) == LOW && count < MAX_CYCLES) {
  count++;
}
```
- Capacitors take time to fill up with electricity. The time it takes is directly proportional to how big the capacitor is.
- By connecting a massive 1-Megohm resistor between `SEND` and `SENSE`, we slow down the charging process of the physical wire.
- We pull `SEND` HIGH, and then rapidly count in a `while` loop until `SENSE` registers a HIGH voltage (2.5V). 
- If the wire is untouched, it charges very fast (low count). When a human touches the wire, the human body acts as a massive capacitor, soaking up the electricity and slowing down the voltage rise (high count)!

### 2. Dynamic Calibration
```cpp
baseline = calSum / CALIBRATE_READS;
long delta = reading - baseline;
bool isTouched = (delta > TOUCH_THRESHOLD);
```
- Humidity, wire length, and nearby electronics change the baseline capacitance of the room.
- To prevent false triggers, the Arduino takes 50 readings at boot to establish what "normal" looks like.
- In the main loop, we only trigger a touch if the current reading spikes drastically (`delta`) above that dynamic baseline!

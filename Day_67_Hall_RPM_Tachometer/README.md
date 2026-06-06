# Day 67: Hall Effect RPM Tachometer (Timer1 Input Capture Unit)

Welcome to Day 67! Today we build a **precision RPM tachometer** using the ATmega328P's **Timer1 Input Capture Unit (ICU)** — a hardware peripheral that timestamps edges on a pin with zero software latency. A Hall Effect sensor detects magnets on a rotating shaft and the ICU records the exact timer count at each pulse, giving us period measurements accurate to **0.5 µs** — far more precise than any software interrupt approach.

---

## 🎯 The "Why" and "What"

* **Software interrupts** (using `attachInterrupt`) have latency jitter of 2–10 µs depending on what code is running. At 10,000 RPM, a pulse arrives every 6 ms — 10 µs jitter is already 0.17% error.
* **Input Capture Unit** latches the timer value the **instant** the edge occurs in hardware — before the ISR even starts. This eliminates interrupt latency entirely.
* **Hall Effect sensors** are non-contact, wear-free, and work at high speed through plastic enclosures — used in automotive wheel speed sensors, brushless motor ESCs, washing machine speed control, and industrial tachometers.

---

## 🔬 Physics & Mathematics

### 1. Hall Effect Sensor Working Principle
When a permanent magnet's field passes through the Hall sensor's semiconductor slab, the **Lorentz force** deflects charge carriers sideways, creating a measurable voltage difference (the Hall voltage):
$$V_H = \frac{I \times B}{n \times q \times d}$$

In practice, the integrated Hall IC converts this to a clean digital output: **LOW when magnet present, HIGH otherwise** (for A3144-type latching/unipolar sensors).

### 2. RPM Calculation from Period
Each magnet pass = one pulse. With one magnet per revolution:

$$\text{Period}_{ticks} = T_2 - T_1 \quad \text{(two consecutive capture timestamps)}$$
$$\text{Period}_{sec} = \frac{\text{Period}_{ticks}}{f_{tick}} = \frac{\text{Period}_{ticks}}{F_{CPU} / \text{Prescaler}}$$
$$\text{RPM} = \frac{60}{\text{Period}_{sec} \times N_{magnets}}$$

With Prescaler = 8 → $f_{tick} = 2\,\text{MHz}$ → **0.5 µs per tick**:

| RPM | Period (ms) | Period (ticks) | Overflow risk? |
| :--- | :--- | :--- | :--- |
| 10,000 | 6 ms | 12,000 | No |
| 1,000 | 60 ms | 120,000 | No |
| 100 | 600 ms | 1,200,000 | Yes — ~18 overflows |
| 10 | 6,000 ms | 12,000,000 | Yes — handled by overflow counter |

### 3. Timer1 Overflow Handling for Low RPM
Timer1 is 16-bit, counting to 65,535. At $f_{tick} = 2\,\text{MHz}$, it overflows every:
$$T_{overflow} = \frac{65536}{2{,}000{,}000} = 32.77\,\text{ms}$$

For slow motors (e.g., 10 RPM → 6 second period → ~183 overflows), we count overflows between captures using the `TIMER1_OVF_vect` ISR and reconstruct the full 32-bit period:
$$\text{Period}_{32} = (\text{overflowCount} \times 65536) + T_2 - T_1$$

### 4. Input Capture Noise Canceller (ICNC1)
Setting `ICNC1 = 1` enables a 4-sample majority filter on the ICP1 pin. The capture is only triggered after 4 **consecutive** equal samples at the I/O clock rate. This delays capture by 4 clock cycles (~250 ns) but eliminates false triggers from electrical noise on long sensor wires.

### 5. Input Capture vs External Interrupt Comparison

| Method | Timestamp Accuracy | CPU Load | Best For |
| :--- | :--- | :--- | :--- |
| **Timer1 ICU (our choice)** | **Hardware — zero jitter** | **Very low (1 ISR per pulse)** | **Precision tachometry, frequency measurement** |
| `attachInterrupt()` + `micros()` | ±2–10 µs software jitter | Low | Simple counting, debounced events |
| Polling in `loop()` | Worst (misses pulses) | High | Only for very slow events |

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | ICU measurement |
| Hall Effect Sensor (A3144 or SS49E) | 1 | Magnetic pulse detection |
| Small Neodymium Magnet(s) | 1+ | Mounted on rotating shaft |
| 10 kΩ Pull-up Resistor | 1 | For open-collector sensor output |
| Small DC motor or fan | 1 | Rotating target |

### Hall Sensor Alternatives

| Sensor | Type | Output | Notes |
| :--- | :--- | :--- | :--- |
| **A3144** | Unipolar | Digital (open-collector) | Our choice, requires pull-up |
| SS49E | Linear | Analog | Measures field strength — not digital |
| AH3503 | Ratiometric | Analog | For absolute position |
| HMC5883L | 3-axis magnetometer | I2C | Different use case |

---

## 🔌 Pin-to-Pin Wiring

```
Hall Sensor VCC ──── 5V
Hall Sensor GND ──── GND
Hall Sensor OUT ─┬── D8 (ICP1 — Input Capture Pin, mandatory)
                 │
               10kΩ ──── 5V  (pull-up for open-collector output)
```

> ⚠️ **Pin 8 (ICP1) is MANDATORY.** The hardware Input Capture Unit is hardwired to this pin on the ATmega328P. No other pin will work without modifying the architecture.

Magnet mounting: glue one small neodymium disc magnet to the side of your motor shaft, wheel, or fan blade. Position the Hall sensor 1–5 mm away with the sensor face toward the magnet.

---

## 💻 How to Test & Validate

1. Wire per the diagram. Glue one magnet to your rotating shaft.
2. Upload [Day_67_Hall_RPM_Tachometer.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_67_Hall_RPM_Tachometer/Day_67_Hall_RPM_Tachometer.ino).
3. Open **Serial Monitor** at **9600 Baud**.
4. Start your motor/fan. Readings appear once per revolution: `[TACHO] RPM: 1234.5 | Freq: 20.58 Hz | Period: 48.6 ms | Ticks: 97200`.
5. Speed up the motor — RPM should increase proportionally.
6. Stop the motor — after 2 seconds: `[TACHO] Motor STOPPED or RPM < 1`.
7. For multiple magnets: change `MAGNETS_PER_REV` to 2, 4, etc. to get correct RPM.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| No readings | Wrong pin | Sensor must connect to **Pin 8 (ICP1)** |
| RPM is 2× too high | 2 magnets mounted but `MAGNETS_PER_REV = 1` | Set `MAGNETS_PER_REV = 2` |
| Wildly fluctuating readings | No noise canceller or long unshielded wire | `ICNC1` is enabled — shorten sensor wire |
| Always shows 0 RPM | Sensor output never goes LOW | Check sensor type; reverse magnet polarity (South pole) |
| RPM correct at high speed, wrong at low | Overflow not counted | Confirm `TIMSK1 = _BV(ICIE1) | _BV(TOIE1)` — both interrupts enabled |

## 🧠 Code Explanation

Let's break down how we measure extreme RPMs using Hardware Timers:

### 1. The Input Capture Unit (ICU)
```cpp
TCCR1B = _BV(ICNC1) | _BV(CS11); 
TIMSK1 = _BV(ICIE1);
```
- Reading a fast motor with `digitalRead()` or even standard `attachInterrupt()` introduces Software Jitter—tiny delays caused by the CPU doing other things. This ruins RPM calculations at high speeds.
- We bypass software entirely and use the ATmega328P's internal hardware: The **Input Capture Unit**.
- When the Hall Sensor detects a magnet, the hardware *instantly* takes a snapshot of the 16-bit `Timer1` clock and freezes it into the `ICR1` register. The CPU can then casually read this perfectly accurate timestamp later!

### 2. Handling Timer Overflows
```cpp
uint32_t periodTicks = ((uint32_t)oflows << 16) + (uint32_t)cap - (uint32_t)last;
```
- Timer1 runs so fast (2 MHz) that it resets to zero every 32 milliseconds.
- If the motor is spinning slowly (e.g., 60 RPM = 1 second per revolution), Timer1 will overflow dozens of times between magnet detections!
- We enable an Overflow Interrupt (`TIMER1_OVF_vect`) to count how many times the clock rolls over. By combining the 16-bit hardware timestamp (`cap`) with our 16-bit software rollover count (`oflows`), we create a massive 32-bit timestamp capable of tracking periods from microseconds all the way up to hours!

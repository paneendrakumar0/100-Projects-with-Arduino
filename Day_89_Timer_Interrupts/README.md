# Day 89: Timer Interrupts for Precise Sampling (CTC Mode & Dynamic Prescaling)

Welcome to Day 89! Today we master the art of **Precision Datalogging** by replacing soft-timed loops with hardware-driven **Timer Interrupts** on the ATmega328P. We will configure the 16-bit **Timer1** in **Clear Timer on Compare Match (CTC)** mode to fire analog read cycles at exact frequencies (50 Hz, 100 Hz, and 200 Hz). We will analyze the physics of sampling jitter, perform timer arithmetic, and build a real-time telemetry parser.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

In digital signal processing (DSP), control theory (PID loops), and sensor datalogging, we assume that data is sampled at a **perfectly constant interval** ($\Delta t$). 

If you sample a sensor in your main `loop()` using a simple check like `if (millis() - lastSample >= 10)`, your sampling interval is **not constant**. It is subject to **jitter**:
- If `Serial.print()` takes 2ms, your next sample is delayed.
- If a sensor takes 5ms to read, your sampling frequency drops.
- This timing variation is called **sampling jitter**. In PID loops, jitter causes instability and oscillations. In DSP (like FFT analysis), jitter distorts frequencies (frequency modulation artifacts).

### What is a Timer Interrupt?
A **Timer Interrupt** is a hardware-level alarm. When the hardware counter inside the chip reaches a certain value, the hardware pauses CPU execution instantly and triggers the timer's ISR. This guarantees that your sensor is sampled at the exact same interval (with sub-microsecond precision), completely independent of whatever the main `loop()` is doing!

---

## 🔬 Physics & Hardware Theory

### 1. Timer Jitter vs. Precise Hardware Timing
The diagram below illustrates how polling in a main loop introduces timing variations compared to a dedicated hardware timer.

```
Loop Polling (With Jitter):
├────── 10ms ──────┼─── 12ms (Serial Write Delay) ───┼── 8ms ───┤
▲                  ▲                                 ▲
Sample 1           Sample 2                          Sample 3

Timer Interrupt (Deterministic):
├────── 10ms ──────┼────── 10ms ──────┼────── 10ms ──────┤
▲                  ▲                  ▲                  ▲
Sample 1           Sample 2           Sample 3           Sample 4
```

### 2. Clear Timer on Compare (CTC) Mode
The ATmega328P has a 16-bit timer called **Timer1**. In CTC mode (specifically Mode 4), the timer counter register (`TCNT1`) increments on every clock cycle (divided by a prescaler). 
The hardware constantly compares `TCNT1` with the value in the Compare Match Register (`OCR1A`).
When `TCNT1` matches `OCR1A`:
1. `TCNT1` is instantly cleared back to $0$ on the next clock cycle.
2. The Timer1 Compare Match A interrupt flag is set, triggering `ISR(TIMER1_COMPA_vect)`.

### 3. Timer Arithmetic
To configure Timer1, we must choose a prescaler (a hardware clock divider that slows the 16 MHz clock) and calculate the compare match value `OCR1A`:
$$\text{OCR1A} = \frac{f_{\text{CPU}}}{\text{Prescaler} \cdot f_{\text{target}}} - 1$$

Where:
- $f_{\text{CPU}} = 16,000,000\,\text{Hz}$
- $f_{\text{target}}$ is our desired sampling rate (e.g., $100\,\text{Hz}$)
- **Prescaler** is a factor of 1, 8, 64, 256, or 1024.

Let's do the math for $100\,\text{Hz}$:
- If $\text{Prescaler} = 1$: $\text{OCR1A} = \frac{16,000,000}{1 \cdot 100} - 1 = 159,999$ (Overflows the 16-bit register limit of 65,535).
- If $\text{Prescaler} = 8$: $\text{OCR1A} = \frac{16,000,000}{8 \cdot 100} - 1 = 19,999$ (Fits perfectly in 16-bit).

Let's do the math for $200\,\text{Hz}$ (with prescaler 8):
$$\text{OCR1A} = \frac{16,000,000}{8 \cdot 200} - 1 = 9,999$$

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| Potentiometer (Optional) | 1 | Connect to A0 to act as a variable analog sensor |
| Jumper Wires & Breadboard | 1 | Prototyping |

---

## 🔌 Pin-to-Pin Wiring

If using a Potentiometer:
| Potentiometer Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **Pin 1 (Left)** | **GND** | Ground reference |
| **Pin 2 (Wiper)** | **A0** | Analog input signal |
| **Pin 3 (Right)** | **5V** | 5V Power Supply |

If no sensor is connected:
- **No wiring required**. The code detects the floating pin and automatically outputs a simulated sine wave + noise for testing.

---

## 💾 Alternatives to Timer Interrupts

| Method | Timing Jitter | CPU Overhead | Max Frequency | Ease of Implementation | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Hardware Timer Interrupt (CTC)** | **< 1 µs** | **Very Low** | > 100 kHz | Moderate | Best for precise data acquisition and control loops. |
| **Millis/Micros Polling** | 100 µs – 10 ms | Moderate | ~10 kHz | Easy | Fine for simple applications, but highly susceptible to blockages. |
| **Delay Loops** | Massive | 100% (Blocks CPU) | ~1 kHz | Very Easy | Prevents execution of any other code. Avoid in professional work. |
| **ADC Free-Running Interrupt** | < 1 µs | Low | 9.6 kHz (10-bit) | Hard | ADC automatically triggers its own conversion complete interrupt. |

---

## 💻 How to Test & Validate

1. Open the Arduino IDE, load [Day_89_Timer_Interrupts.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_89_Timer_Interrupts/Day_89_Timer_Interrupts.ino), and select the COM port.
2. Select your Arduino Uno board and upload the code.
3. Open the **Serial Monitor** at **115200 Baud**. (Ensure you use 115200; the standard 9600 is too slow for 200 Hz telemetry).
4. You will see configuration messages showing Timer1 has initialized at 100 Hz.
5. **Verify Precision Timing**:
   - The data is printed in the format: `TimeUs:<value>,IntervalUs:<value>,SampleValue:<value>`.
   - Observe the `IntervalUs` output.
   - At **100 Hz**, it should read exactly `10000` (or `10004`/`9996` due to the 4-microsecond resolution limit of `micros()`).
   - This proves the sample is taken every $10,000\,\mu\text{s}$ (10ms) with zero drift!
6. **Dynamic Tuning via CLI**:
   - Send `1` to change frequency to **50 Hz**. Observe `IntervalUs` change to exactly `20000` (20ms).
   - Send `3` to change frequency to **200 Hz**. Observe `IntervalUs` change to exactly `5000` (5ms).
   - Send `2` to restore **100 Hz**.
   - Send `p` to pause the output data stream to inspect values.
7. **Visualizing Waveform (Serial Plotter)**:
   - Close the Serial Monitor and open **Tools > Serial Plotter** at **115200 Baud**.
   - You will see the clean, perfectly-sampled sine wave.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `IntervalUs` fluctuates wildly (e.g. 5000 then 15000) | Main loop is delayed and processing multiple samples | Ensure the main loop is not blocked by slow function calls. The timer interrupt fires on time, but if the main loop is slow, it prints accumulated samples late. |
| Output reads garbage characters | Baud rate mismatch | Change the Serial Monitor/Plotter baud rate to **115200**. |
| Analog input value is flat | Potentiometer connected incorrectly | Verify wiring: Pin 1 to GND, Pin 2 (wiper) to A0, Pin 3 to 5V. |
| `millis()` or `delay()` stops working | Timer0 registers modified | Avoid modifying Timer0 configuration (`TCCR0A`/`TCCR0B`) as it drives `millis()`. Use Timer1 or Timer2 instead. |

## 🧠 Code Explanation

Let's break down how to guarantee mathematically perfect sampling rates using Timers:

### 1. The Problem with millis()
- If you poll a sensor using `if (millis() - lastTime > 10)`, the actual timing might be 10ms, 11ms, or 15ms depending on what `Serial.print()` or other code is doing. This timing "jitter" ruins advanced math like Fast Fourier Transforms (FFT) or PID Controllers.

### 2. Configuring Timer1 for CTC Mode
```cpp
OCR1A = 19999;
TCCR1B |= (1 << WGM12) | (1 << CS11);
```
- The ATmega328P has a 16-bit hardware counter (Timer1) driven by the 16 MHz crystal.
- We set a Prescaler of 8, slowing the timer to 2 MHz. We then set the Compare Match Register (`OCR1A`) to 19,999.
- In **Clear Timer on Compare Match (CTC)** mode, the timer counts up to 19,999, instantly resets to 0, and triggers an interrupt. 
- $2,000,000 / 20,000 = 100	ext{ Hz}$. We have achieved mathematically flawless 10.000 millisecond ticks!

### 3. The Timer ISR
```cpp
ISR(TIMER1_COMPA_vect) {
  sampledValue = analogRead(A0);
  newSampleAvailable = true;
}
```
- Every 10ms, no matter what `loop()` is doing, the CPU halts and executes this ISR. It samples the ADC precisely on time, raises a boolean flag, and returns. `loop()` handles the slow serial printing only when the flag is true.

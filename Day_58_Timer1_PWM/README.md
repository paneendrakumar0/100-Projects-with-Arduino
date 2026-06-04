# Day 58: Hardware Timer1 PWM Generator (Direct AVR Registers)

Welcome to Day 58 of the 100-Day Arduino Masterclass! Today, we go deep into the AVR microcontroller's hardware peripherals and configure **Timer1 in Fast PWM Mode** using direct register manipulation. We completely bypass the Arduino `analogWrite()` abstraction to achieve precise control over both **PWM frequency** (from 100 Hz to 25 kHz) and **duty cycle** (0–100%) using a potentiometer.

---

## 🎯 The "Why" and "What"

Arduino's `analogWrite()` is fixed at ~490 Hz or ~980 Hz depending on the pin — you cannot change it without modifying hardware registers.
* **The Problem:** Applications like motor speed controllers, audio signal generators, and switching power supply controllers require precise, application-specific PWM frequencies that `analogWrite()` cannot provide.
* **The Solution:** We configure Timer1 in **Fast PWM Mode 14** (WGM = `1110`) with `ICR1` as the TOP value. This lets us set any frequency within the hardware's range by choosing ICR1, and independently set the duty cycle through OCR1A.

---

## 🔬 Physics & Mathematics

### 1. Timer1 Prescaler and Counting
Timer1 is a 16-bit up-counter running from `0` to `ICR1` (TOP). The counter increments at:
$$f_{\text{count}} = \frac{F_{\text{CPU}}}{\text{Prescaler}} = \frac{16\,\text{MHz}}{8} = 2\,\text{MHz}$$

### 2. PWM Frequency Equation
One full PWM cycle = counting from 0 to ICR1 and resetting:
$$f_{\text{PWM}} = \frac{f_{\text{count}}}{\text{ICR1} + 1} = \frac{F_{\text{CPU}}}{\text{Prescaler} \times (\text{ICR1} + 1)}$$

Solving for ICR1:
$$\text{ICR1} = \frac{F_{\text{CPU}}}{\text{Prescaler} \times f_{\text{PWM}}} - 1$$

**Example — 1 kHz with Prescaler = 8:**
$$\text{ICR1} = \frac{16{,}000{,}000}{8 \times 1{,}000} - 1 = 2000 - 1 = 1999$$

### 3. Duty Cycle Equation
The output pin goes HIGH when the counter = 0, and LOW when the counter matches OCR1A:
$$\text{OCR1A} = \frac{\text{DutyCycle}}{100} \times (\text{ICR1} + 1)$$

**Example — 25% duty at 1 kHz (ICR1 = 1999):**
$$\text{OCR1A} = 0.25 \times 2000 = 500$$

### 4. Timer Register Map (Mode 14 — Fast PWM, TOP = ICR1)

| Register | Bit Pattern | Description |
| :--- | :--- | :--- |
| **TCCR1A** | `COM1A1=1, WGM11=1` | Non-inverted output on OC1A (Pin 9), Mode bits |
| **TCCR1B** | `WGM13=1, WGM12=1, CS=010` | Mode bits + Prescaler /8 |
| **ICR1** | 16-bit TOP value | Controls PWM frequency |
| **OCR1A** | 16-bit compare | Controls duty cycle |

---

## 🔄 PWM Generation Comparison

| Method | Frequency Range | Resolution | CPU Impact | Best Used For |
| :--- | :--- | :--- | :--- | :--- |
| **Timer1 Hardware PWM (Our choice)** | **$\approx 0.24\,\text{Hz} - 2\,\text{MHz}$** | **16-bit ICR1** | **Zero (Hardware runs automatically)** | **Motor control, BLDC ESCs, power supplies** |
| `analogWrite()` | Fixed 490 Hz or 980 Hz | 8-bit (0–255) | Zero (Hardware) | Simple LED dimming, basic motor speed |
| Software Bit-Bang | Limited by code speed | Varies | Very High (Busy loop) | Educational demonstrations only |

---

## 🛠️ Components Needed
* 1x Arduino Uno
* 2x $10\,\text{k}\Omega$ Potentiometers (Duty Cycle on A0, Frequency Preset on A1)
* 1x Breadboard & Jumper wires
* 1x Oscilloscope or logic analyzer (optional, for waveform verification)

---

## 🔌 Pin-to-Pin Wiring

| Component | Arduino Pin | Description |
| :--- | :--- | :--- |
| **Pot 1 Wiper** | **A0** | Duty cycle control (0–100%) |
| **Pot 2 Wiper** | **A1** | Frequency preset selector |
| **PWM Output** | **D9 (OC1A)** | Hardware Timer1 output |

Both potentiometers: outer pins to 5V and GND, middle wiper to analog input.

---

## 💻 How to Test & Validate

1. Upload [Day_58_Timer1_PWM.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_58_Timer1_PWM/Day_58_Timer1_PWM.ino).
2. Open the **Serial Monitor** at **9600 Baud**.
3. Rotate **Pot 1 (A0)** to vary the duty cycle. The serial monitor prints live ICR1 and OCR1A values.
4. Rotate **Pot 2 (A1)** to switch between frequency presets: `100 Hz → 500 Hz → 1 kHz → 5 kHz → 10 kHz → 25 kHz`.
5. Connect an oscilloscope probe to **Pin 9** to measure the actual waveform.

---

## 🛠️ Troubleshooting Guide

* **Pin 9 stays constant HIGH or LOW**: Ensure `pinMode(9, OUTPUT)` is called before configuring TCCR1A. The OC1A output only drives the pin when it is configured as an output.
* **Frequency is exactly half of expected**: Your ICR1 calculation may be using integer division incorrectly. Cast operands to `uint32_t` before multiplying to prevent 16-bit overflow (as done in `computeICR()`).
* **analogWrite() on Pin 9 stops working after this sketch**: Timer1 registers persist across sketches until the Uno is power-cycled or reset. Upload any other sketch to restore defaults.

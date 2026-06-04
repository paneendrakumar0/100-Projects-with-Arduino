# Day 32: Quadrature Rotary Encoder (KY-040 HMI Controller)

Welcome to Day 32 of the 100-Day Arduino Masterclass! Today, we study human-machine interface (HMI) design and external hardware interrupts. We will interface the **KY-040 Quadrature Rotary Encoder** to build a dynamic, interactive selection menu. We will learn how to decode quadrature wave phase shifts in an Interrupt Service Routine (ISR) and implement microsecond-level software debounce filters.

---

## 🎯 The "Why" and "What"

Robotics and industrial machines require reliable user input interfaces to configure parameters, select modes, and navigate settings.
* **The Problem:** Using standard pushbuttons for a menu with dozens of parameters is slow and takes up too many pins. Potentiometers can map values but lack infinite rotation and digital feedback (clicks).
* **The Solution:** A **Rotary Encoder** provides infinite, bi-directional rotation with detent feedback (physical clicks) and an integrated push-button switch. It can scroll through long menus and adjust numeric values with single-pin interrupt responsiveness.

---

## 🔬 Physics & Hardware Theory

### 1. Quadrature Phase Coding (Gray Code)
A rotary encoder converts angular position or motion of a shaft into analog or digital codes.
* Internally, the encoder contains a circular disk with evenly spaced metal contact pads connected to common ground (GND), alongside two contact pins (Channel A/CLK and Channel B/DT).
* **The Phase Shift:** The contact pads are offset such that as the shaft rotates, the two channels generate square wave pulses that are **90 degrees out of phase**. This is a two-bit **Gray Code** sequence where only one bit changes state at a time.

```
Clockwise (CW) Rotation:
CLK (A)  ──┐  ┌──┐  ┌──
           └──┘  └──┘  
DT (B)   ────┐  ┌──┐  ┌
             └──┘  └──┘
(CLK falls to LOW first; at that instant, DT is still HIGH)

Counter-Clockwise (CCW) Rotation:
CLK (A)  ────┐  ┌──┐  ┌
             └──┘  └──┘
DT (B)   ──┐  ┌──┐  ┌──
           └──┘  └──┘  
(DT falls to LOW first; at that instant, when CLK falls to LOW, DT is already LOW)
```

By reading the state of Channel B (DT) at the exact instant Channel A (CLK) falls to **LOW**, we determine the direction:
* If `DT == HIGH`, it is rotating **Clockwise (CW)**.
* If `DT == LOW`, it is rotating **Counter-Clockwise (CCW)**.

---

### 2. External Hardware Interrupts
Standard microcontrollers check pin states by reading them repeatedly in the `loop()` function (polling). If the processor is busy executing code, a user spinning the encoder quickly will cause the board to miss transitions, resulting in menu lag.

An **External Interrupt** bypasses this:
* We wire the CLK pin to Pin 2 (which supports hardware interrupt `INT0` on the ATmega328P).
* We configure the interrupt to run on a `FALLING` edge:
  `attachInterrupt(digitalPinToInterrupt(CLK_PIN), handleEncoderISR, FALLING);`
* When Pin 2 drops from HIGH to LOW, the CPU halts its current instruction, jumps to the **Interrupt Service Routine (ISR)** `handleEncoderISR()`, increments/decrements the position counter, and returns to normal execution. This guarantees zero missed steps.

---

### 3. Mechanical Contact Bounce & ISR Debouncing
Mechanical switches contain small metal plates that vibrate (bounce) rapidly when closing. This bounce generates a train of high-frequency pulses for up to $2 - 5\text{ ms}$.
* In normal code, this bounce is ignored or filtered by a `delay(50)`.
* **In ISRs, delays are forbidden!** The microsecond timer `micros()` continues to run, but `delay()` will lock the system since it relies on timers that require interrupts.
* **The Solution:** We implement a **microsecond lockout timer** in the ISR. If the current time is within $2000\text{ µs}$ ($2\text{ ms}$) of the last interrupt, the CPU discards the pulse as noise.

---

## 🔄 Alternatives Comparison

When selecting input devices for control menus:

| Control Device | Rotation Limit | Pins Required | Interface Speed / Lag | Software Complexity | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Rotary Encoder** | **Infinite** | **3 (CLK, DT, SW)** | **Interrupt-Instant** | **Medium (Requires ISR/Debounce)** | **Menu navigation, volume/speed knobs, parameter settings (Our choice)** |
| **Potentiometer** | **$270^{\circ} - 300^{\circ}$** | **1 (Analog)** | **Polled ADC** | **Low** | **Tuning volumes, setting speeds, simple analog mapping** |
| **Pushbuttons (x3)** | **N/A** | **3 (Up, Down, Enter)** | **Polled / Interrupt** | **Low** | **Simple settings adjustments** |
| **Analog Joystick** | **Spring-centered** | **3 (X, Y, SW)** | **Polled ADC** | **Medium** | **Manual motor steering, 2D cursor controls** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x KY-040 Rotary Encoder Module
* 1x Breadboard
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

| KY-040 Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **CLK** | **D2** | Green | Channel A (External Interrupt Pin) |
| **DT** | **D4** | Yellow | Channel B (Direction Data Pin) |
| **SW** | **D3** | Orange | Push Button Switch Pin |
| **+** (VCC) | **5V** | Red | Power Input |
| **GND** | **GND** | Black | Ground Reference |

---

## 💻 How to Test & Validate

1. Wire the KY-040 encoder module to the Arduino board according to the pin connections table.
2. Open `Day_32_Rotary_Encoder.ino` and upload it.
3. Open the **Serial Monitor** at **9600 Baud**.
4. You will see the main system menu printed to the console:
   ```
   ====== MAIN SYSTEM MENU ======
   Rotate encoder to scroll. Press button to select.
   --------------------------------
    -> [*] System Status Monitor
       [ ] Temperature Diagnostic
       [ ] Wipe Non-Volatile Memory
       [ ] Hardware System Information
   ==============================
   ```
5. Rotate the encoder shaft slowly:
   * With each physical click (detent), you will feel a mechanical bump.
   * The menu will update instantly. The selection pointer `-> [*]` moves up or down matching your rotation direction.
6. Scroll to **Wipe Non-Volatile Memory** and press down on the encoder shaft (triggering the integrated button):
   * The console prints a confirmation: `>>> RUNNING: Wipe Non-Volatile Memory`.
   * It runs the event action printout, holds for 2.5 seconds, and then returns to active menu selection.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **Rotating the encoder causes the selector to jump randomly or skip items:**
  * Mechanical encoders wear out over time, producing long contact bounce durations. Try increasing the debounce lockout time in `handleEncoderISR()` from `2000` to `3000` or `4000` microseconds.
* **The selection moves in the opposite direction of rotation:**
  * Swap the CLK and DT pins in hardware, or swap their pin assignment numbers in the code:
    `const int CLK_PIN = 4; const int DT_PIN = 2;`
* **Pressing the button does nothing:**
  * Make sure the SW pin is connected to Pin 3.
  * Verify the button contact by measuring continuity between SW and GND pins while pressing the shaft down.

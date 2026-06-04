# Day 2: SOS Morse Code Generator (Functions & Timing)

Welcome to Day 2 of the 100-Day Arduino Masterclass! Yesterday, we learned how to blink an LED without freezing the microcontroller. Today, we are going to make our Arduino speak.

We will build an SOS Morse Code generator (••• ——— •••) using both light (LED) and sound (Active Piezo Buzzer). In doing so, we will learn a critical programming concept: **Modular Timing Sequences**. Instead of freezing the controller with blocking code, we will structure our program using arrays and a non-blocking sequence loop powered by `millis()`.

---

## 🧠 Hardware Theory: How Do These Components Work?

Before we wire anything, let's understand the physics of our actuators.

### 1. The Light Emitting Diode (LED)
An LED is not a tiny lightbulb; there is no filament burning inside. It is a semiconductor diode. It consists of two types of materials pressed together: P-type (positive, containing "holes") and N-type (negative, containing extra electrons).

When voltage is applied in the correct direction (Anode to Cathode), electrons from the N-type side rush across the P-N junction to fill the holes in the P-type side. As they fall into these holes, they drop to a lower energy state. This lost energy is released in the form of a photon (light).

### 2. The Active Piezo Buzzer
A piezo buzzer relies on the **Piezoelectric Effect**. Inside the plastic housing is a thin ceramic disk bonded to a metal plate. When you apply a voltage to this ceramic material, it physically deforms and bends. When you remove the voltage, it snaps back.

An "Active" buzzer has a small oscillator circuit built inside it. When you give it a steady 5V DC signal from the Arduino, that internal circuit rapidly turns the voltage on and off, causing the ceramic disk to flex thousands of times a second. This rapid flexing pushes the air, creating a sound wave we hear as a loud beep.

---

## ⏱️ The Rules of Morse Code Timing

To make real Morse Code, timing is everything. We will base our timing on one "unit" of time (set to 200 milliseconds in the code).

- **Dot (•):** 1 unit long.
- **Dash (—):** 3 units long.
- **Gap between parts of the same letter:** 1 unit long.
- **Gap between letters:** 3 units long.
- **Gap between words (repeating the SOS):** 7 units long.

---

## 🔄 Alternatives: Active vs. Passive Buzzers

| Actuator Type | How It Works | Advantages | Disadvantages | Why We Chose It |
| :--- | :--- | :--- | :--- | :--- |
| **Active Buzzer** | Has an internal oscillator. Beeps automatically when given 5V DC. | Simple to use. No complex audio frequencies to generate in code. | Can only play one fixed tone frequency. | **Chosen** for this project to keep the focus on timing and sequence logic. |
| **Passive Buzzer** | Requires an AC signal (square wave) from the Arduino using PWM or `tone()` function to vibrate. | Can play different musical notes and melodies (adjustable pitch). | Requires more code and timer resources to generate audio frequencies. | Better for music, but adds unnecessary complexity for basic SOS warning signals. |
| **Magnetic Speaker** | Uses an electromagnet coil to vibrate a paper/plastic cone. | Can play voice clips, rich sound effects, and music. | Needs an audio amplifier circuit; high power draw can damage Arduino pins directly. | Too complex for a simple day-to-day warning indicator. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega** (or compatible).
2. **Active Piezo Buzzer** (5V).
3. **LED** (Red or green).
4. **220Ω Resistor** (for the LED).
5. **Half-size Breadboard**.
6. **Jumper Wires** (4 male-to-male wires).
7. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

| Component | Component Pin | Arduino Pin | Wire Color (Recommended) | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Buzzer** | positive (+) / Long leg | **Pin 8** | Red | Active buzzer control line |
| **Buzzer** | negative (-) / Short leg | **GND** | Black | System ground |
| **LED** | Anode (+) / Long leg | **Pin 13** | Yellow | Onboard/External LED control |
| **LED** | Cathode (-) / Short leg | **220Ω Resistor** to **GND** | Black | Current limiting circuit to ground |

---

## 🧪 How to Test and Validate

Follow these steps to upload, run, and verify the SOS Morse Code generator:

### 1. Verification of Sound and Light Sync
- Once the code is uploaded, the Arduino should immediately begin playing the SOS pattern.
- **Verify Synchronicity:** Check that the external/onboard LED turns ON at the exact microsecond the active buzzer begins beeping, and turns OFF at the exact microsecond the buzzer goes silent.

### 2. Verification of Sequence Timing
- Open the Serial Monitor in the Arduino IDE (**Tools > Serial Monitor**).
- Verify the baud rate is set to **9600**.
- You should see the following logs printing in real-time, matching the rhythm of the beeps:
  ```text
  • [ON]  Duration: 200 ms
    [OFF] Duration: 200 ms
  • [ON]  Duration: 200 ms
    [OFF] Duration: 200 ms
  • [ON]  Duration: 200 ms
    [OFF] Duration: 600 ms
  • [ON]  Duration: 600 ms
  ```
- **Check the Rhythm:**
  - You should hear three quick beeps (S).
  - A short pause.
  - Three long beeps (O).
  - A short pause.
  - Three quick beeps (S).
  - A long pause (7 units = 1400ms) before the cycle starts again.

### 🔍 Troubleshooting Tips
* **The buzzer clicking instead of beeping:**
  - Ensure you are using an **Active** buzzer. A passive buzzer will make clicking sounds when turned on and off. If using a passive buzzer, you must use the `tone(8, 2000)` command instead of `digitalWrite(8, HIGH)`.
* **The buzzer is too loud:**
  - You can connect a 100Ω or 220Ω resistor in series with the buzzer pin to decrease the current and lower the volume.
* **The LED doesn't blink:**
  - Make sure the LED is not inserted backward (the long leg must connect to Pin 13).
  - Ensure the resistor is connected properly between the cathode and Ground.

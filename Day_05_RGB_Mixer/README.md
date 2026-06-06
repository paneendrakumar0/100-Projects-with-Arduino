# Day 5: RGB LED Color Mixer (PWM Additive Mixing)

Welcome to Day 5 of the 100-Day Arduino Masterclass! Today, we will explore **additive color mixing** by controlling a multi-channel actuator: the **RGB LED**. 

Instead of hardcoding single colors, you will learn how to write a mathematically elegant program that uses three phase-shifted sine waves to sweep through the color wheel continuously, creating a smooth rainbow transition.

---

## 🎯 Today's Learning Goals
1. Understand the internal structure and pinout of Common Cathode RGB LEDs.
2. Master the principles of additive color mixing (Red + Green + Blue = White).
3. Learn how to calculate resistor sizes based on LED forward voltages ($V_f$).
4. Implement a phase-shifted sinusoidal color cycling algorithm.
5. Setup multi-channel PWM outputs on the Arduino.

---

## 🧠 The "Why" and "What": RGB LEDs in Robotics

### What is an RGB LED?
An RGB LED is a compact light source that package three distinct Light Emitting Diodes (one Red, one Green, and one Blue) inside a single transparent or diffused epoxy case. It has four pins: one common connection (either ground or 5V) and three individual anode or cathode pins for each color channel.

### Why is it Used in Robotics?
In robotics and automation systems, RGB LEDs are primary visual feedback indicators to communicate the machine's state to human users:
- **Telemetry Indicators:** Blue = Bluetooth searching; Green = Connected; Red = Error/Disconnected.
- **Battery Status Displays:** Pulsing Green = Charging; Solid Green = Full battery; Orange = Warning; Flashing Red = Critical battery.
- **Sensor Alignment & State Feedback:** In line-following robots, an RGB LED can change color depending on which sensor is currently detecting the black line, aiding in manual calibration.
- **Esthetics & Lighting:** Custom illumination for styling, headlamps on robotic platforms, or safety warning strobes.

---

## ⚡ The Physics & Hardware Theory

### 1. The Physics of Semiconductor Bandgaps (LED Colors)
Why do Red, Green, and Blue LEDs emit different colors? It comes down to the semiconductor materials used in their P-N junctions. 

Different materials have different **bandgaps** (the energy gap electrons must cross to release light). The size of this bandgap ($E_g$) dictates the energy level of the released photon, which in turn determines the wavelength ($\lambda$) and color of the light:

$$E_g = \frac{h \cdot c}{\lambda}$$

Where:
* $h$ is Planck's constant.
* $c$ is the speed of light.
* $\lambda$ is the wavelength.

* **Red LED (Gallium Arsenide Phosphide - GaAsP):** Small bandgap ($\approx 1.8\text{ to }2.0\text{ eV}$), emitting long wavelengths ($\approx 620\text{ to }750\text{ nm}$).
* **Green LED (Indium Gallium Nitride - InGaN):** Medium bandgap ($\approx 2.2\text{ to }2.5\text{ eV}$), emitting medium wavelengths ($\approx 495\text{ to }570\text{ nm}$).
* **Blue LED (Gallium Nitride - GaN):** Large bandgap ($\approx 2.7\text{ to }3.2\text{ eV}$), emitting short wavelengths ($\approx 450\text{ to }495\text{ nm}$).

### 2. Common Cathode vs. Common Anode Configurations
RGB LEDs come in two internal wiring configurations:
* **Common Cathode (Used in this project):** The negative terminals (cathodes) of all three LEDs are tied together internally and connected to a single ground (GND) pin. To light up a color, you write a `HIGH` voltage to its corresponding anode pin.
* **Common Anode:** The positive terminals (anodes) of all three LEDs are tied together internally and connected to 5V (VCC). To light up a color, you must write a `LOW` voltage (GND) to its cathode pin (Active-Low).

```
         Common Cathode (GND)                       Common Anode (5V)
         
               GND                                        +5V
                |                                          |
        +-------+-------+                          +-------+-------+
        |       |       |                          |       |       |
      |   |   |   |   |   |                      |   |   |   |   |   |
      | R |   | G |   | B |                      | R |   | G |   | B |
      V___V   V___V   V___V                      V___V   V___V   V___V
        |       |       |                          |       |       |
        R_Pin   G_Pin   B_Pin                      R_Pin   G_Pin   B_Pin
```

### 3. Calculating Current-Limiting Resistors
Because the three color channels are made of different materials, they have different **Forward Voltages ($V_f$)** (the voltage required to light them). 
* Red $V_f \approx 1.8\text{V}$ to $2.0\text{V}$
* Green $V_f \approx 3.0\text{V}$ to $3.2\text{V}$
* Blue $V_f \approx 3.0\text{V}$ to $3.2\text{V}$

We must calculate the ideal resistor ($R$) for each channel using Ohm's Law ($V = I \cdot R$) assuming a standard safe operating current of $I = 15\text{mA}$ ($0.015\text{A}$):

$$R = \frac{V_{in} - V_f}{I}$$

* **For Red:** $R = \frac{5\text{V} - 2.0\text{V}}{0.015\text{A}} = 200\Omega$ (Standard value: **220Ω**)
* **For Green & Blue:** $R = \frac{5\text{V} - 3.2\text{V}}{0.015\text{A}} = 120\Omega$ (Standard value: **150Ω** or **220Ω**)

> [!CAUTION]
> **Never use a single resistor on the common pin** instead of three separate resistors. Because of the different forward voltages, the Red channel (lowest $V_f$) will hog all the current, preventing Green and Blue from lighting up properly and potentially burning out the Red channel.

---

## 🔄 Alternatives: RGB LEDs vs. NeoPixels

| Device | Type | Communication | Max Colors | Pin Count | Cost | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Standard RGB LED** | Analog Multi-channel | Direct PWM (1 pin per channel) | Infinite (continuous analog mixing) | 4 pins (needs 3 Arduino PWM pins) | Very Low | Basic state indication, learning PWM basics, high speed updates. |
| **Addressable RGB LED (WS2812B / NeoPixel)** | Digital Smart LED | One-wire High-speed Serial | $2^{24} = 16.7$ million (8-bit digital) | 3 pins total (VCC, GND, Data) for **daisy-chained chains of hundreds of LEDs** | Moderate | Multi-LED matrices, custom light strips, complex robotic notification rigs. |
| **Separated LEDs** | Isolated discretes | Single digital output per LED | 3 distinct colors | 2 pins per LED | Very Low | Simple indicators where space is not a premium and color mixing isn't required. |

---

## 🛠️ Components Needed

To build this circuit, you will need:
1. **Arduino Uno or Mega**.
2. **Common Cathode RGB LED**.
3. **Three 220Ω Resistors** (or two 150Ω and one 220Ω).
4. **Half-size Breadboard**.
5. **Jumper Wires** (4 male-to-male wires).
6. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

Look at your RGB LED. Typically, the pins are ordered as: **Red, Common Cathode (longest pin), Green, Blue**.

| RGB Pin | Function | Connect To | Wire Color | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Pin 1** (Short outer) | Red Anode | **220Ω Resistor** ➡️ Arduino **Pin 9** | Red | PWM control for Red |
| **Pin 2** (Longest pin) | Common Cathode | Arduino **GND** | Black | Common Ground |
| **Pin 3** (Short inner) | Green Anode | **220Ω Resistor** ➡️ Arduino **Pin 10** | Green | PWM control for Green |
| **Pin 4** (Short outer) | Blue Anode | **220Ω Resistor** ➡️ Arduino **Pin 11** | Blue | PWM control for Blue |

---

## 🧪 How to Test and Validate

Follow these steps to upload, run, and verify the color mixer:

### 1. Visual Verification of the Color Wheel
- Connect your Arduino to your computer and upload `Day_05_RGB_Mixer.ino`.
- The RGB LED should instantly begin cycling through colors smoothly.
- **Check the transitions:**
  - Red shifts to Orange and Yellow.
  - Yellow transitions into Green.
  - Green shifts to Cyan and Blue.
  - Blue transitions to Violet, Magenta, and back to Red.
- **Diffusion tip:** If the colors look like three separate light spots rather than a mixed color, place a small piece of white paper, a translucent plastic cup, or a hot glue stick over the LED. This diffuses the light and blends the colors together.

### 2. Serial Monitor Verification
- Open the Serial Monitor (**Tools > Serial Monitor**) at **9600 Baud**.
- Verify the telemetry logs are updating in real-time:
  ```text
  Sweep Angle: 120° | PWM R: 0 | G: 255 | B: 0
  ```
- **Check Phase Shift Values:**
  - At Angle 0° (or 360°), Red should be near maximum (255), Green and Blue should be equal and low.
  - At Angle 120°, Green should be near maximum (255), Red and Blue low.
  - At Angle 240°, Blue should be near maximum (255), Red and Green low.

### 🔍 Troubleshooting Tips
* **The colors are jumping abruptly instead of shifting smoothly:**
  - Make sure the resistors are connected to PWM-capable pins (9, 10, and 11). If you connect to non-PWM pins (like 7, 8, 12), the pins will default to binary high/low, causing the LED to flicker through 8 basic colors instead of blending.
* **One color is completely dead:**
  - Check the jumper wire connection for that color channel.
  - Make sure the resistor is plugged into the correct breadboard row.
  - Swap the dead channel's wire with a working one to isolate whether the LED is burnt out or the Arduino pin is faulty.
* **The colors are inverted (e.g. it turns OFF when it should be ON):**
  - You likely have a **Common Anode** RGB LED instead of Common Cathode.
  - **The Fix:** Connect the longest pin to **5V** instead of GND. In the code, invert the values written to the pins by changing `analogWrite(pin, val)` to `analogWrite(pin, 255 - val)`.

## 🧠 Code Explanation

Let's break down the math behind the smooth Rainbow cycle:

### 1. The Sine Wave Math
```cpp
angle += angleIncrement;

int rVal = (sin(angle) + 1.0) * 127.5;
int gVal = (sin(angle + (2.0 * PI / 3.0)) + 1.0) * 127.5;
int bVal = (sin(angle + (4.0 * PI / 3.0)) + 1.0) * 127.5;
```
- To make a smooth rainbow, we treat the colors like a wave.
- `sin(angle)` generates a math wave that goes from `-1.0` to `1.0`. 
- Since PWM can't be negative, we add `1.0` to make it range from `0.0` to `2.0`.
- We then multiply by `127.5`. The result is a perfect wave oscillating between `0` and `255`!
- The secret to the rainbow is **phase shifting**. We shift the Green wave by 120 degrees (`2*PI/3` radians) and the Blue wave by 240 degrees. This ensures that when Red is fading out, Green is perfectly fading in!

### 2. Outputting the Colors
```cpp
analogWrite(RED_PIN, rVal);
analogWrite(GREEN_PIN, gVal);
analogWrite(BLUE_PIN, bVal);
```
- We use PWM on all three pins simultaneously.
- If `rVal = 255`, `gVal = 255`, and `bVal = 0`, the LED will shine Yellow. The smooth sine waves do all this blending for us automatically!

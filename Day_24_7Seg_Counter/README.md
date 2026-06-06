# Day 24: 7-Segment Display Counter (Bit-Masking)

Welcome to Day 24 of the 100-Day Arduino Masterclass! Today, we study basic LED segment interfaces. We will learn how to drive a single-digit 7-segment LED display to count from 0 to 9.

You will master common cathode/anode electrical layouts, calculate current-limiting resistor loads, and implement **binary bit-mask parsing** in code to steer digital output headers.

---

## 🎯 Today's Learning Goals
1. Identify the pinout and segment structure of 7-segment displays.
2. Master the electrical differences between Common Cathode and Common Anode displays.
3. Calculate current-limiting resistor values for individual LED segments.
4. Program a binary bit-mask lookup table.
5. Use bitwise operations (`bitRead`) to drive multiple pins in loop routines.

---

## 🧠 The "Why" and "What": 7-Segments in Robotics

### What is a 7-Segment Display?
A 7-segment display is an optoelectronic device that displays numbers by lighting up combinations of seven individual LED segments (arranged in a block-8 shape). An eighth LED is typically included for the decimal point (DP).

```
          7-Segment Layout Design
          
                 -- A --
               |         |
               F         B
               |         |
                 -- G --
               |         |
               E         C
               |         |
                 -- D --   (o) DP
```

### Why is it Used in Robotics?
While LCD screens (Day 21) are great for detailed text, 7-segment displays are chosen for simplicity, speed, and high visibility:
- **High-Visibility Speedometers:** Displaying wheel RPM or vehicle speed on mobile rovers so it can be seen from meters away in bright sunlight.
- **Scoreboards & Counter Displays:** Counting sorted products on robotic conveyor belts.
- **Diagnostics Codes:** Showing simple error codes (like `E1` for motor fault, `E2` for sensor disconnect) on small, low-cost microcontrollers.
- **Timer / Countdown Clocks:** Visual indicators for autonomous task windows.

---

## ⚡ The Physics & Hardware Theory

### 1. Common Cathode vs. Common Anode Configurations
Because a 7-segment display contains 8 separate LEDs, they share a common connection pin to reduce the overall pin count from 16 to 9 or 10.

* **Common Cathode (Used by default in code):** The negative terminals (cathodes) of all 8 segment LEDs are tied together internally and connected to Ground (GND). 
  - *Triggering:* You write `HIGH` (5V) to the individual segment anode pin to turn it ON.
* **Common Anode:** The positive terminals (anodes) of all 8 segment LEDs are tied together internally and connected to 5V (VCC). 
  - *Triggering:* You write `LOW` (GND) to the individual segment cathode pin to turn it ON (Active-Low).

```
         Common Cathode (GND shared)                Common Anode (5V shared)
         
                     GND                                        +5V
                      |                                          |
             +--------+--------+                        +--------+--------+
             |        |        |                        |        |        |
           |   |    |   |    |   |                    |   |    |   |    |   |
           | A |    | B |    | C |                    | A |    | B |    | C |
           V___V    V___V    V___V                    V___V    V___V    V___V
             |        |        |                        |        |        |
           Anode A  Anode B  Anode C                  Cath A   Cath B   Cath C
          (Pin 2)   (Pin 3)  (Pin 4)                 (Pin 2)  (Pin 3)  (Pin 4)
```

### 2. Resistor Calculations (Why 7 Resistors are Required)
Each segment is a single LED. Standard red LEDs have a forward voltage ($V_f \approx 2.0\text{V}$) and a safe current ($I \approx 15\text{mA}$). Using the resistor formula:

$$R = \frac{5\text{V} - 2.0\text{V}}{0.015\text{A}} = 200\Omega \implies \text{Standard Value: } 220\Omega$$

> [!CAUTION]
> **Do not use a single resistor on the common pin** instead of seven separate resistors. If you use one resistor on the GND common pin, the current is shared. If only one segment (like B in number '1') is ON, it gets the full current and shines brightly. If all 7 segments are ON (number '8'), they must share the current, causing the numbers to look extremely dim and fluctuating as numbers change. It can also exceed the current limit of a single LED segment if not calculated correctly.

### 3. Bit-Masking Math
To represent numbers, we use a single byte (8 bits) as a digital map. 
* We map the pins: `Pins = {A, B, C, D, E, F, G}`
* We map the bits: `0b0GFEDCBA` (Bit 0 is A, Bit 6 is G)
* **Number '3':** Needs segments A, B, C, D, G to be ON, E, F to be OFF.
  - Bit 0 (A) = 1
  - Bit 1 (B) = 1
  - Bit 2 (C) = 1
  - Bit 3 (D) = 1
  - Bit 4 (E) = 0
  - Bit 5 (F) = 0
  - Bit 6 (G) = 1
  - Bit 7 (DP)= 0
  - Result: `0b01001111` binary (or `0x4F` in hex).
* In code, the `bitRead(pattern, i)` function extracts the value of bit $i$ ($0$ or $1$) from the pattern, which we write directly to Pin $i$.

---

## 🔄 Alternatives: Single-Digit vs. Multi-Digit Multiplexers

| Display Type | Pin Count | Digits | Drive Complexity | Best Use Case |
| :--- | :---: | :---: | :--- | :--- |
| **Single-Digit 7-Segment** | 9 pins | 1 | Low (Direct mapping). | **Chosen** for learning segment physics, error numbers, and basic counts. |
| **Multi-Digit 7-Segment** | 12 pins | 4 | High (Requires rapid scanning multiplexing). | Digital clocks, stopwatches, counters. |
| **I2C 4-Digit (TM1637)** | **2 pins** (CLK/DIO) | 4 | Low (Handled by IC library). | Displaying clock times and distance readouts without wasting pins. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **Single-Digit 7-Segment Display** (Common Cathode recommended).
3. **Seven 220Ω Resistors** (one for each segment).
4. **Breadboard & Jumper Wires**.
5. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

Look at your display's pinout. Typically, a 10-pin single-digit display has 5 pins on the top and 5 pins on the bottom. The **middle pin** on both top and bottom is the **Common Pin (GND)**.

| Display Segment Pin | Arduino Connection (through 220Ω Resistor) | Wire Color | Description |
| :---: | :---: | :--- | :--- |
| **A** (Top segment) | Arduino **Pin 2** | Red | Segment A drive line |
| **B** (Top-right) | Arduino **Pin 3** | Orange | Segment B drive line |
| **C** (Bottom-right) | Arduino **Pin 4** | Yellow | Segment C drive line |
| **D** (Bottom segment) | Arduino **Pin 5** | Green | Segment D drive line |
| **E** (Bottom-left) | Arduino **Pin 6** | Blue | Segment E drive line |
| **F** (Top-left) | Arduino **Pin 7** | Purple | Segment F drive line |
| **G** (Middle segment) | Arduino **Pin 8** | Grey | Segment G drive line |
| **Common** (Middle pin) | Arduino **GND** | Black | Common Ground |

---

## 🧪 How to Test and Validate

Follow these steps to upload, verify, and debug your counter:

### 1. Confirm Pinout and Configuration
- Look at the back of your display to identify if it is Common Cathode or Common Anode.
- In the code (`Day_24_7Seg_Counter.ino`), look at line 42: `#define IS_COMMON_CATHODE true`.
  - If Common Anode, change this to `false`.
- Upload the code.

### 2. Verify Count Loop
- Once uploaded, the display should start counting from 0 to 9, changing numbers once every second.
- **Check Segment Lighting:**
  - Verify that each number is formed correctly:
    - '0' has all segments lit except the middle (G).
    - '1' has only B and C (right side).
    - '8' has all segments lit.
  - If one segment (e.g. the top) remains dead, verify the resistor and wiring at Pin 2.

### 3. Check Serial Monitor Telemetry
- Open the Serial Monitor at **9600 Baud**.
- Verify that the prints match the visual digit changes:
  ```text
  Displaying: 3 | Bit-Mask: 0b1001111
  Displaying: 4 | Bit-Mask: 0b1100110
  ```

### 🔍 Troubleshooting Tips
* **One number displays distorted shapes (e.g. '8' looks like 'E'):**
  - Check that the segment pins are not swapped. Ensure Segment A goes to Pin 2, B to Pin 3, C to Pin 4, and so on.
* **The segments light up very dimly:**
  - Verify you did not place a single resistor on the GND common pin instead of seven separate resistors on the anode pins.
* **The numbers are inverted (e.g., segments that should be OFF are ON, and vice versa):**
  - Your `#define IS_COMMON_CATHODE` setting does not match your display. Change the definition.

## 🧠 Code Explanation

Let's break down how we use Binary Bit-Masks to drive the 7-segment display:

### 1. The Binary Bit-Mask
```cpp
const byte numPatterns[] = {
  0b00111111, // 0: A, B, C, D, E, F ON
  0b00000110, // 1: B, C ON
  // ...
};
```
- A 7-segment display has 7 individual LEDs. To display a '1', we need segments B and C on, and the rest off.
- Instead of writing 7 messy `digitalWrite()` lines for every single number, we pack the ON/OFF states into a single 8-bit Byte (`0b0GFEDCBA`). 
- For '1', bits B and C are `1`, everything else is `0`. So the byte is `0b00000110`.

### 2. The Hardware Decoder
```cpp
byte pattern = numPatterns[number];
for (int i = 0; i < 7; i++) {
    int segmentState = bitRead(pattern, i);
    digitalWrite(segmentPins[i], segmentState);
}
```
- We grab the master pattern for the number we want to show.
- A `for` loop cycles through pins 0 through 6. The Arduino `bitRead()` function extracts the specific `1` or `0` for that exact segment from our byte, and we instantly write it to the physical pin!

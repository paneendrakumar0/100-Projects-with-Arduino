# Day 92: Macro Pad with Cherry MX Switches (Diode Matrix Scanning & USB HID Macros)

Welcome to Day 92! Today we build a **6-Key Custom Macro Pad** using Cherry MX style mechanical switches. We will explore how professional mechanical keyboards use a **Diode Matrix** to scan hundreds of keys using a small number of pins, analyze the physics of keyboard "ghosting", and implement multi-key macros over USB HID.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

If you wire 100 keyboard switches directly to a microcontroller, you would need 100 GPIO pins. Even for a small 6-key macro pad, wiring switches directly consumes 6 pins. 

To save pins, keyboards arrange keys in a **grid matrix** of Rows ($R$) and Columns ($C$). 
- The number of keys we can read is $R \times C$.
- The number of pins required is $R + C$.
- For our 6-key macro pad, we arrange them in a $2 \times 3$ matrix. This requires only **5 pins** instead of 6.
- A full 104-key standard keyboard arranged in a $12 \times 9$ matrix requires only **21 pins** instead of 104!

However, when you press multiple keys simultaneously (e.g., in gaming or fast typing), the electrical currents can cross-talk, leading to **ghosting** (the PC registers keys that were never pressed) or **masking** (pressed keys are ignored). To make the matrix reliable, we place a **diode** in series with every key switch.

---

## 🔬 Physics & Hardware Theory

### 1. The Ghosting Problem (Sneak Currents)
Consider a matrix without diodes. If you press three keys that form a corner of a rectangle in the grid—for example, (Row 0, Col 0), (Row 0, Col 1), and (Row 1, Col 1)—a closed loop is formed.

When the microcontroller pulls Row 1 LOW to scan it, current flows from Column 0 (HIGH due to pull-up), through the pressed key at (Row 0, Col 0), up to Row 0, across to Column 1 through the pressed key at (Row 0, Col 1), and down to Row 1 through the pressed key at (Row 1, Col 1). 
Because Column 0 is pulled LOW by this loop, the microcontroller thinks the key at **(Row 1, Col 0) is pressed**, even though it is open! This false reading is called a **Ghost Key**.

```
   WITHOUT DIODES (Ghosting):             WITH DIODES (Blocked):
   Col 0 (7)   Col 1 (8)                 Col 0 (7)   Col 1 (8)
       │           │                         │           │
Row 0 ─┼──[SW1]────┼──[SW2]───         Row 0 ─┼─[SW1]─►│─┼─[SW2]─►│─
(5)    │  (Pressed)│  (Pressed)        (5)    │ (Pressed)│ (Pressed)
       │           │                         │           │
Row 1 ─┼──[SW3]────┼──[SW4]───         Row 1 ─┼─[SW3]─►│─┼─[SW4]─►│─
(6)    │  (Open)   │  (Pressed)        (6)    │ (Open)   │ (Pressed)
       ▲                                      ▲
(Sneak current pulls Col 0 LOW)        (Diode blocks reverse current)
```

### 2. Diodes as One-Way Valves
By placing a diode (like the fast-switching **1N4148** signal diode) in series with every switch, we ensure that current can only flow in one direction: from the Column pin (input) to the Row pin (output). The diode blocks reverse current, preventing sneak paths and ensuring **N-Key Rollover (NKRO)**.

### 3. Matrix Scanning Sequence
The microcontroller loops through rows:
1. All Row pins are set to **high-impedance INPUT** (effectively disconnecting them).
2. All Column pins are set to **INPUT_PULLUP** (HIGH by default).
3. The microcontroller pulls **Row 0 LOW** by changing it to `OUTPUT` and writing `LOW`.
4. It reads the column pins. If any column reads `LOW`, the switch at that row-column intersection is closed.
5. Row 0 is set back to `INPUT`.
6. The microcontroller pulls **Row 1 LOW** and reads columns again.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Leonardo or Micro | 1 | Native USB support (ATmega32U4) |
| Cherry MX Keyswitches | 6 | Tactile mechanical keys |
| 1N4148 Diodes | 6 | Anti-ghosting diodes (one per switch) |
| Jumper Wires & Breadboard | 1 | Prototyping |
| Micro-USB Cable | 1 | USB connection to computer |

---

## 🔌 Pin-to-Pin Wiring

```
Row 0 (Pin 5) ───[Cathode Diode 1 Anode]───[Switch 1]─── Col 0 (Pin 7)
              ───[Cathode Diode 2 Anode]───[Switch 2]─── Col 1 (Pin 8)
              ───[Cathode Diode 3 Anode]───[Switch 3]─── Col 2 (Pin 9)

Row 1 (Pin 6) ───[Cathode Diode 4 Anode]───[Switch 4]─── Col 0 (Pin 7)
              ───[Cathode Diode 5 Anode]───[Switch 5]─── Col 1 (Pin 8)
              ───[Cathode Diode 6 Anode]───[Switch 6]─── Col 2 (Pin 9)
```

- **Row 0**: Connects to the cathodes (striped side) of Diodes 1, 2, and 3.
- **Row 1**: Connects to the cathodes (striped side) of Diodes 4, 5, and 6.
- **Columns**:
  - Column 0 (Pin 7) connects to switches 1 and 4.
  - Column 1 (Pin 8) connects to switches 2 and 5.
  - Column 2 (Pin 9) connects to switches 3 and 6.

---

## 💾 Alternatives to Keyboard Matrices

| Method | Pin Count | Ghosting Risk | Simultaneous Presses | CPU Overhead | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Diode Matrix** | **Low (R + C)** | **None** | **N-Key Rollover** | **Low** | Standard for high-quality keyboards. |
| **Resistor Ladder (ADC)** | Ultra-Low (1 pin) | High | Poor | High | Good for single-key inputs, fails on chords. |
| **Direct Pin Hookup** | High (1 pin/key) | None | N-Key Rollover | Very Low | Limited to small number of keys. |
| **I/O Shift Registers** | Low (3 pins SPI) | None | N-Key Rollover | Medium | Requires shift-in register chips like 74HC165. |

---

## 💻 How to Test & Validate

1. Wire up the 6 mechanical switches with 1N4148 diodes on a breadboard.
2. Open the Arduino IDE, load [Day_92_HID_Macro_Pad.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_92_HID_Macro_Pad/Day_92_HID_Macro_Pad.ino).
3. **If using native USB (Leonardo/Micro)**:
   - Select **Arduino Leonardo** board and upload the sketch.
   - Open a text editor.
   - Press **Switch 1**. It will trigger `Ctrl+C`. Press **Switch 2** to trigger `Ctrl+V`.
   - Verify that your text is copied and pasted instantly!
   - You can send number keys `1` to `6` in the Serial Monitor to trigger the macros over USB.
4. **If using Arduino Uno (Simulation Mode)**:
   - Select **Arduino Uno** and upload the sketch.
   - Open the **Serial Monitor** at **9600 Baud**.
   - Send `1`, `2`, `3`, `4`, `5`, or `6` in the serial input line.
   - The console will output:
     `[SIMULATED PRESS] Key 1 at Matrix (0,0) -> Ctrl+C (Copy)`
     `[SIMULATED KEYBOARD] Sending: CTRL + C (Copy)`
   - This lets you verify the matrix routing and command mappings before soldering!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Pressing one key triggers multiple characters | Missing diode or reversed diode | Diodes must point towards the row pin (Cathode/stripe connected to Row). |
| A key doesn't register at all | Bad solder joint or broken switch | Check continuity using a multimeter. Ensure row and column pins are mapped correctly in the sketch. |
| Keys trigger twice on a single press | Switch debounce delay too short | The code has a 10ms debounce check, which is standard for Cherry MX switches. If using cheap tactile switches, increase the debounce delay in `scanKeyboardMatrix()` to `20ms`. |
| Compiles with USB errors | Compiling for Uno/Mega without using the auto-detection | Select the correct board in Arduino IDE (Tools > Board) or let the code use its fallback serial mode. |

## 🧠 Code Explanation

Let's break down the industrial technique used inside every mechanical keyboard:

### 1. Matrix Scanning (Saving I/O Pins)
- A 104-key mechanical keyboard doesn't use 104 pins on a microcontroller. 
- Switches are wired into a grid of Rows and Columns. For our 6-key macro pad, a 2x3 matrix means we only need 5 microcontroller pins!
- The Arduino sets all columns to `INPUT_PULLUP`. It then pulls Row 0 `LOW`. It reads the 3 columns. If Col 2 reads `LOW`, we instantly know the key at Matrix(0, 2) is pressed! It then pulls Row 1 `LOW` and repeats. This happens thousands of times a second.

### 2. The Ghosting Problem and Signal Diodes
- **Ghosting** happens when 3 keys sharing rows/columns are pressed simultaneously. Current flows backwards through the closed switches, creating a "sneak path" that tricks the microcontroller into thinking a 4th key is pressed.
- By placing a tiny signal diode (like a 1N4148) on one leg of every single switch, we force the electricity to act as a one-way valve. The sneak path is blocked, granting the keyboard **N-Key Rollover (NKRO)**—the ability to press every key at once perfectly!

### 3. Firing Complex System Hotkeys
```cpp
Keyboard.press(KEY_LEFT_CTRL);
Keyboard.press('c');
delay(20);
Keyboard.releaseAll();
```
- A macro is just a rapid sequence of keypresses. We instruct the USB HID controller to hold down the modifier (`CTRL`), tap the payload key (`C`), and critically, we call `releaseAll()`. 
- If you forget `releaseAll()`, the computer thinks you are permanently holding down the `CTRL` key, which makes using the PC impossible until you reboot!

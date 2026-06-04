# Day 93: Sim Racing Pedals & Shifter Controller (Deadzones & Dynamic Calibration)

Welcome to Day 93! Today we build the mechatronic interface logic for custom **Sim Racing Pedals** and a **Sequential Shifter**. We will learn how to handle noisy, drifting analog sensors (potentiometers, Hall sensors, and load cells) by implementing **inner and outer deadzones**, designing an **on-board auto-calibration system**, and mapping inputs to standard USB Game Controller axes.

---

## 🎯 The "Why" and "What"

In sim racing (driving simulators), precise pedal inputs are the difference between a clean lap and spinning out.
- **Throttle**: Needs linear travel control to modulate acceleration.
- **Brake**: Needs pressure-based control (usually a load cell measuring force) to avoid locking up tires.
- **Clutch**: Needs positional control.

Because mechanical assemblies wear down, springs stretch, and temperatures affect sensor resistance, the minimum and maximum analog voltages from your pedals will drift over time.
Additionally, when you rest your foot on a pedal, you don't want it to register as a 1% brake drag. Conversely, when you press the throttle fully, you want to guarantee 100% activation without needing to crush the pedal.

To solve this, we implement **Software Deadzones** and **Dynamic Calibration**:
1. **Lower Deadzone**: Ignores initial minor sensor readings (resting foot).
2. **Upper Deadzone**: Caps the reading to 100% early, allowing for full input even if travel is mechanically restricted.
3. **Dynamic Calibration**: Automatically updates the minimum and maximum boundaries based on the actual physical travel limits recorded during use.

---

## 🔬 Physics & Hardware Theory

### 1. Pedal Sensors: Potentiometer vs. Load Cell
- **Potentiometer (Displacement)**: A resistive divider. Turning the pedal shaft moves a wiper along a carbon track, changing resistance. The voltage output is linear with angular displacement:
  $$V_{\text{out}} = V_{\text{cc}} \cdot \frac{\theta}{\theta_{\text{max}}}$$
- **Load Cell (Force)**: A metal bar with **strain gauges** glued to it. When stepped on, the metal bends by micrometers, changing the strain gauges' resistance. This is arranged in a **Wheatstone Bridge** circuit and amplified by an IC (like the `HX711` or an operational amplifier) to output a voltage relative to pressure.

### 2. Deadzone Math & Mapping
Standard Arduino analog inputs are 10-bit ($0$ to $1023$ LSB). Let's define:
- $X_{\text{raw}}$: Current raw sensor reading.
- $X_{\text{min}}, X_{\text{max}}$: Calibrated minimum and maximum raw limits.
- $D_{\text{lower}}, D_{\text{upper}}$: Deadzone percentages (e.g. $D_{\text{lower}} = 0.05$ or $5\%$, $D_{\text{upper}} = 0.95$ or $95\%$).

The active span is:
$$\text{Span} = X_{\text{max}} - X_{\text{min}}$$

The thresholds are calculated as:
$$T_{\text{lower}} = X_{\text{min}} + (\text{Span} \cdot D_{\text{lower}})$$
$$T_{\text{upper}} = X_{\text{min}} + (\text{Span} \cdot D_{\text{upper}})$$

We constrain the raw input to these thresholds and apply **Linear Interpolation** to map it to the game controller's axis range ($0$ to $1023$):
- If $X_{\text{raw}} \le T_{\text{lower}}$: Output = $0$
- If $X_{\text{raw}} \ge T_{\text{upper}}$: Output = $1023$
- Else:
  $$\text{Output} = \frac{X_{\text{raw}} - T_{\text{lower}}}{T_{\text{upper}} - T_{\text{lower}}} \cdot 1023$$

```
Raw Voltage:   0V ─────────┬───────────────────────────────┬───────── 5V
                           │◄────── Active Range ─────────►│
                       Lower Deadzone                  Upper Deadzone
                          (e.g., 5%)                      (e.g., 95%)
Output Value:  0 ──────────┼───────────────────────────────┼───────── 1023
```

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Leonardo or Micro | 1 | Native USB game controller emulation (ATmega32U4) |
| Rotary Potentiometers (10k) | 3 | Simulates throttle, brake, clutch pedal sensors |
| Pushbutton Switches | 2 | Sequential Shifter up/down switches |
| Breadboard & Jumper Wires | 1 | Prototyping and wiring |
| Micro-USB Cable | 1 | Connects board to PC |

---

## 🔌 Pin-to-Pin Wiring

| Potentiometer/Switch | Arduino Pin | Description |
| :--- | :--- | :--- |
| **Throttle Wiper** | **A0** | Analog Throttle Input |
| **Brake Wiper** | **A1** | Analog Brake Input (or Load Cell Amp output) |
| **Clutch Wiper** | **A2** | Analog Clutch Input |
| **Shift Up Switch** | **D3** | Shift Up Trigger (internal pullup) |
| **Shift Down Switch** | **D4** | Shift Down Trigger (internal pullup) |
| **Common GND** | **GND** | Ground rails |
| **Common VCC** | **5V** | 5V Power rails |

---

## 💾 Alternatives to Custom DIY Interfaces

| Method | Resolution | Axis Count | PC Recognition | Custom Deadzones | Cost |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **DIY Arduino (Our project)** | **10-bit (1024 steps)** | **Up to 6 axes** | **Direct USB Gamepad** | **Yes (Software configured)** | **Very Low** |
| **Leo Bodnar Board** | 12-bit or 16-bit | 8 axes | Plug & Play | No (Requires PC tool) | High |
| **Commercial USB Adapters** | 10-bit | 3 axes | Plug & Play | No | Moderate |
| **Teensy (LC / 4.0)** | 12-bit (4096 steps) | Up to 10 axes | Direct USB Gamepad | Yes | Low |

---

## 💻 How to Test & Validate

1. Wire three potentiometers to A0, A1, A2, and two pushbuttons to Pins 3 and 4.
2. Open the Arduino IDE, load [Day_93_Sim_Racing_Pedals.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_93_Sim_Racing_Pedals/Day_93_Sim_Racing_Pedals.ino).
3. **If using native USB (Leonardo/Micro)**:
   - Select **Arduino Leonardo** in Board Settings.
   - Go to Sketch > Include Library > Manage Libraries, search for **Joystick** (by MHeironimus), and install it.
   - Upload the sketch.
   - Run Windows' built-in game controller calibration tool (`joy.cpl`).
   - Turn the potentiometers and watch the Throttle, Brake, and Clutch bars sweep smoothly!
   - Press the shifter buttons to watch buttons 1 and 2 activate.
4. **If using Arduino Uno (Simulation Mode)**:
   - Select **Arduino Uno** and upload the sketch.
   - Open **Tools > Serial Plotter** at **115200 Baud**.
   - You will see the live graphing of raw inputs vs processed outputs.
   - Close the plotter and open the **Serial Monitor** at **115200 Baud**.
   - **Auto-Calibration Demonstration**:
     - Send `c`. Auto-calibration starts.
     - Send `t` and `b` commands multiple times to simulate depressing throttle/brake pedals fully.
     - Send `c` again to stop calibration and save limits.
     - Observe how the limits have expanded and the output scaling adapts dynamically!
     - Send `u` to simulate a gear shift UP.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Throttle/Brake floats randomly when not touched | Floating input pin | Ensure potentiometers are connected properly to VCC and GND. If a pin is fully disconnected, it will pick up electromagnetic noise. |
| Pedals do not reach 100% (1023) or 0% in-game | Uncalibrated sensor limits | Run calibration mode (`c` in CLI) and press each pedal through its complete range of motion. |
| Inverted pedal axis (e.g. 100% output when released) | Potentiometer wired backwards | Swap the VCC and GND wires on that specific pedal's potentiometer, or modify the mapping calculation in the code: `1023 - processPedalValue()`. |
| Shift buttons do not register | Floating digital pin | The code uses `INPUT_PULLUP`. Ensure the switch connects the pin to **GND** when pressed. |

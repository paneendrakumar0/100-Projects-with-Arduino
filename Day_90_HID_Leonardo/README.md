# Day 90: Arduino as a USB HID (Keyboard/Mouse Emulation using ATmega32U4)

Welcome to Day 90! Today we transform our microcontroller into a native **USB Human Interface Device (HID)**, allowing it to control a computer directly as a hardware Keyboard and Mouse. We will explore how native USB hardware controllers function, write automated typing macros and shortcut commands, and build a "Mouse Jiggler" to keep the PC awake. 

---

## 🎯 The "Why" and "What"

In mechatronics and human-computer interaction (HCI), we often want custom hardware to talk to a PC without writing a custom PC-side driver. For example:
- A custom **Sim Racing Steering Wheel** or pedal set.
- An **industrial macro control box** with physical buttons to launch scripts or control CAD software.
- A **foot switch** to control presentation slides.

By configuring the microcontroller to report itself as a **USB HID Device**, the PC automatically loads its built-in keyboard/mouse drivers. The computer cannot distinguish between your Arduino code and a standard $10 USB keyboard.

---

## 🔬 Physics & Hardware Theory

### 1. Native USB vs. USB-to-Serial Chips
- **Arduino Uno (ATmega328P)**: The main microcontroller does not have hardware USB circuitry. It communicates via a secondary serial chip (like the `CH340G` or `ATmega16U2`) which acts as a bridge. The PC only sees it as a **Virtual COM Port**.
- **Arduino Leonardo / Micro (ATmega32U4)**: The main microcontroller has a built-in hardware USB transceiver (D+ and D- lines connected directly to pins). This allows the chip to dynamically masquerade as a keyboard, mouse, MIDI device, or joystick.

```
Arduino Uno (Serial Only):
┌────────────┐        ┌──────────────┐        ┌────┐
│ ATmega328P │ ──TTL──►│ USB-to-Serial│ ──USB──►│ PC │ (Virtual COM Port)
└────────────┘        └──────────────┘        └────┘

Arduino Leonardo (Native USB):
┌────────────┐
│ ATmega32U4 │ ───────────────────────────USB─────────►│ PC │ (Keyboard/Mouse/COM)
└────────────┘
```

### 2. USB Descriptors & Reports
When a USB device is plugged in, it sends **USB Descriptors** (arrays of bytes) to the PC during the "enumeration" phase. These descriptors define what type of device it is (Keyboard, Mouse, Gamepad) and what size of packets (Reports) it will send:
- **Keyboard Report**: An 8-byte buffer. Byte 0 is modifier keys (Shift, Ctrl, Alt, GUI). Byte 1 is reserved. Bytes 2-7 contain up to 6 simultaneous key presses (which is why cheap USB keyboards are "6-key rollover").
- **Mouse Report**: A 4-byte buffer containing state of buttons (left, right, middle click) and relative movements ($\Delta x, \Delta y, \text{wheel}$).

### 3. The Crucial safety rule: Lockout Delay
If you write a loop that constantly presses `Enter` or sends keystrokes, you will spam the computer so fast that you won't be able to open the Arduino IDE to upload a fix. 
**Solution**: Always insert a **5-second delay** in `setup()` before starting any HID commands. This gives you time to hit the reset button and upload new code if the device goes rogue.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Leonardo or Micro | 1 | Microcontroller with native USB (ATmega32U4) |
| Micro-USB Jumper Cable | 1 | Connects board to PC |
| Pushbutton | 2 | Action triggers (Text Typer and Jiggler Toggle) |
| Jumper Wires & Breadboard | 1 | Prototyping |

---

## 🔌 Pin-to-Pin Wiring

| Component Pin | Arduino Pin | Description |
| :--- | :--- | :--- |
| **Button A Pin 1** | **GND** | Ground reference |
| **Button A Pin 2** | **D2** | Keyboard Typer (internal pull-up enabled) |
| **Button B Pin 1** | **GND** | Ground reference |
| **Button B Pin 2** | **D3** | Mouse Jiggler Toggle (internal pull-up enabled) |

---

## 💾 Alternatives to ATmega32U4 Boards

| Method | Native USB | Ease of Use | Cost | Hardware Required | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **ATmega32U4** | **Yes** | **Very Easy** | Low | Leonardo / Pro Micro | Standard Arduino `Keyboard.h` library works out-of-the-box. |
| **V-USB** | Software-emulated | Hard | Tiny | Arduino Uno + Zener Diodes | Bit-bangs USB over GPIO pins. Low speed, highly timing sensitive. |
| **HoodLoader2** | Yes (Indirect) | Hard | Low | Arduino Uno (ATmega16U2) | Reflashes the Uno's onboard USB-to-Serial co-processor. |
| **ESP32 (BLE HID)** | Yes (Bluetooth) | Easy | Moderate | ESP32 Board | Simulates keyboard/mouse wirelessly over Bluetooth Low Energy. |

---

## 💻 How to Test & Validate

### If using Arduino Leonardo / Micro (Native USB):
1. Wire up the two pushbuttons to Pins 2 and 3.
2. Open the Arduino IDE, load [Day_90_HID_Leonardo.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_90_HID_Leonardo/Day_90_HID_Leonardo.ino), select board **Arduino Leonardo** (or Micro), and upload the code.
3. Open a text editor (e.g. Notepad) and click inside it to focus.
4. Press **Button A** (Pin 2). The Arduino will type:
   `Hello from Day 90 of Arduino 100-Day Masterclass!`
5. Press **Button B** (Pin 3). The onboard LED will light up, indicating the **Mouse Jiggler** is active. Every 5 seconds, the cursor will jiggle slightly.
6. Open the **Serial Monitor** at **9600 Baud** to send commands manually:
   - Send `l` to lock the PC.
   - Send `a` to toggle between applications (Alt+Tab).

### If using Arduino Uno / Nano (Simulation Mode):
1. The code automatically detects that it is running on an Uno/Mega and compiles in **Simulation Mode** (bypassing USB errors).
2. Connect your Uno and open the **Serial Monitor** at **9600 Baud**.
3. Send `t`, `j`, `l`, or `a`. 
4. The Serial Monitor will print out the simulated USB HID packets, showing you exactly what key codes or mouse movements a Leonardo would have transmitted!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Sketch compiles with error: `Keyboard.h: No such file or directory` | Compiling for Uno/Nano without simulation wrapper | Our sketch includes a wrapper to prevent this, but make sure you select **Arduino Leonardo** or **Micro** in Tools > Board. |
| Button triggers continuously | Missing pull-up resistor | The code uses `INPUT_PULLUP` to set the pin HIGH by default. Make sure the button is wired to connect the pin to **GND** when pressed. |
| Roguish keystrokes locking out the PC | Infinite loop in HID transmission | Unplug the USB cable immediately. Hold down the physical **Reset Button** on the Arduino, plug the USB cable back in, click "Upload" in the IDE, and release the Reset button only when the IDE status changes to "Uploading...". |

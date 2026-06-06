# Day 91: Custom PC Volume Knob (Rotary Encoder + USB HID Media Controller)

Welcome to Day 91! Today we combine physical mechatronics with native USB interfaces to build a **Custom PC Volume Knob**. We will interface a **Quadrature Rotary Encoder** (with built-in push button) to command Windows/macOS/Linux system audio over USB. We will study the physics of quadrature waveforms, design a robust software decoder, and examine the **USB HID Consumer Control Page** layout.

---

## 🎯 The "Why" and "What"

User interfaces (UI) benefit heavily from dedicated physical inputs. In mechatronics, tactile feedback is superior to graphical controls: turning a physical aluminum volume dial is much faster and more satisfying than clicking a mouse slider on a screen.

### What is a Rotary Encoder?
Unlike an analog potentiometer (which is limited to ~300 degrees of rotation and outputs an absolute analog voltage), a **Rotary Encoder** has **infinite rotation** in both directions. It outputs digital pulses (quadrature signals) that allow the microcontroller to measure relative angle steps and direction.

To interface this knob with a computer, we use the USB **Consumer Control Page**. Unlike standard alphanumeric keys (A, B, C, Enter), volume and playback controls are located on a separate USB Usage Page. This enables the operating system to intercept them globally, regardless of which window is in focus.

---

## 🔬 Physics & Hardware Theory

### 1. Quadrature Phase Shift
A mechanical rotary encoder contains a disc with slots and two contact terminals (A and B) that drag against it. The pins are mechanically offset. As the disc turns, it opens and closes the circuits, generating two digital square waves that are **90 degrees out of phase**.

```
Clockwise (CW) Rotation:
  CLK (A)  ───┐   ┌───┐   ┌───┐
              └───┘   └───┘   └───
  DT (B)     ───┐   ┌───┐   ┌───┐
                └───┘   └───┘   └───
  (CLK transitions first. When CLK falls from HIGH to LOW, DT is still HIGH.)

Counter-Clockwise (CCW) Rotation:
  CLK (A)    ───┐   ┌───┐   ┌───┐
                └───┘   └───┘   └───
  DT (B)   ───┐   ┌───┐   ┌───┐
              └───┘   └───┘   └───
  (DT transitions first. When CLK falls from HIGH to LOW, DT is already LOW.)
```

By checking the logic state of **DT** at the moment **CLK** transitions from HIGH to LOW (a falling edge), we can determine the direction of rotation.

### 2. USB Usage Pages: Keyboard vs. Consumer
USB devices organize functions into **Usage Pages**:
- **Usage Page 0x07 (Keyboard/Keypad)**: Contains standard keys (Shift, A, Z, 1, F1).
- **Usage Page 0x0C (Consumer)**: Contains multimedia keys (Volume Up, Volume Down, Mute, Play/Pause, Next Track, Power, Calculator).

Using NicoHood's `HID-Project` library, we access the Consumer Page using the `Consumer` object:
```cpp
Consumer.write(MEDIA_VOLUME_UP);   // Increments system volume
Consumer.write(MEDIA_VOLUME_DOWN); // Decrements system volume
Consumer.write(MEDIA_VOLUME_MUTE); // Toggles mute state
```

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Leonardo or Micro | 1 | Microcontroller with native USB (ATmega32U4) |
| KY-040 Rotary Encoder Module | 1 | Quadrature rotary input with integrated push button |
| Breadboard & Jumper Wires | 1 | Prototyping and connections |
| Micro-USB Cable | 1 | USB connection to computer |

---

## 🔌 Pin-to-Pin Wiring

| KY-040 Encoder Pin | Arduino Uno/Leonardo Pin | Description |
| :--- | :--- | :--- |
| **GND** | **GND** | Ground reference |
| **+** (VCC) | **5V** or **Not Connected** | Power (not strictly needed if using internal pullups) |
| **CLK** (Signal A) | **D5** | Quadrature Channel A (with internal pullup) |
| **DT** (Signal B) | **D6** | Quadrature Channel B (with internal pullup) |
| **SW** (Button) | **D7** | Pushbutton contact (with internal pullup) |

---

## 💾 Alternatives to Rotary Encoders

| Method | Absolute/Relative | Rotation Range | PC Interface | Software Complexity | Tactile Feel |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Rotary Encoder (KY-040)** | **Relative** | **Infinite** | **USB Consumer Page** | **Low (State Polling)** | **Excellent (Clicky detents)** |
| **Analog Potentiometer** | Absolute | ~300 degrees | Serial + PC Mapper Agent | Medium | Good (Smooth damping) |
| **Pushbuttons (Up/Down)** | Relative | N/A | USB Keyboard or Consumer | Low | Moderate |
| **Infrared (IR) Receiver** | Relative | N/A | USB Keyboard or Consumer | High | Poor |

---

## 💻 How to Test & Validate

1. Wire the KY-040 Encoder to pins 5, 6, and 7 on your Arduino.
2. Open the Arduino IDE, load [Day_91_HID_Volume_Knob.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_91_HID_Volume_Knob/Day_91_HID_Volume_Knob.ino).
3. **If using native USB (Leonardo/Micro)**:
   - Select **Arduino Leonardo** in Board Settings.
   - Go to Sketch > Include Library > Manage Libraries, search for **HID-Project** (by NicoHood), and install it.
   - Upload the sketch.
   - Turn the encoder knob. You should see the **Windows/macOS volume overlay** appear and adjust up/down!
   - Press the encoder shaft down. The system audio should **Mute/Unmute** (indicated by the onboard LED turning on/off).
4. **If using standard Arduino Uno (Simulation Mode)**:
   - Upload the sketch directly.
   - Open the **Serial Monitor** at **9600 Baud**.
   - Turn the knob or press the button.
   - The monitor will print:
     `[VOLUME] Knob Rotated: CW -> Volume UP`
     `[SIMULATED USB HID] Sent: Consumer Key Media Volume Up`
   - You can also send `+`, `-`, or `m` via the serial input to simulate encoder inputs!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Turning the knob volume moves 2 steps up and then 1 down (Jitter) | Contact bounces or noisy transitions | Keep the encoder contacts clean, and ensure the polling rate in `loop()` is fast (avoid using `delay()` anywhere in the loop). |
| Volume goes down when turning clockwise | Channel A and Channel B are swapped | Swap the wires on pins **D5** (CLK) and **D6** (DT), or swap their assignments in the sketch code. |
| Button doesn't trigger mute | Loose wire on SW pin | Verify the SW pin goes to **D7** and the GND pin of the encoder goes to Arduino GND. |
| Compiles with error: `HID-Project.h: No such file or directory` | Missing library dependency | Open the Library Manager in the Arduino IDE, search for **HID-Project**, and click install. If you don't have it, the code will automatically fall back to keyboard shortcuts (`Ctrl+Up` / `Ctrl+Down`). |

## 🧠 Code Explanation

Let's break down how we decode a physical dial into perfect digital signals:

### 1. The Physics of Quadrature Encoders
- Unlike a standard potentiometer that gives an absolute analog voltage, a Rotary Encoder gives relative digital steps. 
- Inside the dial are two physical metal contact switches, mechanically offset from one another. 
- As you turn the dial, they generate two square waves (Channel A and Channel B) that are exactly 90 degrees out of phase.

### 2. State-Machine Decoding
```cpp
if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
  if (digitalRead(ENCODER_PIN_B) != currentStateCLK) {
    volumeUp();
  } else {
    volumeDown();
  }
}
```
- By monitoring Channel A (CLK) for a falling edge, we know the dial just clicked one physical "detent".
- In that exact microsecond, we look at the state of Channel B (DT). If Channel A transitioned *before* Channel B, the dial is turning Clockwise. If Channel B transitioned *before* Channel A, it's turning Counter-Clockwise. It's a mathematically flawless way to detect infinite rotation!

### 3. Native USB HID Consumer Control
```cpp
Consumer.write(MEDIA_VOLUME_UP);
```
- The ATmega32U4 chip (Leonardo/Micro) features a hardware USB transceiver. 
- Instead of relying on buggy PC-side software to read Serial strings, the Arduino physically enumerates to the operating system as a native USB Multimedia Keyboard. 
- Sending the `MEDIA_VOLUME_UP` hex code triggers the operating system's built-in volume control directly, bypassing all custom drivers!

# Day 28: Infrared (IR) Remote Control Decoder (NEC Protocol)

Welcome to Day 28 of the 100-Day Arduino Masterclass! Today, we explore wireless communication by interfacing a 38 kHz Infrared (IR) receiver to capture and decode signals from common handheld remotes. We will program a non-blocking wireless state toggle system using the modern `IRremote` library (v4.x) and learn how to extract command codes from unknown remotes.

---

## 🎯 The "Why" and "What"

Infrared communication is a staple of short-range, line-of-sight wireless control in consumer electronics (TVs, air conditioners) and robotics. In robotics, IR remotes are used to:
1. Trigger autonomous modes (e.g., Line-Following vs Obstacle Avoidance).
2. Override system controls manually.
3. Transmit state parameters wirelessly over a short distance.

By using an active IR receiver (like the TSOP38238/VS1838B), we can convert raw optical pulses into digital hexadecimal codes, enabling multi-channel remote execution.

---

## 🔬 Physics & Hardware Theory

### 1. Carrier Modulation (38 kHz Frequency Gating)
The air is filled with ambient infrared radiation from sunlight, heaters, and home lighting. If an IR receiver simply read raw IR light, this background noise would cause constant false signals.
To solve this, IR remotes transmit data using a **$38\text{ kHz}$ carrier frequency**. 
* The remote's IR LED flashes on and off exactly $38,000$ times per second to represent a logic high.
* The IR receiver module contains an internal band-pass filter that blocks all light except light pulsing near $38\text{ kHz}$. It then demodulates this carrier, pulling its output pin **LOW** when a $38\text{ kHz}$ signal is present, and **HIGH** (via internal pull-up) when it is absent.

```
Remote:    Pulse [|||||]   Space [     ]   Pulse [|||||]
            (38kHz LED)                      (38kHz LED)
Receiver:   Output LOW      Output HIGH      Output LOW
```

---

### 2. The NEC Transmission Protocol
The most common coding protocol in hobbyist electronics is the **NEC Protocol**. It uses **Pulse-Distance Modulation** to represent bits:
* **Start Frame:** A $9\text{ ms}$ burst of carrier signal followed by a $4.5\text{ ms}$ space. This alerts the receiver to prepare for data.
* **Logic '0':** A $562.5\text{ µs}$ pulse burst followed by a $562.5\text{ µs}$ space (total time $1.125\text{ ms}$).
* **Logic '1':** A $562.5\text{ µs}$ pulse burst followed by a $1.6875\text{ ms}$ space (total time $2.25\text{ ms}$).
* **Repeat Code:** If a button is held down, the remote sends a $9\text{ ms}$ pulse, a $2.25\text{ ms}$ space, and a $562.5\text{ µs}$ pulse. This tells the microcontroller to ignore repeated clicks or implement acceleration.

A standard transmission contains **32 bits**:
$$\text{Frame Structure} = [8\text{-bit Address}] + [8\text{-bit Inverted Address}] + [8\text{-bit Command}] + [8\text{-bit Inverted Command}]$$

---

## 🔄 Alternatives Comparison

When selecting wireless interfaces for robotics, developers select based on range, data bandwidth, and line-of-sight constraints:

| Protocol / Module | Frequency | Range | Line of Sight? | Power Consumption | Complexity | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Infrared (VS1838B)** | **Infrared Light** | **$5\text{m} - 10\text{m}$** | **Yes (Strict)** | **Very Low ($<5\text{mA}$)** | **Low** | **Simple commands, short-range controllers (Our choice)** |
| **RF (HC-12 / NRF24)** | **$433\text{MHz} / 2.4\text{GHz}$** | **$100\text{m} - 1\text{km}$** | **No** | **Medium ($15\text{mA} - 100\text{mA}$)** | **Medium** | **Long-range telemetry, RC cars, sensor nodes** |
| **Bluetooth (HC-05)** | **$2.4\text{ GHz}$** | **$10\text{m}$** | **No** | **Medium ($\sim 40\text{mA}$)** | **Medium** | **Smartphone-to-robot telemetry, configuration interfaces** |
| **Wi-Fi (ESP8266/ESP32)** | **$2.4\text{ GHz}$** | **$50\text{m} - 100\text{m}$** | **No** | **High ($80\text{mA} - 250\text{mA}$)** | **High** | **IoT logging, web dashboards, video streaming** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x VS1838B or TSOP38238 Infrared Receiver (38 kHz)
* 1x Handheld IR Remote (e.g., standard Chinese kit remote or home TV remote)
* 3x LEDs (Red, Green, Blue)
* 3x 220 Ohm current-limiting resistors
* 1x Breadboard
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

### 1. IR Receiver Sensor Pins
Ensure you locate the correct pinout for your module. Standard VS1838B units looking at the rounded bulb face (left to right):

| IR Receiver Pin | Arduino Uno Pin | Wire Color (Recommended) | Description |
| :--- | :--- | :--- | :--- |
| **Pin 1 (OUT)** | **D11** | Green | Demodulated Signal Output |
| **Pin 2 (GND)** | **GND** | Black | Ground |
| **Pin 3 (VCC)** | **5V** | Red | Power Input ($5\text{V}$ or $3.3\text{V}$) |

### 2. LED Outputs
| LED Indicator | Arduino Pin | Resistor | Description |
| :--- | :--- | :--- | :--- |
| **Red LED Anode** | **D5** | 220 Ohm to GND | Red Subsystem Toggle |
| **Green LED Anode** | **D6** | 220 Ohm to GND | Green Subsystem Toggle |
| **Blue LED Anode** | **D7** | 220 Ohm to GND | Blue Subsystem Toggle |
| **LED Cathodes** | **GND** | Direct | Ground rail |

---

## 💻 How to Test & Validate

1. Connect the components as detailed in the wiring diagrams.
2. Install the **IRremote** library by Armin Joachimsmeyer (version **4.x**) via the Library Manager.
3. Open `Day_28_IR_Decoder.ino` and upload it to your board.
4. Open the **Serial Monitor** at **9600 Baud**.
5. Aim your remote control at the IR receiver and press **Button 1**:
   * The Red LED will toggle state (ON $\leftrightarrow$ OFF).
   * The Serial Monitor will print: `[SYSTEM] Toggled RED LED -> ON`.
6. Press **Button 2** (toggles Green LED) and **Button 3** (toggles Blue LED).
7. Press the **Power Button**: All active LEDs will immediately shut down (E-Stop).
8. **Calibration:** If your remote uses different key codes, watch the Serial Monitor:
   * Press your desired button and locate the `Command Code: 0xXX` value in the console.
   * Edit the code constants (`NEC_KEY_1`, etc.) with your decoded hex codes, upload, and test!

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **No signal decoded / Nothing in Serial Monitor:**
  * Verify your receiver module pinout. Reversing VCC and GND will cause the chip to heat up rapidly and can destroy it.
  * Check the batteries in the IR remote. Test the remote with your phone's camera (aim the remote at the camera lens and press a button; you should see a faint purple flash on the screen if the IR LED is firing).
* **Serial output shows `[WARNING] Corrupted signal received` consistently:**
  * Ensure the remote is within line-of-sight and within range ($<5\text{m}$).
  * Fluorescent bulbs or direct sunlight can interfere with the sensor. Block external light sources.
* **The code fails to compile:**
  * Ensure you have installed version 4.x of the `IRremote` library. Older versions do not support the `IrReceiver` class object or `.decode()` structure.

## 🧠 Code Explanation

Let's break down how to decode invisible infrared light:

### 1. The NEC Protocol
```cpp
#include <IRremote.hpp>
IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
```
- When you press a button on a remote, the IR LED flashes at exactly 38,000 times a second (38 kHz). This specific frequency cuts through the infrared noise of the sun and house lights.
- The flash pattern spells out a 32-bit binary code (the NEC Protocol).
- The `IrReceiver` object monitors the digital pin in the background using hardware interrupts. It captures the exact microsecond lengths of the flashes and decodes them into a clean Hexadecimal number for us.

### 2. Processing Commands
```cpp
if (IrReceiver.decode()) {
    bool isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT);
    
    if (!isRepeat) {
        processIRCommand(IrReceiver.decodedIRData.command);
    }
    IrReceiver.resume(); 
}
```
- `IrReceiver.decode()` returns `true` if a full, valid IR packet was captured.
- We check for a "Repeat" flag. If you hold a button down, the remote sends a special "Repeat" code. We ignore it so that our LED doesn't rapidly toggle on and off while the button is held!
- Finally, `IrReceiver.resume()` resets the sensor so it can catch the next incoming flash.

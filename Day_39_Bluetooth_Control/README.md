# Day 39: HC-05 Bluetooth Module (UART Control)

Welcome to Day 39 of the 100-Day Arduino Masterclass! Today, we study wireless remote control systems. We will interface the popular **HC-05 Bluetooth Classic Module** using standard **UART serial communication**. We will write a SoftwareSerial interface, build a packet-parsing character buffer with end-of-packet terminators, and learn how to construct a voltage divider to protect 3.3V logic inputs.

---

## 🎯 The "Why" and "What"

Robotics systems require wireless telemetry and manual control.
* **The Problem:** The standard Arduino is tied to the computer via a USB cable. Polling controls over USB prevents the robot from driving around untethered.
* **The Solution:** The **HC-05 Bluetooth Module** behaves as a **wireless serial bridge (SPP - Serial Port Profile)**. It establishes a wireless connection to a smartphone or computer, transparently passing serial ASCII text commands over the air to the Arduino.

We will write a non-blocking parser that receives commands (e.g. `F;` for Forward, `S;` for Stop) to control simulated robot chassis motor outputs.

---

## 🔬 Physics & Communication Theory

### 1. UART Serial Communication (Asynchronous Bus)
UART (Universal Asynchronous Receiver-Transmitter) is a simple point-to-point serial communication protocol that does not share a clock line.
* **Synchronization:** Because there is no clock line, both devices must agree beforehand on the transmission speed (the **Baud Rate**, e.g., 9600 bits per second).
* **Frame Format:** Data is grouped into frames. A standard frame contains:
  * **Start Bit (Logic LOW):** Signals the receiver that a byte is arriving.
  * **8 Data Bits:** The actual byte payload (representing an ASCII character).
  * **Stop Bit (Logic HIGH):** Resets the line for the next byte.

```
Idle (HIGH)  ──┐   ┌───DATA BITS (8 bits)───┐   ┌─── Idle (HIGH)
               └───┘                        └───┘
             Start Bit                     Stop Bit
```

---

### 2. SoftwareSerial Emulation
The Arduino Uno has only one hardware UART port (connected to pins 0 and 1, which route through the USB chip to your PC). 
* To preserve pins 0 and 1 for compiling, uploading, and debug printing to the Arduino Serial Monitor, we use **SoftwareSerial**.
* SoftwareSerial uses software interrupts to emulate UART timing, allowing us to turn normal digital pins (Pin 10 and 11) into TX/RX serial ports.

---

### 3. Logic Level Translation (Resistor Voltage Divider)
The Arduino Uno runs on **5V logic** (logic HIGH = 5V). The HC-05 chip runs strictly on **3.3V logic** (logic HIGH = 3.3V).
* **Arduino RX (Pin 10):** The HC-05 outputs 3.3V. The Arduino recognizes any voltage above 3.0V as logic HIGH, so connecting HC-05 TXD directly to Arduino Pin 10 is safe.
* **HC-05 RX (TX Pin 11):** The Arduino outputs 5V. Supplying 5V directly to the HC-05 RXD pin will overstress and eventually burn out the Bluetooth module.
* **The Solution:** We construct a **Voltage Divider** using a $1\text{ k}\Omega$ and a $2\text{ k}\Omega$ resistor to step the 5V signal down to 3.3V:

```
  Arduino TX Pin 11 ─────[ 1kΩ Resistor ]─────┬─────► HC-05 RXD Pin
                                              │
                                        [ 2kΩ Resistor ]
                                              │
                                             GND
```

$$\text{Voltage Out} = 5\text{V} \times \left( \frac{2\text{ k}\Omega}{1\text{ k}\Omega + 2\text{ k}\Omega} \right) = 3.33\text{V}$$

---

## 🔄 Alternatives Comparison

When selecting wireless protocols for mechatronics:

| Communication Module | Range | Power Consumption | Bandwidth | Protocol / Setup | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **HC-05 Bluetooth Classic** | **$10\text{m}$** | **Medium ($30\text{mA}$)** | **Medium (SPP)** | **Very Low (Standard Serial)**| **Untethered robot controls, basic smartphone interfaces (Our choice)** |
| **HM-10 (BLE)** | **$10\text{m} - 50\text{m}$** | **Extremely Low ($<5\text{mA}$)** | **Low** | **Medium (GATT services)** | **Battery-sensitive IoT, iOS app connections** |
| **nRF24L01+ RF** | **$100\text{m} - 1\text{km}$**| **Medium ($15\text{mA}$)** | **High (Custom packets)**| **High (SPI registers)** | **RC transmitters/receivers, drone telemetry** |
| **ESP8266 Wi-Fi** | **$50\text{m} - 100\text{m}$**| **High ($100\text{mA}-250\text{mA}$)**| **Very High** | **Very High (TCP/IP stacks)** | **Web interfaces, video feeds, massive data streams** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x HC-05 Bluetooth Module
* 1x $1\text{ k}\Omega$ resistor
* 1x $2\text{ k}\Omega$ resistor (or $2.2\text{ k}\Omega$)
* 4x LEDs (simulating Left Forward, Left Reverse, Right Forward, Right Reverse motors)
* 4x 220 Ohm current-limiting resistors
* 1x Breadboard
* Jumper wires
* An Android Smartphone or PC with Bluetooth capability

---

## 🔌 Pin-to-Pin Wiring

Ensure the RX/TX lines cross correctly:
* Arduino TX (Pin 11) goes to HC-05 RX (via the voltage divider).
* Arduino RX (Pin 10) goes directly to HC-05 TX.

| HC-05 Pin | Arduino Uno Pin | Wire Connection Details | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **5V** | Direct | Module power |
| **GND** | **GND** | Direct | Common Ground reference |
| **TXD** | **D10** | Direct | HC-05 Transmit to Arduino Software RX |
| **RXD** | **D11** | **Through 1kΩ / 2kΩ Voltage Divider!** | Arduino Software TX to HC-05 Receive |

| Motor Simulator LED | Arduino Pin | Resistor | Description |
| :--- | :--- | :--- | :--- |
| **L_FWD Anode** | **D5** | 220 Ohm to GND | Left Motor Forward Indicator |
| **L_REV Anode** | **D6** | 220 Ohm to GND | Left Motor Reverse Indicator |
| **R_FWD Anode** | **D7** | 220 Ohm to GND | Right Motor Forward Indicator |
| **R_REV Anode** | **D8** | 220 Ohm to GND | Right Motor Reverse Indicator |
| **LED Cathodes** | **GND** | Direct | Common Ground |

---

## 💻 How to Test & Validate

1. Wire up the HC-05 module and the simulator LEDs on the breadboard. **Double-check the resistor voltage divider on the HC-05 RXD pin!**
2. Upload `Day_39_Bluetooth_Control.ino` to the Arduino.
3. Open the **Serial Monitor** on your PC at **9600 Baud**.
4. **Bluetooth Pairing & Terminal Setup:**
   * Turn on Bluetooth on your Android phone. Search for devices.
   * Select **HC-05** and pair using the default password: **`1234`** (or `0000`).
   * Download a free Bluetooth Serial Terminal app from the Google Play Store (e.g. "Serial Bluetooth Terminal" by Kai Morich).
   * Open the app, select "Devices" in the sidebar, and connect to **HC-05**.
5. **Send Commands:**
   * In the phone terminal input bar, type **`F;`** and press Send:
     * The L_FWD and R_FWD LEDs will turn ON.
     * The phone screen will display confirmation: `SUCCESS: F;`.
     * The PC Serial Monitor will print: `[BLUETOOTH CMD] Received: 'F' -> ACTION: Move Forward`.
   * Type **`L;`** (Spin Left) and verify the L_REV and R_FWD LEDs turn ON.
   * Type **`S;`** (Stop) to turn all LEDs OFF.
6. Type an invalid command (e.g. `HELLO;`):
   * The phone terminal will display: `ERROR: Unknown Command;`.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The HC-05 module status LED flashes rapidly, and the terminal cannot connect:**
  * Rapid flashing ($\sim 5$ times per second) means the module is powered but unpaired/disconnected. Connect via your phone terminal app. Once connected, the LED will flash slowly (double pulses every 2 seconds).
* **The serial monitor displays gibberish or nothing when commands are sent:**
  * Ensure the phone terminal app and the Arduino code are both configured for **9600 Baud**.
  * Check that your TX/RX wires are crossed correctly. Remember: Arduino Software RX (10) connects to HC-05 TXD, and Arduino Software TX (11) connects to HC-05 RXD.
* **Commands are ignored unless the Arduino is rebooted:**
  * Ensure you are appending the semicolon packet terminator (`';'`) to the end of every command. The buffer parser filters incoming bytes and will not execute commands until it receives the `;` character.
* **The Bluetooth module is not visible in the phone's search list:**
  * Ensure VCC is connected to 5V and GND to Ground. Check if the red LED on the HC-05 board is glowing.
  * Apple iPhones do not support standard Bluetooth Classic (SPP) profile. If using an iOS device, you must use a Bluetooth Low Energy (BLE) module (like the HM-10) instead of the HC-05.

## 🧠 Code Explanation

Let's break down how we parse Bluetooth commands safely using a Buffer:

### 1. Software Serial
```cpp
#include <SoftwareSerial.h>
SoftwareSerial bluetoothSerial(10, 11);
```
- The Arduino Uno only has one hardware Serial port (Pins 0/1). If we wire the Bluetooth module to Pins 0/1, we can't upload code to the Arduino anymore!
- `SoftwareSerial` uses bit-banging magic to create a *virtual* serial port on Pins 10 and 11, allowing us to talk to the HC-05 while keeping the USB port free for PC debugging.

### 2. The Command Parser
```cpp
if (c == ';') {
    packetBuffer.toUpperCase();
    executeRobotCommand(packetBuffer);
    packetBuffer = ""; 
} else {
    packetBuffer += c;
}
```
- Serial data doesn't arrive all at once. The word "FORWARD" arrives one letter at a time, milliseconds apart.
- If we try to process instantly, we only see "F".
- Instead, we add characters to a `packetBuffer` string. We only trigger the motors when we see the `;` terminator character. This ensures our command packet is 100% complete and uncorrupted before the robot acts!

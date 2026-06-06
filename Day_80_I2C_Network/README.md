# Day 80: I2C Multi-Master/Slave Network (Wire Callback Architecture)

Welcome to Day 80! Today we construct an **Inter-Microcontroller Network** using the **I2C (Inter-Integrated Circuit) Bus**. We will write a unified codebase that can configure our Arduino as either an **I2C Master** or an **I2C Slave** using preprocessor macros. We will explore **open-drain bus physics**, implement **interrupt-driven slave callback ISRs**, and learn how to transmit multi-byte telemetry and control packets safely.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/CAN_Bus.jpg" alt="CAN Bus" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

As robots grow in complexity, a single microcontroller runs out of pins, processing speed, or hardware timers. The standard engineering solution is **distributed processing**:
1. **Sensor Node**: A dedicated Arduino reads multiple analog sensors, filters the noise (e.g. Day 75 Complementary Filter), and acts as an I2C slave.
2. **Actuation Node**: Another Arduino manages stepper acceleration profiles (e.g. Day 68) or PWM motor speeds.
3. **Master Controller**: The brain of the robot requests telemetry from the sensor nodes, executes control algorithms (like PID), and transmits velocity commands to the actuation nodes.

Interfacing multiple nodes requires a robust, addressable communication bus. I2C allows up to 127 devices to share just **two wires** (plus GND).

---

## 🔬 Physics & Hardware Theory

### 1. I2C Open-Drain Bus Physics
Unlike standard GPIO pins (which use push-pull output configurations to actively drive the line to 5V or GND), I2C pins (SDA and SCL) are **open-drain**.
- **Pull-Up Resistors**: The bus lines are connected to 5V via external pull-up resistors (typically $4.7\,\text{k}\Omega$).
- **Bus Dominance**: A device on the bus can only do two things:
  1. *Release the line*: The transistor opens, and the pull-up resistor pulls the line up to 5V (logical HIGH / Passive state).
  2. *Pull the line LOW*: The transistor closes, shorting the line to GND (logical LOW / Active state).
- **Collision Prevention**: Because no device ever drives the line to 5V, two devices transmitting opposite bits simultaneously will not cause a short circuit. The line simply stays LOW.

```
                  +5V
                   │
             [ 4.7kΩ Pullup ]
                   ├─── SDA / SCL Bus Line
                   ├─── Master Open-Drain Output
                   └─── Slave Open-Drain Output
```

### 2. Clock Synchronization & Start/Stop Conditions
- **Start Condition**: Master pulls SDA LOW while SCL remains HIGH.
- **Stop Condition**: Master releases SDA (lets it go HIGH) while SCL is HIGH.
- **Clock Sync**: SCL is driven by the Master. Data on SDA must remain stable while SCL is HIGH, and can only change when SCL is LOW.

### 3. Interrupt-Driven Callbacks vs. Polling
Slaves do not poll the bus. Listening in a loop would consume 100% of the CPU.
Instead, the Atmega328P has dedicated **I2C hardware registers** that monitor the bus. When a Start condition matching the Slave's address (`0x08`) is detected, a hardware interrupt halts the main CPU and triggers callback functions:
- **`onRequest()` ISR**: Triggered when the Master requests data. The Slave writes bytes to the bus buffer.
- **`onReceive()` ISR**: Triggered when the Master sends data. The Slave reads the incoming bytes.
- **`volatile` Variables**: Variables modified inside these ISRs must be declared `volatile` so the compiler does not optimize them away, and atomic blocks must disable interrupts briefly during writes in the main loop to prevent data corruption.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 2 | One Master Node, One Slave Node |
| Potentiometer (10kΩ) | 1 | Telemetry simulation (attached to Slave) |
| 4.7 kΩ Resistor | 2 | External I2C bus pull-up resistors |
| Breadboard & Wires | 1 | Bus connections |

---

## 🔌 Pin-to-Pin Wiring

| Master Uno Pin | Slave Uno Pin | Pull-up Resistor | Description |
| :--- | :--- | :--- | :--- |
| **GND** | **GND** | - | **Common Ground (Mandatory!)** |
| **5V** | **5V** | - | Common power (if sharing USB power) |
| **A4 (SDA)** | **A4 (SDA)** | Connect to 5V via 4.7kΩ | I2C Data Bus Line |
| **A5 (SCL)** | **A5 (SCL)** | Connect to 5V via 4.7kΩ | I2C Clock Bus Line |

---

## 💾 Communication Bus Alternatives

| Bus | Wire Count | Max Speed | Master/Slave | Max Range | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **I2C** | 2 (+GND) | 400 kbps | Multi-Master/Slave | ~2 meters | Open-drain. Addressing built-in. Great for onboard ICs. |
| **SPI** | 4 (+GND) | 10+ Mbps | Single-Master/Multi-Slave | ~3 meters | Push-pull. Extremely fast. Needs a CS wire per slave. |
| **UART** | 2 (+GND) | 115.2 kbps| Peer-to-Peer (point-to-point) | ~15 meters| Simple, asynchronous. No addressing. |
| **CAN Bus**| 2 (+GND) | 1 Mbps | Multi-Master (Arbitration) | ~1 km | Differential. Noise immune. Needs transceivers (MCP2515). |

---

## 💻 How to Test & Validate

1. Wire the two Arduino boards together as shown in the table. **Do not forget to connect the GND pins together!**
2. **Flash the Slave**:
   - Open [Day_80_I2C_Network.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_80_I2C_Network/Day_80_I2C_Network.ino).
   - Set `#define I2C_MODE 0` at line 44.
   - Connect the Slave Arduino, select its port, and click Upload.
3. **Flash the Master**:
   - Change line 44 to `#define I2C_MODE 1`.
   - Connect the Master Arduino, select its port, and click Upload.
4. **Read Output**:
   - Connect the **Master** to your PC and open the **Serial Monitor** at **9600 Baud**.
   - You will see the Master send periodic requests and print the responses:
     ```text
     [MASTER] Requesting 2 bytes from Slave 0x08...
     [MASTER] Received Slave Sensor: 512
     [MASTER] Transmitting LED state: HIGH
     ```
   - Watch the Slave board's onboard LED (Pin 13) blink on and off every second as commanded by the Master.
   - Rotate the potentiometer attached to the Slave's A0 pin; you will see the sensor value update dynamically on the Master's Serial Monitor!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Master reports transmission errors (Error code 2 or 4) | Shared GND missing or open I2C lines | You MUST connect the GND pins of both Arduinos together. Verify SDA (A4) and SCL (A5) connections. |
| Transmission hangs/freezes | Missing pull-up resistors | Long wires increase bus capacitance. Add external 4.7kΩ resistors on SCL and SDA lines to 5V. |
| Decoded data values are corrupted | Atomic access violation in Slave | When reading a multi-byte variable (16-bit integer) in the loop that is written by an ISR, you must temporarily disable interrupts (`noInterrupts()`) during the copy to prevent partial write updates. |
| Master prints `0` sensor value but is connected | Slave A0 pin is floating | Ensure the Slave potentiometer center pin is connected securely to A0. |

## 🧠 Code Explanation

Let's break down how we built a Master/Slave hardware network using I2C:

### 1. The Master's Role: Orchestration
```cpp
Wire.requestFrom(SLAVE_I2C_ADDR, 2);
byte high = Wire.read();
byte low  = Wire.read();
uint16_t val = (high << 8) | low;
```
- In an I2C network, the Slave is completely passive. The Master (Mode 1) dictates the rhythm of the entire bus.
- Every 1 second, the Master asserts the SCL clock line and requests exactly 2 bytes of data from address `0x08`. It blocks, waits for the Slave to push the bytes across the SDA wire, and then mathematically reassembles the 16-bit sensor data!

### 2. The Slave's Role: Interrupt-Driven Callbacks
```cpp
Wire.onRequest(handleMasterRequest);
// ...
void handleMasterRequest() {
  Wire.write(highByte);
  Wire.write(lowByte);
}
```
- The Slave (Mode 0) runs its own code in `loop()`, reading sensors. It does not pause to poll for network traffic.
- When the Master initiates communication, the hardware I2C peripheral inside the Slave's ATmega328P instantly halts the CPU and triggers an Interrupt Service Routine (ISR).
- The `handleMasterRequest` function executes instantaneously, blasting the data onto the bus, and then the Slave seamlessly returns to its `loop()` as if nothing happened!

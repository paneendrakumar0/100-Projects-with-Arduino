# Day 52: CAN Bus Controller (MCP2515 SPI Node)

Welcome to Day 52 of the 100-Day Arduino Masterclass! Today, we dive into automotive and high-reliability aerospace networks by building a **CAN Bus (Controller Area Network) Node** using an **MCP2515 CAN Controller** and a CAN transceiver. To understand the low-level bit operations of automotive networks, we will write a raw **SPI-based driver for the MCP2515 from scratch**, configuring CAN frames, timing registers, and filters without high-level library dependencies.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/CAN_Bus.jpg" alt="CAN Bus" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

Robotics and automotive systems require multiple controllers to share sensor data and control commands rapidly.
* **The Problem:** Point-to-point buses like RS485/Modbus require a master node to query slave nodes, creating latency. Peer-to-peer serial lines get cluttered quickly. If two nodes transmit at the same time on standard serial, a collision corrupts the data.
* **The Solution:** **CAN Bus**:
  1. **Non-Destructive Bitwise Arbitration**: Allows any node to transmit immediately. If a collision occurs, the message with the higher priority (lowest ID value) continues transmitting uninterrupted, while the lower-priority node waits.
  2. **MCP2515 Controller**: Acts as a hardware protocol coprocessor. It handles bit stuffing, frame generation, error tracking, and CRC validation, exposing standard SPI registers to the Arduino.

---

## 🔬 Physics & Mathematics

### 1. CAN Physical Layer & Termination
The CAN Bus uses a differential twisted pair named **CAN_H** (CAN High) and **CAN_L** (CAN Low). 
* **Recessive state (1)**: Both lines hover at $2.5\text{V}$ ($V_{\text{DIFF}} = 0\text{V}$).
* **Dominant state (0)**: CAN_H is driven up to $3.5\text{V}$ and CAN_L is pulled down to $1.5\text{V}$ ($V_{\text{DIFF}} = 2.0\text{V}$).

```
  Volt
  4.0V +
  3.5V +        /----\       (CAN_H)
  2.5V +-------/------\----- (Recessive idle level)
  1.5V +-------\______/----- (CAN_L)
  0.0V +--------------------
```

To prevent electrical signal reflections from traveling back along the bus and corrupting data, a **$120\,\Omega$ termination resistor** must be placed between CAN_H and CAN_L at both ends of the network.

---

### 2. Non-Destructive Bitwise Arbitration
CAN IDs determine priority. Because a dominant bit (0) physically overrides a recessive bit (1), if two nodes transmit simultaneously, the node that outputs a dominant bit wins:

```
  Node 1 (ID 0x244 = 010 0100 0100)  --- 0 1 0 0 1 0 0 0 1 0 0
  Node 2 (ID 0x100 = 001 0000 0000)  --- 0 0 1 ... (Wins arbitration!)
  Actual Bus State                   --- 0 0 1 ...
                                         ^
                                  Collision point: Node 1 outputs 1 (Recessive)
                                  but reads 0 (Dominant) from bus. Node 1 stops TX.
```

Node 2 continues transmitting without losing a single bit of data!

---

### 3. Bit Timing and CNF Registers Math
CAN bus speeds must match across all nodes. The duration of 1 bit is divided into **Time Quanta ($T_Q$)**.
To configure a bus speed of **$125\,\text{kbps}$** using a **$16\,\text{MHz}$ crystal oscillator**:

1. We set the total bit time to **$8\,T_Q$**.
2. This implies the duration of $1\,T_Q$ is:
   $$\text{Bit Time} = \frac{1}{\text{Baud Rate}} = \frac{1}{125,000\,\text{bps}} = 8.0\,\mu\text{s}$$
   $$T_Q = \frac{\text{Bit Time}}{8} = 1.0\,\mu\text{s}$$
3. We compute the Baud Rate Prescaler ($BRP$) using the MCP2515 timing equation:
   $$T_Q = \frac{2 \cdot (BRP + 1)}{f_{\text{osc}}}$$
   $$1.0\,\mu\text{s} = \frac{2 \cdot (BRP + 1)}{16,000,000}$$
   $$BRP + 1 = 8 \Rightarrow BRP = 7$$
4. We write $7$ to the $BRP$ bits in `CNF1` (Register `0x2A`), and define segment intervals (Propagation delay, Phase Seg 1, Phase Seg 2) in `CNF2` and `CNF3` registers to total exactly $8\,T_Q$.

---

## 🔄 Network Bus Comparison

| Protocol | Physical Layer | Max Baud Rate | Arbitration | Hardware overhead | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CAN Bus** | **Differential Pair** | **$1\,\text{Mbps}$** | **Dominant/Recessive (Arbitration)** | **High (Controller + Transceiver) (Our choice)** | **Automotive, motor controllers (VESC), high-reliability HMI** |
| **Modbus (RS485)** | Differential Pair | $115.2\,\text{kbps}$ | Master-Slave polling (No collisions) | Low (Transceiver only) | Long-range factory automation, environmental monitoring |
| **LIN Bus** | Single Wire | $20\,\text{kbps}$ | Master-Slave scheduling | Low (Transceiver only) | Low-cost automotive sub-systems (mirrors, seats) |
| **Ethernet** | 4-Wire / 8-Wire | $>100\,\text{Mbps}$ | CSMA/CD (Collision detection) | Very High | Heavy data streaming, LiDAR point clouds, ROS nodes |

---

## 🛠️ Components Needed

* 2x Arduino Uno boards (or 1 Uno and 1 Mega)
* 2x MCP2515 CAN Bus SPI Modules (featuring TJA1050 transceivers)
* **2x $120\,\Omega$ resistors**
* 1x Breadboard & Jumper wires

---

## 🔌 Pin-to-Pin Wiring

Connect the MCP2515 module to the Arduino hardware SPI bus.

```
 Arduino D10 (CS)   -------------------> CS (Chip Select)
 Arduino D11 (MOSI) -------------------> SI (Serial Input)
 Arduino D12 (MISO) <------------------- SO (Serial Output)
 Arduino D13 (SCK)  -------------------> SCK (Serial Clock)
```

### Wiring Table
| MCP2515 Board Pin | Arduino Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** | Power input |
| **GND** | **GND** | Shared ground |
| **CS** | **D10** | Chip Select |
| **SO** | **D12** | Master In Slave Out (MISO) |
| **SI** | **D11** | Master Out Slave In (MOSI) |
| **SCK** | **D13** | SPI Clock |
| **INT** | Pin D2 (Optional) | Interrupt line |

---

## 💻 How to Test & Validate

To test the CAN network, you must connect **two separate Arduino nodes** together to form a bus.

1. **Wire the CAN Bus**:
   * Connect **CAN_H** of Node 1 to **CAN_H** of Node 2.
   * Connect **CAN_L** of Node 1 to **CAN_L** of Node 2.
   * Connect a **$120\,\Omega$ resistor** between CAN_H and CAN_L at Node 1.
   * Connect a **$120\,\Omega$ resistor** between CAN_H and CAN_L at Node 2.
   * Connect the **GND** pins of both Arduinos together to establish a common reference voltage.
2. **Upload & Monitor**:
   * Upload [Day_52_CAN_Bus.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_52_CAN_Bus/Day_52_CAN_Bus.ino) to both boards.
   * Open the **Serial Monitor** at **9600 Baud** for one of the boards.
   * Every 2 seconds, the node will transmit a telemetry packet and log the event:
     `[CAN TX] Frame ID: 0x244 | DLC: 4 | Data: 0x55 0x02 0x1A 0x00`
   * Symmetrically, it will read the packet transmitted by the other node and print it:
     `[CAN RX] Frame ID: 0x244 | DLC: 4 | Payload: 0x55 0x02 0x18 0x00`
3. **Test Interrupt Trigger**:
   * Ground Pin 7 on Node 1 (using a jumper wire).
   * Node 1 will instantly transmit a high-priority interrupt frame:
     `[CAN TX] Frame ID: 0x100 | DLC: 2 | Data: 0xAA 0x99`
   * Node 2 will immediately receive and log the message!

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The serial monitor keeps printing `[ERROR] MCP2515 failed to enter Config mode!`**:
  * Double check the SPI wiring. Ensure CS, MOSI, MISO, and SCK are wired to Pins 10, 11, 12, and 13 respectively.
  * Verify that the MCP2515 board is powered with 5V.
* **Nodes are transmitting, but neither node receives any packets**:
  * Ensure the CAN_H and CAN_L lines are not swapped.
  * Check the bus termination. Measure the resistance between CAN_H and CAN_L with the power turned off. The total resistance should be **$60\,\Omega$** (two $120\,\Omega$ resistors in parallel). If it is $120\,\Omega$, you are missing one resistor. If it is infinite, you are missing both.
* **Mismatched Crystal Frequency (Crucial Module issue)**:
  * Cheap MCP2515 breakout modules are sold with either a **$16\,\text{MHz}$** crystal or an **$8\,\text{MHz}$** crystal.
  * The code timing configurations are calculated for a **$16\,\text{MHz}$** crystal. If your module has an $8\,\text{MHz}$ crystal (check the silver cylinder on the board), the baud rate will run at $62.5\,\text{kbps}$ instead of $125\,\text{kbps}$.
  * To fix this for an $8\,\text{MHz}$ module, adjust the baud rate prescaler ($BRP$) in register `CNF1` to **`0x03`** (BRP=3) instead of `0x07`. This matches the timing values to the slower clock rate.

## 🧠 Code Explanation

Let's break down how we talk to the CAN Bus controller using raw SPI registers:

### 1. Bit Timing and Baud Rate Configuration
```cpp
mcpWriteRegister(REG_CNF1, 0x07);
mcpWriteRegister(REG_CNF2, 0x91);
mcpWriteRegister(REG_CNF3, 0x01);
```
- CAN bus requires exact synchronization between nodes. 125 kbps means 1 bit takes exactly 8 microseconds.
- An automotive CAN controller breaks that 8µs bit down into Time Quanta (TQ) to handle propagation delays across long copper wires.
- We configure `CNF1`, `CNF2`, and `CNF3` to set the Prescaler to 8 (yielding 1µs TQ) and divide the bit into a Sync phase, Prop phase, Phase 1, and Phase 2. This perfectly aligns our sampling point to 75% of the bit time!

### 2. Fast SPI Transmission (RTS)
```cpp
digitalWrite(CAN_CS_PIN, LOW);
SPI.transfer(INST_RTS_TXB0);
digitalWrite(CAN_CS_PIN, HIGH);
```
- We write our Standard ID, Data Length (DLC), and 8 bytes of payload into the `TXB0` registers.
- Instead of using a slow software command to tell the chip to transmit, we use a dedicated hardware SPI instruction: Request-To-Send (`INST_RTS_TXB0`, 0x81).
- This blasts the packet out onto the differential CAN lines instantly, autonomously handling bit-stuffing and CRC generation in hardware!

# Day 51: Industrial Modbus RTU Slave Node (RS485)

Welcome to Day 51 of the 100-Day Arduino Masterclass! Today, we transition to industrial automation and robust serial networks by building a **Modbus RTU Slave Node** using a **MAX485 Transceiver Module**. To understand how industrial PLCs (Programmable Logic Controllers) communicate with sensors and actuators, we will write a raw **Modbus RTU packet framing engine, silent character interval timer, and CRC-16 verifier** completely from scratch, without external Modbus libraries.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/CAN_Bus.jpg" alt="CAN Bus" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Relay_Module.jpg" alt="Relay Module" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

Consumer protocols like I2C, SPI, and basic TTL serial work well on breadboards over short distances. 
* **The Problem:** In factories, long cable runs (up to hundreds of meters) pick up massive electromagnetic interference (EMI) from high-voltage motors and switches. Under these conditions, standard 5V TTL serial signals get corrupted instantly, and multi-node configurations become complex.
* **The Solution:** We use **RS485 half-duplex differential communication** running the **Modbus RTU industrial application protocol**:
  1. **MAX485 Transceiver**: Converts standard Arduino serial pins into differential voltages ($A$ and $B$ lines).
  2. **Modbus RTU Parser**: Processes packets that carry a Slave ID, a Function Code, register addresses, data values, and a 16-bit Cyclic Redundancy Check (CRC-16).
  3. **Registers Map**: Allows any master PLC or computer to read our sensors (analog registers) and write to our actuators (LED/relay registers) over a robust 2-wire bus.

---

## 🔬 Physics & Mathematics

### 1. RS485 Differential Signaling Physics
Unlike single-ended TTL serial (which measures voltage relative to a shared Ground wire), RS485 uses **differential signaling** over a twisted pair of wires named $A$ and $B$:
* Logic High (1) is represented by: $V_A - V_B > +200\,\text{mV}$
* Logic Low (0) is represented by: $V_A - V_B < -200\,\text{mV}$

```
       TTL Serial (Single-Ended)                      RS485 (Differential)
  5V +-----\                                    A +--------\       /------
            \                                               \     /
  0V +-------\------ (Ref to GND)               B +----------\___/--------
                                                   (V_A - V_B determines logic state)
```

If electromagnetic noise strikes the cable, it affects both wires equally (Common-Mode Noise). Since the receiver only measures the *difference* in voltage between $A$ and $B$, the noise is mathematically canceled out:
$$(V_A + \text{Noise}) - (V_B + \text{Noise}) = V_A - V_B$$

This provides extreme noise immunity over runs up to **$1200\,\text{meters}$**.

---

### 2. Silent Interval Frame Delimitation
Modbus RTU does not use special start/stop characters (like `$` or `\n`). Instead, it detects packet boundaries by measuring the silent gap (idle time) on the serial line.
* **The Rule**: A packet is complete when the line is silent for at least **3.5 character times**.
* **The Mathematics**: At $9600\,\text{Baud}$, we transmit $9600\,\text{bits/second}$. A single serial frame (8N1) consists of 10 bits (1 start bit, 8 data bits, 1 stop bit).
  $$\text{Time per character} = \frac{10\,\text{bits}}{9600\,\text{bits/sec}} = 1.04\,\text{ms}$$
  $$\text{3.5-Character Silent Interval} = 3.5 \times 1.04\,\text{ms} = 3.64\,\text{ms}$$

In our code, we use a robust **$5\,\text{ms}$** idle timer to safely detect frame completion.

---

### 3. CRC-16 Generator Polynomial Math
To verify packet integrity, Modbus RTU uses a 16-bit Cyclic Redundancy Check (CRC-16). The sender treats the data bytes as a huge binary polynomial, divides it by a standard generator polynomial ($x^{16} + x^{15} + x^2 + 1$, reflected as `0xA001`), and appends the remainder.
Our slave node recalculates this remainder by shifting and XORing the bytes:

```
 For each byte in buffer:
   CRC = CRC XOR Byte
   Repeat 8 times:
     If LSB of CRC is 1:
       CRC = (CRC >> 1) XOR 0xA001
     Else:
       CRC = CRC >> 1
```

If the calculated CRC does not match the trailing bytes of the packet, the frame is rejected.

---

## 🔄 Network Protocol Comparison

| Protocol | Physical Layer | Max Nodes | Duplex | Distance Limit | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Modbus RTU** | **RS485 (Twisted Pair)** | **247** | **Half-Duplex** | **$1200\,\text{m}$ (Our choice)** | **Factory automation, PLCs, industrial sensors** |
| **Modbus TCP** | Ethernet / Wi-Fi | Unlimited | Full-Duplex | Depend on Network | Enterprise SCADA integration, smart building controllers |
| **CAN Bus** | CAN physical bus | 110 | Half-Duplex | $40\,\text{m}$ (at 1Mbps) | Automotive networking, high-speed robotics networks |
| **I2C** | 2-Wire (SDA/SCL) | 127 | Half-Duplex | $<3\,\text{m}$ | Short-range, onboard chip-to-chip interfaces |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x MAX485 TTL-to-RS485 Converter Module
* 1x $10\,\text{k}\Omega$ Potentiometer (or any analog sensor connected to A0)
* 1x Breadboard & Jumper wires
* 1x USB-to-RS485 Dongle (Optional: to interface the bus with a PC for testing)

---

## 🔌 Pin-to-Pin Wiring

> [!NOTE]
> The MAX485 transceiver requires flow control. We tie **DE** (Driver Enable, active HIGH) and **RE** (Receiver Enable, active LOW) together. When Arduino Pin 4 is HIGH, the transceiver acts as a transmitter. When Pin 4 is LOW, it listens as a receiver.

```
 Arduino D2 (RX) <------------------- RO (Receiver Output)
 Arduino D3 (TX) -------------------> DI (Driver Input)
 Arduino D4 (Flow) -----------------> DE + RE (Tied together)
 
                 +--------------+
                 |    MAX485    | [A] <====================== RS485 Bus Line A
                 |              | [B] <====================== RS485 Bus Line B
                 +--------------+
```

### Wiring Table
| MAX485 Pin | Arduino Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** | Power input |
| **GND** | **GND** | Common ground |
| **RO** | **D2** | SoftwareSerial RX |
| **DI** | **D3** | SoftwareSerial TX |
| **DE & RE** | **D4** | Flow Control (HIGH=TX, LOW=RX) |
| **A** | **Bus line A** | Differential positive terminal |
| **B** | **Bus line B** | Differential negative terminal |

---

## 💻 How to Test & Validate

To test the node, you can use a PC software utility (like **QModMaster** or **Modpoll**) along with a USB-to-RS485 converter dongle connected to your computer.

1. **Verify Raw Hex Frames**:
   * Set the PC test software to **9600 Baud**, **8 Data Bits**, **1 Stop Bit**, **No Parity**.
   * Set Slave ID to `1`.
2. **Test Function Code 0x03 (Read Holding Registers)**:
   * Request to read **2 registers** starting at address `0x0000`.
   * **PC Transmits (Hex)**: `01 03 00 00 00 02 C4 0B`
   * **Arduino Decodes & Responds (Hex)**: `01 03 04 XX XX YY YY CRC_L CRC_H`
     *(Where `XX XX` is the current raw sensor input on A0, and `YY YY` is the LED status (00 00 or 00 01)).*
3. **Test Function Code 0x06 (Write Single Register)**:
   * Write value `1` to Register `0x0001` (to turn the Pin 13 LED ON).
   * **PC Transmits (Hex)**: `01 06 00 01 00 01 19 CA`
   * **Arduino Actuates LED & Responds (Hex)**: `01 06 00 01 00 01 19 CA`
     *(Observe the Arduino's built-in LED turn ON!)*
   * Write value `0` to Register `0x0001` to turn the LED OFF.
     * **PC Transmits (Hex)**: `01 06 00 01 00 00 D8 0A`

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The communications do not respond, but TX/RX LEDs flash on the USB dongle**:
  * The $A$ and $B$ differential lines are swapped. Swap the connections at the MAX485 block ($A \rightarrow B$, $B \rightarrow A$).
  * The Slave ID in the master application does not match the `SLAVE_ADDRESS` constant in code (default is `1`).
* **You get constant packet errors or timeout issues**:
  * Ensure the Arduino and MAX485 share a solid Ground connection.
  * For long cable runs, add a **$120\,\Omega$ termination resistor** across the $A$ and $B$ terminals at both ends of the bus to prevent signal reflections.
* **The driver fails to revert to listen mode after sending**:
  * Ensure `rs485Serial.flush()` is called in your transmit function. If the flow control pin is pulled LOW before the UART finishes pushing bytes out of the hardware register, the end of the packet will be cut off.

## 🧠 Code Explanation

Let's break down how we built a raw Modbus RTU node:

### 1. The RS485 Half-Duplex Transceiver
```cpp
void transmitFrame(uint8_t* buffer, int length) {
  digitalWrite(RS485_FLOW_PIN, HIGH);
  rs485Serial.write(buffer, length);
  rs485Serial.flush();
  digitalWrite(RS485_FLOW_PIN, LOW);
}
```
- RS485 is a 2-wire differential bus. It is "Half-Duplex", meaning devices cannot talk and listen at the same time.
- The MAX485 chip has a Flow Control pin (`DE/RE`). We must pull it `HIGH` to put the chip into Transmit Mode, blast our byte array out the serial port, call `.flush()` to ensure every single bit has physically left the Arduino, and then instantly pull the pin `LOW` to go back to Listening mode before the master replies!

### 2. Modbus Silent Interval Delimiter
```cpp
if (rxIndex > 0 && (millis() - lastCharTimeMs >= MODBUS_SILENT_INTERVAL_MS)) {
    processModbusFrame();
    rxIndex = 0; 
}
```
- Unlike some protocols that use start/stop characters (like `<` and `>`), Modbus uses **Time** as a delimiter.
- The standard dictates that any silence on the line longer than 3.5 character times (about 4-5ms at 9600 baud) signifies the End of a Frame.
- Every time a byte arrives, we reset `lastCharTimeMs`. Once the line goes quiet for >5ms, our `if` statement triggers, taking all the bytes we collected in the buffer and passing them to the CRC validator and parser!

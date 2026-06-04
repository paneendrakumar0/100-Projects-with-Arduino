# Day 69: DHT22 Temperature & Humidity Sensor (Custom Single-Wire Bit-Bang Parser)

Welcome to Day 69! Today we dive deep into low-level digital communication protocols. Instead of using standard, high-level libraries (like the Adafruit DHT library), we will build a **custom single-wire, half-duplex bit-bang parser** from scratch for the **DHT22 (AM2302) sensor**. We will also calculate the local **Dew Point** using the Magnus Formula and the **Heat Index (Feels Like)** using the Steadman/Rothfusz regression equation.

---

## 🎯 The "Why" and "What"

The DHT22 (AM2302) is a popular, low-cost digital temperature and humidity sensor. In robotics and automation, monitoring environmental conditions is key for:
1. **HVAC Systems**: Climate control inside robotic enclosures or smart greenhouses.
2. **Predictive Maintenance**: Detecting moisture build-up or overheating near sensitive electronics.
3. **Sensor Fusion**: Calibrating other sensors (like sound speed for ultrasonic rangefinders, which depends on air temperature).

Rather than using a library, writing the driver ourselves helps us understand:
- Exact microsecond-level GPIO control.
- Half-duplex communication (where a single pin switches dynamically between output and input).
- Direct pulse-width modulation decoding.
- Data verification via checksum validation.

---

## 🔬 Physics & Hardware Theory

### 1. The Single-Wire Interface
The DHT22 does not use standard I2C, SPI, or UART. It uses a custom **single-wire, half-duplex protocol**:
- **Idle State**: The host (Arduino) keeps the data line pulled HIGH via an external 10 kΩ pull-up resistor.
- **Host Start Signal**: To initiate reading, the Arduino pulls the pin LOW for at least 1 ms (we use 2 ms to ensure the sensor wakes up), then releases the line (pulling it HIGH) and waits 20–40 µs.
- **Sensor Response**: The sensor responds by pulling the line LOW for 80 µs, then letting it go HIGH for 80 µs.
- **Data Transmission**: The sensor then transmits 40 bits of data.

### 2. Bit Encoding (Pulse-Width Modulation)
Every bit starts with a **50 µs LOW** pulse from the sensor. The value of the bit is determined by the duration of the subsequent **HIGH** pulse:
- **Logical '0'**: HIGH duration of **26–28 µs**.
- **Logical '1'**: HIGH duration of **70 µs**.

We decode this by timing the HIGH pulse using `micros()`. If the pulse width is greater than 40 µs, it represents a `1`; otherwise, it represents a `0`.

```
          ┌───────┐            ┌───────────────┐
          │  '0'  │            │      '1'      │
  ________│ 26-28 │____________│     70 us     │________
    50 us   val=0      50 us        val=1       50 us
```

### 3. Data Format
The 40 bits are grouped into 5 bytes:
- **Byte 0**: Humidity MSB
- **Byte 1**: Humidity LSB
  - $\text{Relative Humidity (\%)} = \frac{(\text{Byte 0} \ll 8) \mid \text{Byte 1}}{10}$
- **Byte 2**: Temperature MSB
- **Byte 3**: Temperature LSB
  - $\text{Temperature (°C)} = \frac{(\text{Byte 2} \ll 8) \mid \text{Byte 3}}{10}$
  - *Note: If the highest bit of Byte 2 (bit 15) is 1, the temperature is negative.*
- **Byte 4**: Checksum
  - $\text{Checksum} = (\text{Byte 0} + \text{Byte 1} + \text{Byte 2} + \text{Byte 3}) \ \& \ 0\text{xFF}$

### 4. Advanced Environmental Metrics
- **Dew Point (Magnus-Tetens Formula)**:
  $$\alpha(T, RH) = \ln(RH/100) + \frac{a \cdot T}{b + T}$$
  $$T_{dew} = \frac{b \cdot \alpha}{a - \alpha}$$
  *(Where $a = 17.625$ and $b = 243.04\,\text{°C}$)*
- **Heat Index (Feels Like)**: Calculated using the Steadman/Rothfusz multi-parameter regression formula, which estimates how hot the air actually feels to human skin by incorporating humidity.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| DHT22 (AM2302) Sensor | 1 | High-accuracy temperature/humidity sensor |
| 10 kΩ Resistor | 1 | Pull-up resistor (VCC to DATA) |
| Breadboard | 1 | Prototyping board |
| Jumper Wires | 4 | Connections |

---

## 🔌 Pin-to-Pin Wiring

| DHT22 Pin | Arduino Uno / Supply | Description |
| :--- | :--- | :--- |
| **Pin 1 (VDD)** | **5V or 3.3V** | Power Supply |
| **Pin 2 (DATA)** | **D2** | Data I/O (Connect 10kΩ resistor from Pin 2 to VDD) |
| **Pin 3 (NC)** | **Not Connected** | Leave open |
| **Pin 4 (GND)** | **GND** | Ground |

---

## 💻 How to Test & Validate

1. Wire the circuit as described, ensuring the 10 kΩ pull-up resistor is in place.
2. Open the Arduino IDE, load [Day_69_DHT22_Custom_Protocol.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_69_DHT22_Custom_Protocol/Day_69_DHT22_Custom_Protocol.ino), and select the appropriate Port.
3. Open the **Serial Monitor** at **9600 Baud**.
4. Observe the environmental logs printed every 2.5 seconds:
   `[DHT22] Temp: 24.5°C / 76.1°F | RH: 55.2% | HeatIdx: 25.1°C | DewPt: 14.8°C`
5. Breathe gently onto the sensor; you should immediately see the Relative Humidity (RH) and Dew Point values increase on the next samples.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `ERROR: Read failed (timeout...)` | Data pin not connected or floating | Ensure Pin D2 is securely connected. Check 10kΩ pull-up resistor. |
| `ERROR: Read failed` (checksum mismatch) | Timing jitter or signal noise | Ensure wires are short. Disable interrupts if other modules cause delays. |
| Sensor returns `0.0` or stale values | Read requested too quickly | DHT22 needs at least 2 seconds between reads. Verify the program waits 2.5s. |
| Negative temperature reading in warm room | MSB bit manipulation bug | Ensure the sign bit extraction masks correctly (`data[2] & 0x80`). |

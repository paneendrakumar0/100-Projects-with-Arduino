# Day 25: 4-Digit 7-Segment Clock (TM1637)

Welcome to Day 25 of the 100-Day Arduino Masterclass! Today, we expand our segment display knowledge to multi-digit arrays. We will learn how to interface a 4-digit 7-segment display module driven by the TM1637 chip and program a digital clock with a flashing colon separator.

You will master the concept of Persistence of Vision (POV) digit multiplexing, study the TM1637 synchronous 2-wire serial protocol, and cascade clock variables in C++.

---

## 🎯 Today's Learning Goals
1. Understand Persistence of Vision (POV) digit multiplexing.
2. Master the synchronous 2-wire serial protocol of the TM1637 driver chip.
3. Wire the TM1637 module and understand CLK and DIO lines.
4. Program a software clock accumulator using `millis()`.
5. Implement a non-blocking 500ms flashing colon chime.

---

## 🧠 The "Why" and "What": Clocks in Robotics

### What is a 4-Digit 7-Segment Display?
A 4-digit 7-segment display packages four individual numeric characters, along with a colon (`:`) or decimal points, in a single housing. Driving 4 digits directly requires 12 control pins. To save pins and offload scanning cycles from the main CPU, we use modules equipped with a dedicated **TM1637 display driver**.

### Why is it Used in Robotics & Instrumentation?
Clocks and elapsed timers are crucial indicators:
- **Autonomous Mission Timers:** Displaying the time elapsed since the robot was activated or the time remaining before a robot is forced to shut down.
- **Instrument Dashboards:** Displaying stopwatch data on industrial equipment.
- **Lap Timers:** Displaying race track lap times in millisecond resolution.
- **Wall Clocks:** Basic digital clocks, alarms, and timers.

---

## ⚡ The Physics & Hardware Theory

### 1. Persistence of Vision (POV) Digit Multiplexing
If you wanted to display `12:34` on a 4-digit display directly, you would run into a problem: the 7 segment pins (A-G) are connected in parallel to all four digits. If you write `1` to segment pins, all four digits will display `1` at the same time.

To display different numbers, we must use **Time-Division Multiplexing**:
1. Turn ON Digit 1, write pattern for `1`. Wait 2ms. Turn OFF Digit 1.
2. Turn ON Digit 2, write pattern for `2`. Wait 2ms. Turn OFF Digit 2.
3. Turn ON Digit 3, write pattern for `3`. Wait 2ms. Turn OFF Digit 3.
4. Turn ON Digit 4, write pattern for `4`. Wait 2ms. Turn OFF Digit 4.
5. Repeat this loop continuously.

```
       Digit Multiplexing Sequence (POV)
       
       Frame 1:  [ 1 ] [   ] [   ] [   ]  <-- Digit 1 active for 2ms
       Frame 2:  [   ] [ 2 ] [   ] [   ]  <-- Digit 2 active for 2ms
       Frame 3:  [   ] [   ] [ 3 ] [   ]  <-- Digit 3 active for 2ms
       Frame 4:  [   ] [   ] [   ] [ 4 ]  <-- Digit 4 active for 2ms
       
       Scan Rate: > 60 Hz ➡️ Brain perceives solid text "1234" (POV)
```

Because of **Persistence of Vision (POV)** (the human eye retains an image for roughly $1/16^{\text{th}}$ of a second), scanning the digits faster than **$60\text{ Hz}$** blends the flashes together. To the human brain, all four digits look solid.

However, running this high-speed scanning loop on the Arduino consumes up to $30\%$ of the CPU cycles. If your code halts for a sensor read (like DHT11, Day 9), the multiplexing stops, and the display starts flickering or goes dark.

### 2. The TM1637 Dedicated Driver Chip
The TM1637 chip solves this by acting as an autonomous display co-processor.
* It contains internal static RAM to store the digit values.
* It handles the high-speed multiplexing scanning internally using its own hardware timers.
* Once the Arduino tells the TM1637 "write 1234," the Arduino can go to sleep or run heavy algorithms. The TM1637 maintains a perfectly solid, flicker-free display.

### 3. TM1637 2-Wire Serial Protocol
The TM1637 communicates using a custom synchronous serial protocol that resembles I2C but lacks addressing (since it is a point-to-point connection):
* **CLK (Clock):** Driven by the Arduino. Tells the TM1637 when to read a bit.
* **DIO (Data Input/Output):** Bidirectional data line. Data is transferred LSB-first (Least Significant Bit).
* **Data Byte Packet Structure:**
  - **Start Bit:** DIO pulled LOW while CLK is HIGH.
  - **8-bit Command/Data Byte:** Transmitted bit-by-bit on the rising edge of CLK.
  - **ACK (Acknowledge):** The TM1637 pulls the DIO line LOW on the 9th clock pulse to confirm it received the byte.
  - **Stop Bit:** DIO pulled HIGH while CLK is HIGH.

```
TM1637 2-Wire Byte Packet:

          START             LSB                 MSB      ACK      STOP
  CLK  ______|‾‾‾|   |‾‾‾|   |‾‾‾|   ...   |‾‾‾|   |‾‾‾|  |‾‾‾|   |‾‾‾|____
  DIO  ____\_____/   \___/   \___/   ...   \___/   \___/  \___/   /________
            (Data bits latched on rising edge)
```

---

## 🔄 Alternatives: TM1637 vs. RTC Modules vs. MAX7219

| Display Interface | Communication | Pin Count | Onboard Timekeeper | Accuracy | Best Use Case |
| :--- | :--- | :---: | :--- | :--- | :--- |
| **TM1637 4-Digit Display** | 2-Wire Serial (CLK/DIO) | 4 pins | No (requires software clock). | Moderate (limited by Arduino crystal drift). | **Chosen** for budget clocks, timers, and simple numeric displays. |
| **DS3231 I2C RTC Module** | I2C (2 pins) | 4 pins | Yes (integrated battery & TCXO crystal). | Extremely High (drift $< 1\text{ min/year}$). | Real-time clocks that must remember time when powered off. |
| **MAX7219 8-Digit Driver** | SPI (CS, CLK, DIN) | 5 pins | No | Moderate | Driving up to 8 digits, large control panel indicators. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **TM1637 4-Digit 7-Segment Display Module** (usually has a colon ':' in the middle).
3. **Breadboard & Jumper Wires**.
4. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

| TM1637 Module Pin | Arduino Pin | Wire Color | Description |
| :---: | :---: | :--- | :--- |
| **VCC** | **5V** | Red | Power supply (+5V) |
| **GND** | **GND** | Black | Ground reference |
| **CLK** | **Pin 2** | Yellow | Synchronous clock line |
| **DIO** | **Pin 3** | Green | Serial data input/output line |

---

## 🧪 How to Test and Validate

Follow these steps to install the library and verify your clock:

### 1. Library Installation
- Open the Arduino IDE.
- Go to **Tools > Manage Libraries**.
- Search for **"TM1637"** by **Avishay Orbach**.
- Select **Install**.

### 2. Verify Clock Tick and Colon Flash
- Upload `Day_25_4Digit_7Seg_Clock.ino`.
- Open the Serial Monitor at **9600 Baud**.
- **Display Check:**
  - The display should light up showing: `12:00`.
  - **The Colon Test:** The central colon (`:`) should blink ON and OFF exactly every 500ms.
- **Clock Accumulation:**
  - Watch the display for 1 minute.
  - The minutes digit should increment: `12:01`, and the Serial Monitor should print logs confirming the time change:
    ```text
    [CLOCK] Time: 12:01:00
    [CLOCK] Time: 12:01:01
    ```

### 🔍 Troubleshooting Tips
* **The display is completely dead (blank):**
  - Verify that VCC goes to **5V** (not 3.3V) and GND is connected.
  - Make sure the CLK pin is wired to Pin 2 and DIO to Pin 3.
* **The clock displays gibberish symbols instead of numbers:**
  - Check your library version. Make sure you installed the library by **Avishay Orbach**. Different TM1637 libraries use different function calls.
* **The colon does not flash (always ON or always OFF):**
  - Verify your module has a physical colon. Some modules have 4 decimal points (one per digit) instead of a colon.
  - If using a decimal point module, the `showNumberDecEx` colon mask `0b01000000` will turn on the second digit's decimal point.

# Day 55: Parallel HD44780 LCD (Direct 8-Bit Interface)

Welcome to Day 55 of the 100-Day Arduino Masterclass! Today, we explore parallel bus architectures by interfacing a standard **HD44780 16x2 Character LCD** via its raw **8-bit parallel data interface**. Rather than relying on the standard Arduino LiquidCrystal library or serial expanders (I2C), we will write raw **bus strobe waveforms, command initialization sequences, and custom CGRAM character font loading** from scratch. 

To preserve the Arduino's serial hardware UART (Pins D0/D1) for debugging, we will implement a hardware/software strategy known as **Port Splitting**.

---

## 🎯 The "Why" and "What"

Modern sensors and displays often hide behind serial interfaces like I2C or SPI. 
* **The Problem:** Serial buses must transmit data bit-by-bit, introducing communication latency. Parallel buses send an entire byte of data simultaneously, providing high throughput. However, an 8-bit parallel bus requires 8 data pins plus control lines. If we map this directly to Port D on the ATmega328P (Uno Pins D0 to D7), we lose the RX and TX pins, disabling the USB Serial Monitor interface for debugging.
* **The Solution:** We implement **Port Splitting**. We map the 8 data lines across two physical port registers: Uno Pins D2 to D7 (Port D) and D8 to D9 (Port B). In software, we split, shift, and mask the data byte before sending it, preserving Pins D0 and D1 for serial diagnostics.

---

## 🔬 Physics & Mathematics

### 1. Parallel Bus Signaling & Enable Strobes
The HD44780 operates using two registers: the **Command Register** (RS Pin LOW) and the **Data Register** (RS Pin HIGH).
To transmit, we write the data byte onto the data pins, then pulse the **Enable (E)** pin. The LCD controller latches the state of the data bus on the **falling edge** of the Enable signal.

```
 RS Pin     +---------\___________________________/~~~~~~~~~
 Data Bus   ==========[       Valid Byte          ]=========
 Enable Pin __________/~~~~~~~~~~~~~~~~~~~~~~~~~~~\_________
                                                  ^
                                           Data Latched here!
```

According to the HD44780 timing specifications, the Enable pulse width ($PW_{EH}$) must be at least **$450\,\text{ns}$** to guarantee reliable data capture.

---

### 2. Port Splitting Mathematics
We must map an 8-bit byte ($\text{Value} = [b_7 \, b_6 \, b_5 \, b_4 \, b_3 \, b_2 \, b_1 \, b_0]$) to:
* Pins D2 to D7 (Port D, bits 2 to 7) for bits $[b_5 \dots b_0]$
* Pins D8 to D9 (Port B, bits 0 to 1) for bits $[b_7 \dots b_6]$

To avoid overwriting pins D0/D1 (Serial) on Port D, and pins D10-D13 (SPI) on Port B, we use bitwise masking and shifting:

#### Port D Formula:
$$\text{PORTD}_{\text{new}} = (\text{PORTD}_{\text{old}} \ \& \ \text{0x03}) \ | \ ((\text{Value} \ \& \ \text{0x3F}) \ll 2)$$
* `PORTD & 0x03` preserves the state of D0 and D1.
* `(Value & 0x3F) << 2` isolates the lower 6 bits of the data value and shifts them into bits 2 to 7.

#### Port B Formula:
$$\text{PORTB}_{\text{new}} = (\text{PORTB}_{\text{old}} \ \& \ \text{0xFC}) \ | \ ((\text{Value} \ \& \ \text{0xC0}) \gg 6)$$
* `PORTB & 0xFC` preserves pins D10 to D13 (SPI CS/MOSI/MISO/SCK).
* `(Value & 0xC0) >> 6` isolates the upper 2 bits of the data value and shifts them into bits 0 to 1.

---

### 3. CGRAM Custom Font Loading
Character LCDs store standard alphanumeric glyphs in ROM. To display custom graphics (like progress bars or icons), we must write custom bitmaps to the **Character Generator RAM (CGRAM)**. 
* The LCD has 64 bytes of CGRAM, which can hold up to **8 custom characters** (each $5 \times 8$ pixels).
* We set the CGRAM start address (`0x40`) and write 8 bytes sequentially. Each byte represents a row of 5 pixels (1 = pixel ON, 0 = pixel OFF).

---

## 🔄 LCD Bus Comparison

| Interface Type | Pin Count | Speed / Latency | Command Overhead | Wiring Complexity |
| :--- | :--- | :--- | :--- | :--- |
| **8-Bit Parallel** | **10 Pins** | **Ultra-Fast (Direct bus)** | **Very Low (No byte translation) (Our choice)** | **High (Requires port mapping)** |
| **4-Bit Parallel** | 6 Pins | Medium-Fast (Bytes split) | Medium (Each byte requires two writes) | Medium |
| **I2C (PCF8574 Backpack)**| 2 Pins | Slow (100-400 kHz) | High (Requires I2C register updates) | Low |
| **SPI (74HC595 Shift)** | 3 Pins | Medium-Fast (8-10 MHz) | Medium (Requires shift register writes) | Medium |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x HD44780 16x2 Character LCD (or 20x4 LCD)
* 1x $10\,\text{k}\Omega$ Potentiometer (for screen contrast control)
* 1x Breadboard & Jumper wires

---

## 🔌 Pin-to-Pin Wiring

Make sure to hook up the data lines to the split ports exactly as shown in the table to align with the software equations.

```
                  +---------------------------+
                  |     HD44780 16x2 LCD      |
                  +---------------------------+
                   |  |  |  |  |  |  |  |  |  |
                  D0 D1 D2 D3 D4 D5 D6 D7 RS E
                   |  |  |  |  |  |  |  |  |  |
 Arduino Uno Pins D2 D3 D4 D5 D6 D7 D8 D9 A0 A1
```

### Wiring Table
| LCD Pin | Arduino Pin | Port Bit | Description |
| :--- | :--- | :--- | :--- |
| **1 (VSS)** | **GND** | GND | Ground |
| **2 (VDD)** | **5V** | 5V | Power supply |
| **3 (VO)** | **Potentiometer Wiper** | - | Contrast adjustment voltage ($0 - 5\text{V}$) |
| **4 (RS)** | **A0** | Port C0 | Register Select (0=Command, 1=Data) |
| **5 (R/W)** | **GND** | GND | Read/Write (Permanently tied to Ground) |
| **6 (E)** | **A1** | Port C1 | Enable Strobe signal |
| **7 (D0)** | **D2** | Port D2 | Data Bit 0 |
| **8 (D1)** | **D3** | Port D3 | Data Bit 1 |
| **9 (D2)** | **D4** | Port D4 | Data Bit 2 |
| **10 (D3)** | **D5** | Port D5 | Data Bit 3 |
| **11 (D4)** | **D6** | Port D6 | Data Bit 4 |
| **12 (D5)** | **D7** | Port D7 | Data Bit 5 |
| **13 (D6)** | **D8** | Port B0 | Data Bit 6 |
| **14 (D7)** | **D9** | Port B1 | Data Bit 7 |
| **15 (A)** | **5V** | 5V | Backlight LED Anode |
| **16 (K)** | **GND** | GND | Backlight LED Cathode |

---

## 💻 How to Test & Validate

1. Wire the LCD and the potentiometer as shown in the table. 
2. Upload [Day_55_Parallel_LCD.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_55_Parallel_LCD/Day_55_Parallel_LCD.ino) to the Arduino.
3. **Adjust Contrast**:
   * If you see nothing, rotate the potentiometer shaft. VO adjusts the LCD liquid crystal bias voltage. At $0\text{V}$, character blocks appear solid. At $5\text{V}$, they fade completely. Adjust until text is clearly visible.
4. **Observe Layout**:
   * Row 1 displays static text: `Day 55 Parallel`
   * Row 2 displays the loading animation: `Loading: [███    ]`
   * The loading block segments utilize custom characters generated in the CGRAM matrix.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The display displays solid black rectangles on the first row, and nothing on the second row**:
  * The LCD is powered on, but it did not execute the initialization sequence. Double check your wiring on pins **RS (A0)** and **E (A1)**.
  * Check the initialization timing in the code. Ensure the startup delay (`delay(50)`) is present to let the LCD's internal logic stabilize.
* **The LCD displays random, flickering garbage characters**:
  * The data lines are wired incorrectly or out of sequence. Ensure D2-D7 represent bits 0-5, and D8-D9 represent bits 6-7.
  * Ensure the shared Ground connection is stable.
* **Some custom characters appear corrupted or distorted**:
  * Ensure you reset the DDRAM pointer address after writing to CGRAM. The command `lcdWriteCommand(0x80)` must be called to return the cursor to standard display coordinates before printing text.

## 🧠 Code Explanation

Let's break down how we built an 8-bit parallel bus using Port Splitting:

### 1. Port Splitting the Data Bus
```cpp
PORTD = (PORTD & 0x03) | ((value & 0x3F) << 2);
PORTB = (PORTB & 0xFC) | ((value & 0xC0) >> 6);
```
- An 8-bit parallel LCD needs 8 data wires. The ATmega328P's "Port D" has 8 pins (D0-D7). But D0 and D1 are used for the USB Serial connection! If we overwrite them, we lose `Serial.print()`.
- **The Split:** We take our 8-bit character (e.g., `value`).
- We slice off the bottom 6 bits (`& 0x3F`), shift them left by 2 (`<< 2`), and inject them into D2-D7 on `PORTD`. 
- We slice off the top 2 bits (`& 0xC0`), shift them right by 6 (`>> 6`), and inject them into D8-D9 on `PORTB`. 
- Our 8-bit byte is perfectly split across two physical hardware ports simultaneously!

### 2. The Enable Strobe
```cpp
digitalWrite(PIN_E, HIGH);
delayMicroseconds(5); 
digitalWrite(PIN_E, LOW);
```
- Once all 8 data wires are energized with the correct 1s and 0s, the LCD doesn't read them immediately.
- We have to flick the Enable (`E`) pin HIGH, wait a few microseconds, and pull it LOW.
- The LCD's internal controller latches the 8 bits into its memory on the *falling edge* of this strobe pulse!

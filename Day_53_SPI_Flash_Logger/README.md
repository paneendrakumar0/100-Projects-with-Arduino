# Day 53: SPI Flash Memory Logger (Winbond W25QXX)

Welcome to Day 53 of the 100-Day Arduino Masterclass! Today, we implement a rugged, high-speed **flight-recorder data logging system** using a **Winbond W25Q64 (64 Megabit / 8 Megabyte) SPI Flash Memory Chip**. To master absolute memory architectures, we will write a raw **register-level SPI driver from scratch** to handle page programs, sector erases, status register polling, and JEDEC device identification without high-level helper libraries.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/EEPROM.jpg" alt="EEPROM" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

Internal Arduino Uno EEPROM is tiny (only $1\,\text{KB}$).
* **The Problem:** Logging dense sensor data (like high-frequency IMU telemetry) will exhaust the internal EEPROM in seconds. While SD card modules provide gigabytes of storage, they require heavy library overhead (FAT32 file systems), draw high currents, and are prone to disconnecting under high vibrations (e.g., inside drones or rovers).
* **The Solution:** We use **Winbond W25QXX SPI Flash Memory**:
  1. **High Capacity**: W25Q64 offers $8\,\text{MB}$ of silicon storage, enough to log hours of telemetry.
  2. **Direct Memory Control**: We communicate directly using SPI commands to write struct records onto pages and erase sectors.
  3. **High Reliability**: The surface-mounted chip has no moving parts, low power consumption, and executes page writes in sub-milliseconds.

---

## 🔬 Physics & Mathematics

### 1. Flash Silicon Memory Architecture
Flash memory (NOR flash) is arranged in a hierarchical layout:
$$\text{8 MB Chip} \rightarrow 128\,\text{Blocks (64 KB)} \rightarrow 2048\,\text{Sectors (4 KB)} \rightarrow 32,768\,\text{Pages (256 Bytes)}$$

```
+-------------------------------------------------------------+
|                     W25Q64 Flash Chip (8 MB)                |
|  +------------------+  +------------------+                 |
|  |   Block 0 (64KB) |  |   Block 1 (64KB) |  ... (128 Blocks)|
|  |  +------------+  |  |                  |                 |
|  |  |Sector0(4KB)|  |  |                  |                 |
|  |  | +--------+ |  |  |                  |                 |
|  |  | |Page 0  | |  |  |                  |                 |
|  |  | |(256B)  | |  |  |                  |                 |
|  |  | +--------+ |  |  |                  |                 |
|  |  +------------+  |  |                  |                 |
|  +------------------+  +------------------+                 |
+-------------------------------------------------------------+
```

* **The Erase Constraint**: Flash memory cells consist of floating-gate transistors. Writing a byte can only change bits from a logic `1` to a logic `0`. To reset a bit from `0` to `1`, the memory must be **erased**. The minimum erasable unit is a **Sector ($4\,\text{KB}$)**, which fills the entire sector with `0xFF`.
* **The Write Constraint**: Programming data (fuzzing `1`s to `0`s) is done using **Page Program** commands, which can write a maximum of **$256\,\text{bytes}$** (one page) at a time.

---

### 2. SPI Command Transactions & Busy Polling
Every flash operation takes a specific amount of time to complete:
* **Page Program**: $\approx 0.5 - 3\,\text{ms}$
* **Sector Erase**: $\approx 40 - 200\,\text{ms}$

To prevent the Arduino from sending new commands while the memory chip is executing an erase or write internally, we read **Status Register 1** (using instruction `0x05`) and poll the **BUSY bit (Bit 0)**. We must wait until BUSY drops to `0` before pulling CS low for the next operation.

```
 Arduino CS  +___\_______________________________________/~~~~~~~~+
 Arduino SCK +    ||||||||||||||||||||||||||||||||||||||
 Arduino MOSI+    [CMD:0x05]  [Read status byte continuously]
 Arduino MISO+    ------------[ 0x01 ]  [ 0x01 ]  [ 0x00 ]
                                                   \__ BUSY drops to 0: ready!
```

---

## 🔄 Memory Hardware Comparison

| Technology | Storage Capacity | Write Cycle Time | Write Endurance | Connection Interface | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **SPI Flash (W25Q64)** | **$8\,\text{MB}$** | **$0.8\,\text{ms}$ (per page)** | **$100,000$ cycles** | **SPI Bus (Our choice)** | **Telemetry flight recorders, black-boxes, firmware storage** |
| **Internal EEPROM** | $1\,\text{KB}$ | $3.3\,\text{ms}$ (per byte) | $100,000$ cycles | Register API | Small configuration variables (Calibration thresholds) |
| **FRAM (Ferroelectric RAM)** | $32\,\text{KB}$ | $<1\,\text{microsecond}$ | $10^{12}$ (Infinite) | I2C / SPI | High-frequency logging, real-time counters |
| **MicroSD Card** | $16\,\text{GB}+$ | Variable ($>10\,\text{ms}$) | $10,000$ cycles | SPI (with FAT system) | Heavy image recording, diagnostic files |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x Winbond W25Q64 SPI Flash Module (or bare SOP-8 chip on breakout)
* 1x Breadboard & Jumper wires
* **1x 4-Channel Logic Level Converter** (W25QXX runs on 3.3V logic. If your board does not have onboard shifters, use dividers or a logic converter to step down the D10/D11/D13 lines).

---

## 🔌 Pin-to-Pin Wiring

> [!CAUTION]
> The Winbond W25Q64 is a **3.3V chip**. Connecting VCC directly to the Arduino 5V pin will destroy the flash memory silicon. Power the board **strictly** from the Arduino 3.3V output.

### Wiring Table
| W25Q64 Module Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **3.3V** | Red | Core power input (Must be 3.3V!) |
| **GND** | **GND** | Black | Ground reference |
| **CS (Chip Select)**| **D10** | Yellow | SPI Slave Select (Active Low) |
| **DO (Data Out)**   | **D12** | Green | SPI Master In Slave Out (MISO) |
| **DI (Data In)**    | **D11** (via Level Shifter) | Blue | SPI Master Out Slave In (MOSI) |
| **CLK (Clock)**     | **D13** (via Level Shifter) | Purple | SPI Clock (SCK) |

---

## 💻 How to Test & Validate

1. **Verify Logic Connection**:
   * Wire the components as shown in the table, using level-shifting for MOSI/SCK/CS signals.
   * Upload [Day_53_SPI_Flash_Logger.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_53_SPI_Flash_Logger/Day_53_SPI_Flash_Logger.ino) to the Arduino.
2. **Examine Diagnostic Telemetry**:
   * Open the **Serial Monitor** at **9600 Baud**.
   * On startup, the program queries the chip's JEDEC ID. You should see:
     ```
     [FLASH] Initializing Winbond W25Q64 Chip...
     [FLASH] JEDEC Identification Telemetry:
       Manufacturer ID: 0xEF
       Memory Type:     0x40
       Capacity ID:     0x17
     [FLASH] W25Q64 confirmed active. Proceeding with logging demo...
     ```
3. **Observe Log Output**:
   * The program erases Sector 0, writes two structured telemetry records, reads them back, and prints the formatted values:
     ```
     [FLASH] Erasing 4 KB Sector at address 0x000000...
     [FLASH] Sector Erase Complete.
     [FLASH] Logging Record 1...
     [FLASH] Logging Record 2...

     [FLASH] Reading raw logged telemetry logs...
     ---------------------------------------------
       Record #1
       Timestamp:   1000 ms
       Temperature: 24.57 °C
       Event Count: 1
       Status:      OK_SYS
     ---------------------------------------------
       Record #2
       Timestamp:   2000 ms
       Temperature: 25.82 °C
       Event Count: 2
       Status:      ALERT
     ---------------------------------------------
     ```

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The serial monitor prints `JEDEC ID: 0x00 0x00 0x00` or `0xFF 0xFF 0xFF`**:
  * The SPI pins are wired incorrectly. Double check MOSI (D11), MISO (D12), and SCK (D13).
  * Ensure the CS pin (D10) is connected securely.
  * The chip has no power. Ensure the W25Q64 is getting 3.3V on its VCC pin.
* **The JEDEC ID is read correctly, but writing records fails (reads back 0xFF)**:
  * The Write Enable (WREN) command was not accepted. Ensure your level shifter is working. W25QXX chips require write-enable latches to be set before every write or erase.
* **Data wraps around or becomes corrupted when writing long arrays**:
  * > [!WARNING]
    > **Page Boundary Wraparound**: The W25QXX Page Program command (`0x02`) cannot cross a 256-byte page boundary. For example, if you start writing a 20-byte block at address `0x0000FA` (250), the write will NOT continue to address `0x000100` (256). Instead, it wraps around to the start of Page 0 (`0x000000`), corrupting whatever was there. To log larger buffers, write a page-splitting function in your code.

## 🧠 Code Explanation

Let's break down how we perform raw memory operations on a Winbond SPI Flash chip:

### 1. The Sector Erase Cycle
```cpp
digitalWrite(FLASH_CS_PIN, LOW);
SPI.transfer(CMD_SECTOR_ERASE_4K);
SPI.transfer((address >> 16) & 0xFF);
SPI.transfer((address >> 8) & 0xFF);
SPI.transfer(address & 0xFF);
digitalWrite(FLASH_CS_PIN, HIGH);
```
- Flash memory physically cannot overwrite a `0` bit with a `1` bit. You can only flip `1`s to `0`s.
- Therefore, before writing new data, you MUST erase the memory (which flips all bits in the sector to `1`, or `0xFF`).
- We send the Erase command (`0x20`) followed by a 24-bit address (`A23` down to `A0`), chopped into three 8-bit chunks using bit-shifting.

### 2. Writing C++ Structs directly to Silicon
```cpp
void writeBytes(uint32_t address, uint8_t* buffer, int length)
// ...
TelemetryRecord record1 = { 1000, 24.57, 1, "OK_SYS" };
writeBytes(writeAddr, (uint8_t*)&record1, sizeof(record1));
```
- Instead of converting our variables into messy Comma-Separated Strings, we write the raw binary memory of our C++ `struct` directly to the flash chip!
- By casting the address of our struct to a byte pointer `(uint8_t*)&record1`, our `writeBytes` loop steps through the exact 16 bytes of RAM where the integer, float, and char array live, transferring them 1:1 onto the Flash silicon.

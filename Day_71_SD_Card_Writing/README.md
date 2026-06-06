# Day 71: SD Card Module File Writing (SPI Datalogger Basics)

Welcome to Day 71! Today we interface a standard **SPI SD Card Module** with our Arduino to establish local file writing capability. We will study the fundamentals of the **FAT filesystem**, examine how **logic level translation** works for 3.3V cards, and implement safe file operations (Open, Write, Flush, Close) to guarantee data integrity.

---

## 🎯 The "Why" and "What"

Robots and telemetry systems generate vast amounts of data (sensor streams, navigation coordinates, error codes). While internal EEPROM is useful for small configurations, it is far too small (typically 1 KB on Uno) to store long-term logs.

An **SD Card (Secure Digital) Module** allows the robot to:
1. **Datalog**: Save hours of high-frequency sensor readings.
2. **Black Box Recorder**: Log diagnostic data and state transitions to debug mechanical failures in the field.
3. **Configuration Loading**: Read complex maps, paths, or settings from files on boot instead of hardcoding them.

Writing a solid driver requires understanding how to interface with filesystems and ensure data isn't lost if power is disconnected.

---

## 🔬 Physics & Hardware Theory

### 1. FAT Filesystem & Memory Structure
SD Cards use flash memory organized in blocks (typically 512 bytes). The Arduino `SD` library maps a **FAT16** or **FAT32** (File Allocation Table) filesystem over these raw blocks.
- **FAT structure**: Contains a Master Boot Record (MBR), a partition table, the File Allocation Table (which links file clusters), and the data area.
- **Microcontroller limits**: Because the ATmega328P has only 2 KB of SRAM, it cannot load entire files into memory. The `SD` library maintains a single **512-byte cache buffer** in SRAM. When you write data, it goes into this buffer. Only when the buffer fills up, or when `flush()` / `close()` is called, is the data actually written to the physical SD card.

### 2. Logic Level Translation
SD cards operate strictly at **3.3V logic levels**. If you connect the Arduino's 5V outputs (MOSI, SCK, CS) directly to the card, the excessive voltage will blow the internal flash controller.
- **Breakout Regulator**: Most modules include a 3.3V low-dropout (LDO) regulator to step down the 5V power supply.
- **Level Shifter**: The module also includes an IC (typically a `74LVC125A` or `HEF4050`) that translates the 5V signals from the Arduino Uno to 3.3V, while keeping MISO (which goes from the SD card to the Arduino) readable by the 5V microcontroller.

```
       5V Arduino Uno                  SD Card module
┌─────────────────────────┐         ┌───────────────────┐
│     D11 (MOSI) ───5V───►├────────►│ [ Level Shifter ] │ ───3.3V───► SD Card
│     D13 (SCK)  ───5V───►├────────►│ [ 74LVC125A ]     │ ───3.3V───► SD Card
│     D10 (CS)   ───5V───►├────────►│                   │ ───3.3V───► SD Card
│                         │         │                   │
│     D12 (MISO) ◄───5V───┼◄────────┤◄───3.3V Logic ────│ ◄──3.3V──── SD Card
└─────────────────────────┘         └───────────────────┘
```

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| SD Card Module | 1 | SPI Card Reader with level shifters |
| MicroSD Card | 1 | Capacity ≤ 32 GB, formatted as FAT16 or FAT32 |
| Breadboard & Jumper Wires | 1 | Prototyping and wiring |

---

## 🔌 Pin-to-Pin Wiring

| SD Card Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **GND** | **GND** | Common Ground |
| **VCC** | **5V** | Power Supply (onboard regulator drops to 3.3V) |
| **MISO** | **D12** | SPI MISO |
| **MOSI** | **D11** | SPI MOSI |
| **SCK** | **D13** | SPI SCK |
| **CS** | **D4** | Chip Select (Configurable) |

---

## 💾 Alternatives to SD Cards

| Medium | Storage Capacity | Write Cycle Limit | Interface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **SD Card** | 512 MB – 32 GB | ~100k cycles/sector | SPI / SDIO | Removable, easy to read on PC. Heavy library overhead. |
| **EEPROM** (Internal) | 1 KB (Uno) | 100,000 writes | Registers | Tiny capacity, but zero external wiring. |
| **SPI Flash** (W25Q) | 4 MB – 32 MB | 100,000 writes | SPI | Fast, reliable, soldered on board. Needs filesystem software. |
| **FRAM** (Ferroelectric) | 8 KB – 256 KB | 100 Trillion | I2C / SPI | Virtually infinite writes, very fast, expensive. |

---

## 💻 How to Test & Validate

1. Format your MicroSD card using a PC. Ensure it is formatted as **FAT32** (for cards > 2GB) or **FAT16** (for cards ≤ 2GB). Do not use exFAT.
2. Wire the SD card module to the Arduino Uno as shown in the table.
3. Open the Arduino IDE, load [Day_71_SD_Card_Writing.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_71_SD_Card_Writing/Day_71_SD_Card_Writing.ino), and select the COM port.
4. Open the **Serial Monitor** at **9600 Baud**.
5. You should see `[SYSTEM] SD Card initialized successfully`. If initialization fails, refer to the troubleshooting guide below.
6. Type `w` in the serial input to write a log. You will see:
   `[SD] Writing data at t=4567 ms...`
   `[SD] File closed successfully.`
7. Type `r` to read the log file back. The contents will print to the Serial console.
8. Type `d` to delete the file, or `i` to force re-initialize the card.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `SD Card initialization failed!` | Card formatted as exFAT/NTFS | Reformat card as FAT32 using SD Card Formatter tool. |
| `SD Card initialization failed!` | Card capacity too large (SDXC) | Ensure card is standard SD or SDHC (≤ 32 GB). |
| `SD Card initialization failed!` | Poor/incorrect SPI wiring | Double-check connections. Ensure SPI pins match (MISO D12, MOSI D11, SCK D13, CS D4). |
| File writes successfully but data is empty | File not closed | Always call `myFile.close()` or `myFile.flush()` to commit cache buffer to physical flash. |
| Data gets corrupted during logging | Power spike or card removal | Add a decoupling capacitor (e.g. 10µF) across the module's VCC and GND. |

## 🧠 Code Explanation

Let's break down how we securely write to physical media:

### 1. The SPI Level-Shifting Interface
```cpp
// Check if card is present and can be initialized
if (!SD.begin(CS_PIN)) {
  // Initialization failed!
}
```
- SD Cards use logic levels of 3.3V, but the Arduino operates at 5.0V. We use a module with an onboard regulator and a 74LVC125A level-shifter chip to translate the 5V signals to safe 3.3V signals.
- The `SD.begin()` function sends a raw SPI "wake up" command to the card. If the card isn't physically inserted, or the FAT file system is corrupted, it returns false, letting us know it's unsafe to proceed.

### 2. Physical Writes and File Flushing
```cpp
myFile.print(timestamp);
myFile.flush();
myFile.close();
```
- Writing data to an SD card is physically very slow compared to the Arduino's CPU. The `SD.h` library uses a 512-byte SRAM cache. When we call `.print()`, it actually just writes to this fast RAM cache.
- If power is pulled before the cache is written to the physical SD card, data is permanently lost.
- By explicitly calling `flush()` and `close()`, we force the Arduino to halt and physically commit the SRAM cache to the non-volatile SD flash memory, ensuring perfect data integrity!

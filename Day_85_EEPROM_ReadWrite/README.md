# Day 85: EEPROM Memory Reading/Writing (Persistent Configuration Manager)

Welcome to Day 85! Today we master **non-volatile parameter serialization**. We will write a structured configuration system that saves multiple variables (floats, integers, and character arrays) to the Arduino's built-in **EEPROM** using `EEPROM.put()` and `EEPROM.get()`. We will study the physics of **floating gate transistors**, compile-time struct alignment, and implement a validation checksum to guarantee data integrity across reboots.

---

## 🎯 The "Why" and "What"

Microcontrollers wipe their SRAM memory clean every time they lose power.
- **The Problem**: If you tune a robot's PID gains (like on Day 77) or calibrate IMU offsets (like on Day 74), these parameters will be lost on reboot. Re-uploading code or recalibrating every time is unacceptable.
- **The Solution**: Store configuration settings in the onboard Electrically Erasable Programmable Read-Only Memory (**EEPROM**). Unlike SRAM, EEPROM holds its state without power.

By packing parameters into a single struct, we can write a uniform configuration block, complete with a checksum to verify that the memory hasn't faded over years of deployment.

---

## 🔬 Physics & Hardware Theory

### 1. Floating Gate Transistor Physics
EEPROM memory is constructed from arrays of floating gate metal-oxide-semiconductor field-effect transistors (FGMOS):
- **The Floating Gate**: A conductive gate completely insulated by a high-resistance silicon dioxide ($SiO_2$) layer. It has no physical electrical connections.
- **Programming (Tunneling)**: To write a '0', a high voltage (e.g. 12-20V, generated internally by an onboard charge pump) is applied. Electrons tunnel through the oxide layer into the floating gate (Fowler-Nordheim Tunneling) where they become trapped.
- **Erasing**: The polarity is reversed, forcing the trapped electrons back out of the floating gate.
- **State Reading**: The trapped negative charge generates an electric field that modifies the threshold voltage of the transistor. The chip senses this threshold shift to read the bit as a '0' (programmed) or a '1' (erased).

```
                 Control Gate
             ┌──────────────────┐
             │      Metal       │
             ├──────────────────┤
             │   Insulator      │
             ├──────────────────┤
             │  Floating Gate   │ (Traps electrons here)
             ├──────────────────┤
             │   Insulator      │
 ┌───────────┴──────────────────┴───────────┐
 │   Source (N+)            Drain (N+)      │
 └──────────────────────────────────────────┘
```

### 2. Struct Serialization & Byte Alignment
To save an entire struct of different variables, we must serialize (flatten) the structure into a contiguous array of bytes.
- **AVR Alignment**: In 32-bit systems (like ARM Cortex), the compiler inserts empty padding bytes to align variables to 4-byte boundaries. In 8-bit AVR microcontrollers, variables are packed strictly on **1-byte boundaries**. There is zero padding.
- **Contiguous Address Space**: Our `SystemConfig` struct size is:
  - `bootCount`: 4 bytes
  - `sensorMultiplier`: 4 bytes
  - `sensorOffset`: 4 bytes
  - `systemName`: 16 bytes
  - `checksum`: 2 bytes
  - **Total Size** = $4 + 4 + 4 + 16 + 2 = 30\,\text{bytes}$.
- `EEPROM.put()` handles serializing this contiguous block, writing all 30 bytes sequentially starting from address 0.

### 3. Checksum Integrity Validation
Flash and EEPROM memories can suffer from **data retention fading** over 10-20 years as trapped electrons slowly leak back through the oxide barrier.
To prevent the MCU from loading corrupted values, we calculate a **checksum** of the variables before saving, and store it inside the struct.
During `setup()`, the structure is loaded. The CPU calculates the checksum again. If the calculated checksum does not match the stored checksum, the data is flagged as corrupted, and the system falls back to safe factory default values.

---

## 🔩 Components Needed

No external components are required. The project runs on the Arduino's internal hardware registers and utilizes the Serial Monitor.

---

## 🔌 Pin-to-Pin Wiring

No external wiring is required.

---

## 💾 Non-Volatile Memory Alternatives

| Medium | Storage Capacity | Write Endurance | Interface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **EEPROM (Internal)**| 1 KB (Uno) | 100,000 writes | Registers | Built-in, low capacity. |
| **External EEPROM**  | 2 KB - 256 KB | 1,000,000 writes| I2C / SPI | Soldered IC (e.g. 24LC256). Very reliable. |
| **SPI Flash**        | 4 MB - 128 MB | 100,000 writes | SPI | High capacity. Requires page/sector erase blocks. |
| **FRAM (Ferroelectric)**| 8 KB - 256 KB | 100 Trillion | I2C / SPI | Fastest. Infinite cycles. Retains data via magnetic domains. |

---

## 💻 How to Test & Validate

1. Upload [Day_85_EEPROM_ReadWrite.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_85_EEPROM_ReadWrite/Day_85_EEPROM_ReadWrite.ino) to the Arduino Uno.
2. Open the **Serial Monitor** at **9600 Baud**.
3. **Boot Cycle Log**:
   - On the first boot, the system registers uninitialized memory, defaults to factory variables, increments `bootCount` to 1, and saves them.
   - Press the Reset button on the Arduino. The system reboots.
   - The monitor prints: `[BOOT] Boot cycle #2 logged successfully`. The count persists!
4. **Editing Calibration Configuration**:
   - Save new settings: Type `s 2.50 -0.05 Rover-Beta` and press Enter.
   - The CLI parses the parameters, calculates a fresh checksum, writes them to EEPROM, and prints the updated state:
     ```text
     [SYSTEM] Configuration saved to EEPROM.
     --------------------------------------
      System Name : Rover-Beta
      Boot Count  : 2
      Multiplier  : 2.5000
      Zero Offset : -0.0500
      Checksum    : 0x2A1
     --------------------------------------
     ```
   - Disconnect the USB cable to cut off power completely.
   - Reconnect the USB cable and open the Serial Monitor.
   - You will see the system boot, report cycle #3, and correctly load the values (`Rover-Beta`, `2.5000`, `-0.0500`) from memory!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| System loads default values on every boot | Checksum mismatch or uninitialized memory | Ensure you call `saveConfiguration()` after updating parameters to write the correct checksum to EEPROM. |
| Boot count resets to 0 | `resetToFactoryDefaults()` triggered | If you clear the EEPROM manually or power fluctuates, the checksum check fails, and the system reloads defaults. |
| Char array (System Name) prints garbage characters | Missing null terminator | Ensure string copies leave room for the null terminator (`\0`). Our buffer is 16 bytes, limiting the string to 15 characters. |
| Float values lose decimal accuracy | Single-precision limits | Float variables on AVR are limited to 32-bits (6-7 decimal places of precision). |

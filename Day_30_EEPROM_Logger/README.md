# Day 30: EEPROM Data Logger (Wear-Leveling Ring Buffer)

Welcome to Day 30 of the 100-Day Arduino Masterclass! Today, we study non-volatile memory management on microcontrollers. We will interface the Arduino's internal **EEPROM** (Electrically Erasable Programmable Read-Only Memory) to build a data logger. Because EEPROM has a limited write life, we will design and implement a professional **Wear-Leveling Ring Buffer** with **Integrity Checksums** to maximize the lifespan of the hardware.

---

## 🎯 The "Why" and "What"

In robotics and automation systems, critical parameters (like calibration offsets, network configurations, serial numbers, error states, and history logs) must survive system power cycles. 
* **The Problem:** Writing data to the same memory cell repeatedly degrades it. If we log sensor values directly to address `0x00` every few seconds, the memory gate will wear out and fail in less than 2 weeks!
* **The Solution:** A **Wear-Leveling Ring Buffer** distributes writes sequentially across the entire 1024-byte space of the EEPROM. Instead of burning one memory location, we cycle through all 128 available memory slots, increasing the operational lifetime of the device by $128 \times$ (from days to years).

---

## 🔬 Physics & Hardware Theory

### 1. Flash and EEPROM Transistor Physics
EEPROM is made of arrays of **Floating Gate Transistors**. 
* Unlike standard transistors where the gate is connected directly to a trace, a floating gate is sandwiched inside a sandwich of silicon dioxide insulator.
* **Programming (Write):** A high voltage is applied to the control gate, forcing electrons through the insulating oxide layer into the floating gate via a quantum mechanics process called **Fowler-Nordheim Tunneling**. The trapped negative electrons shift the threshold voltage of the transistor, which registers as a logic `0` (or programmed state).
* **Erasing:** Reversing the voltage pulls the trapped electrons back out of the floating gate, registering as a logic `1` (erased state).
* **The Limit:** Each write/erase cycle forces electrons through the silicon dioxide barrier. Over time, the oxide breaks down, trapping permanent charges or creating leaks. This limits standard microchip EEPROMs to **$\approx 100,000$ write cycles** per byte before the memory cell fails.

---

### 2. Wear-Leveling Algorithm
Instead of writing to a single address:
1. We segment the 1024-byte EEPROM into 128 slots of 8 bytes each.
2. Each slot contains a structured packet: `[4-byte LogID] + [2-byte SensorValue] + [2-byte Checksum]`.
3. To write a new entry, we locate the slot with the highest `LogID`, move to the next slot index:
   $$\text{Next Slot} = (\text{Newest Slot} + 1) \pmod{128}$$
4. If we reach the end of the memory boundary (Slot 127), the index wraps back to Slot 0, overwriting the oldest logs.
5. On boot, the system scans the entire memory block. It verifies the checksum of each slot to skip blank/corrupt fields and automatically re-points to the newest entry.

```
EEPROM Address Map:
[ Slot 00 ] -> LogID: 101 | Val: 452 | Checksum: OK
[ Slot 01 ] -> LogID: 102 | Val: 461 | Checksum: OK  <-- Newest Active Log
[ Slot 02 ] -> LogID: 000 | Val: 000 | Checksum: Invalid (Erased 0xFF)
[ Slot 03 ] -> LogID: 000 | Val: 000 | Checksum: Invalid (Erased 0xFF)
...
[ Slot 127 ] -> LogID: 100 | Val: 432 | Checksum: OK
```

---

## 🔄 Alternatives Comparison

When choosing non-volatile memory for data loggers:

| Storage Type | Capacity | Write Endurance | Interface | Hardware Cost | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Internal EEPROM** | **$1\text{ KB}$ (Uno)** | **$100\text{k}$ cycles** | **Internal Register** | **$0.00 (Built-in)** | **Calibration coefficients, system states, small log files (Our choice)** |
| **External EEPROM (24LC256)** | **$32\text{ KB}$** | **$1\text{M}$ cycles** | **I2C ($400\text{ kHz}$)** | **Low ($\approx \$1$)** | **Extended configurations, calibration tables** |
| **SPI Flash (W25Q32)** | **$4\text{ MB}$** | **$100\text{k}$ cycles** | **SPI ($104\text{ MHz}$)** | **Low-Medium ($\approx \$1.50$)** | **Storing graphics/audio, firmware updates** |
| **SD Card Module** | **$4\text{ GB} - 32\text{ GB}$** | **Millions (FAT controllers)**| **SPI** | **Medium ($\approx \$3$)** | **Massive data streams, high-frequency logging** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x Potentiometer or Photoresistor (optional, used to feed changing values into the analog logging pin A0)
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

No external components are strictly required. If you want to connect a Potentiometer to A0 for simulated sensor logging:

| Potentiometer Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **Pin 1 (GND)** | **GND** | Black | Ground Reference |
| **Pin 2 (Wiper)** | **A0** | Green | Analog Output Voltage |
| **Pin 3 (VCC)** | **5V** | Red | Power |

---

## 💻 How to Test & Validate

1. Upload `Day_30_EEPROM_Logger.ino` to the Arduino Uno.
2. Open the **Serial Monitor** at **9600 Baud**.
3. The setup routine will scan the memory. If the EEPROM is brand new, you will see:
   `[SYSTEM] Boot scan finished. No valid records found (EEPROM empty or cleared).`
4. Interact using the **Serial Command Input Bar**:
   * **Write Log:** Type `w` and press Enter.
     * The console outputs: `[LOG] Saved to Slot 0 (Address: 0) | ID: 1 | Value: 345`.
     * Type `w` again to write another. It will write sequentially to Slot 1, then Slot 2.
   * **Read Logs:** Type `r` and press Enter.
     * This scans the EEPROM and prints out all valid logged entries chronologically.
   * **Wipe EEPROM:** Type `c` and press Enter.
     * This runs `EEPROM.update()` across the address space, resetting the cells to `0xFF`.
   * **Test Persistence:** Write 3 logs, then press the physical Reset button on your Arduino (or unplug the USB). Open the Serial Monitor again. The boot scan will find your logs, identify the newest slot, and set the write head to the next slot (Slot 3)!

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **Serial prints random garbage characters on start:**
  * Double check that the Serial Monitor baud rate is set exactly to **9600 Baud**.
* **Boot scan reports `Found valid records` immediately on a brand new board:**
  * Manufacturers often test boards before shipping, which leaves random values in the EEPROM. Type `c` in the serial prompt to execute a format pass.
* **The analog sensor values are constant or floating erratically:**
  * If nothing is wired to A0, the pin acts as an antenna and picks up ambient static noise (values between 0 and 1023). This is normal. Hook up a potentiometer to A0 to log controlled voltage values.

# Day 72: CSV Datalogger (Sensors to SD Card with Safe Toggle/Eject)

Welcome to Day 72! Today we build a high-reliability **CSV (Comma-Separated Values) Sensor Datalogger**. We integrate multiple analog sensors (an LDR and a Potentiometer), map the raw readings to physical units (Voltage), write standard CSV-formatted log files, and implement a **safe shutdown/eject button** using a non-blocking debounced input state machine.

---

## 🎯 The "Why" and "What"

A raw text logger is fine for basic debugging, but for engineering applications, data needs to be structured.
- **CSV Format**: By structuring data as comma-separated columns with a header row, we create files that can be directly imported into analytical tools like **MATLAB, Python (Pandas), or Microsoft Excel** for plotting and regression.
- **Safe Card Ejection**: If a microcontroller is in the middle of writing a block to the SD Card and power is lost or the card is removed, the FAT partition table can get corrupted, bricking the filesystem. By adding a physical toggle button, we can instruct the Arduino to flush its cache, write a footer, close file handles, and turn off indicator lights — telling the user it is **safe to eject the card**.

---

## 🔬 Physics & Hardware Theory

### 1. CSV Data Formatting
A CSV file is a text file where each line is a record, and fields are separated by a delimiter (typically a comma).
Our format is:
`Timestamp_ms,LDR_Raw,Pot_Raw,Voltage_V`
For example:
`23450,412,512,2.50`
Writing files this way allows us to read columns directly into matrices:
$$\mathbf{X} = \begin{bmatrix} 1000 & 400 & 100 & 0.49 \\ 2000 & 410 & 200 & 0.98 \\ 3000 & 420 & 300 & 1.47 \end{bmatrix}$$

### 2. Button Debouncing Physics & Logic
A tactile pushbutton consists of two metal contacts. When pressed, the contacts do not touch instantly; they slide and bounce against each other for several milliseconds before establishing solid contact. The microcontroller (running at 16 MHz) can register these mechanical bounces as dozens of individual presses.

To solve this without blocking execution (i.e. without using `delay()`), we use a software **debounce timer**:
- We poll the button pin. If the electrical state changes, we store the current timestamp (`millis()`).
- We only register a state change if the pin remains stable for at least `50 ms` (defined as `DEBOUNCE_DELAY_MS`).
- This filters out high-frequency contact noise:

```
Button Pin Voltage
  5V  ───┐     ┌─┐ ┌──┐ ┌─────────────────
         │     │ │ │  │ │
  0V     └─────┘ └─┘  └─┘
         ◄──────Bouncing►◄────Stable LOW──►
                        ▲
                     Register Pressed here (after 50ms)
```

### 3. LDR Voltage Divider Physics
The Photoresistor (LDR) changes its resistance ($R_{LDR}$) depending on ambient light intensity:
- Dark: $R_{LDR} \approx 1\,\text{M}\Omega$
- Light: $R_{LDR} \approx 10\,\text{k}\Omega$ or less.

Since the Arduino ADC measures voltage, not resistance, we construct a **voltage divider** with a fixed $10\,\text{k}\Omega$ resistor ($R_{fixed}$):
$$V_{out} = V_{in} \cdot \left(\frac{R_{fixed}}{R_{LDR} + R_{fixed}}\right)$$
As light increases, $R_{LDR}$ decreases, pushing $V_{out}$ closer to $V_{in}$ (5V). This increases the ADC reading.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Controller |
| SD Card Module | 1 | SPI Datalogger Interface |
| MicroSD Card | 1 | Formatted FAT16/FAT32 |
| Tactile Pushbutton | 1 | Toggle logging / Safe Eject |
| Photoresistor (LDR) | 1 | Light sensor |
| 10 kΩ Potentiometer | 1 | Voltage/Speed Simulation |
| 10 kΩ Resistor | 1 | LDR voltage divider resistor |
| Green LED | 1 | Logging state indicator |
| 220 Ω Resistor | 1 | LED current limiting |
| Breadboard & Wires | 1 | Circuit assembly |

---

## 🔌 Pin-to-Pin Wiring

| SD Card Pin | Sensor / LED Pin | Arduino Pin | Description |
| :--- | :--- | :--- | :--- |
| **GND** | Sensor GND / LED Cathode | **GND** | Common Ground |
| **VCC** | Potentiometer Pin 1 / LDR Divider VCC | **5V** | Power rail |
| **MISO** | - | **D12** | SPI MISO |
| **MOSI** | - | **D11** | SPI MOSI |
| **SCK** | - | **D13** | SPI SCK |
| **CS** | - | **D4** | SPI CS |
| - | **Button Pin 1** | **D2** | Start/Stop button (Internal Pullup) |
| - | **Button Pin 2** | **GND** | Button ground |
| - | **LED Anode** | **D5** | Status LED (via 220Ω resistor) |
| - | **LDR/10kΩ Junction** | **A0** | LDR Analog input |
| - | **Potentiometer Wiper** | **A1** | Potentiometer wiper input |

---

## 💾 Alternatives to CSV Data Formats

| Format | Readability | File Size | Arduino Processing Cost | PC Integration |
| :--- | :--- | :--- | :--- | :--- |
| **CSV** | Human & Machine | Small | Very Low (simple text prints) | Direct (Pandas, Excel, MATLAB) |
| **JSON** | Human & Machine | Large | High (needs JSON parsing library) | Great (Web apps, Python) |
| **Binary** | Machine Only | Extremely Small | Zero (direct memory writes) | Needs custom script to decode |
| **XML** | Human & Machine | Extremely Large | Very High (unsuitable for Uno) | Great (Legacy web systems) |

---

## 💻 How to Test & Validate

1. Assemble the circuit on the breadboard.
2. Load [Day_72_CSV_Datalogger.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_72_CSV_Datalogger/Day_72_CSV_Datalogger.ino) onto the Arduino Uno.
3. Open the **Serial Monitor** at **9600 Baud**.
4. The SD Card should initialize, and the console will state: `[STATUS] Press Button D2 or type 't' to START logging.`
5. Press the physical button on Pin 2 or type `t` in the serial terminal:
   - The Green LED on Pin 5 will start blinking at 1 Hz.
   - The Serial Monitor will dump the values logged every second.
6. Shine a light on the LDR or turn the potentiometer; watch the values shift in real-time.
7. Press the button again to halt logging. The Green LED will turn off. The console prints: `[LOGGER] >>> LOGGING STOPPED (Safe to eject SD Card) <<<`.
8. Type `r` in the serial terminal to output the entire contents of the CSV file. Verify the formatting and header row match perfectly.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Datalogger fails to boot; LED flashes rapidly | SD Card not found | Verify SPI connections. Check if the card is formatted as FAT32. |
| Button toggle doesn't work | No ground or wrong pin | Verify button is connected between D2 and GND. |
| Button toggles continuously when pressed once | Switch bounce | Check if debounce time is set correctly (50ms). |
| LDR raw value is stuck at 0 or 1023 | Voltage divider wired incorrectly | Ensure the LDR is in series with a 10kΩ resistor, with the signal A0 tapped between them. |
| CSV header row prints multiple times | Board resetting or file formatted | The sketch only writes the header if the file does not exist. Format the card or delete the file to trigger a clean header write. |

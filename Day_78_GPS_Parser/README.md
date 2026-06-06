# Day 78: GPS Module (NEO-6M) Parsing (Raw NMEA Parser)

Welcome to Day 78! Today we interface the popular **NEO-6M GPS Module** using **SoftwareSerial** and decode coordinate telemetry from scratch — **no external GPS libraries allowed!** We will write a custom **NMEA-0183 string parser**, implement an XOR checksum validator, and write coordinates in floating-point Decimal Degrees.

---

## 🎯 The "Why" and "What"

Global Positioning System (GPS) receivers are critical in long-range autonomous robotics:
1. **Outdoor Navigation**: Autonomous agricultural robots, drone waypointing, and marine surface vehicles rely on GPS to cross fields, fly routes, or traverse oceans.
2. **Time Synchronization**: GPS satellites carry atomic clocks. A receiver outputs highly precise UTC time, enabling robots to sync local clocks across networks.
3. **Telemetry Logs**: Log coordinates to file (which we did on Day 72) to trace physical pathways on digital maps (like Google Earth).

Rather than calling pre-written library methods, parsing the raw string streams ourselves is highly educational, teaching us how to parse serial text frames, calculate checksums, and convert navigational formats.

---

## 🔬 Physics & Hardware Theory

### 1. GPS Triangulation Physics
A GPS receiver listens to RF signals at $1.57542\,\text{GHz}$ (L1 band) transmitted by a constellation of satellites. By calculating the exact time of flight of signals from at least **four satellites** (triangulation / trilateration), the receiver solves for four unknowns: Latitude ($X$), Longitude ($Y$), Altitude ($Z$), and Clock Bias offset ($T$).

### 2. The NMEA-0183 Standard
GPS modules stream ascii characters over a standard serial interface (UART) using standard **NMEA-0183** sentences. Every sentence starts with a `$` and ends with a carriage return and newline (`\r\n`).
We target two primary sentence types:
- **`$GPRMC` (Recommended Minimum Navigation Information)**:
  Contains time, fix status (Active vs Void), latitude, longitude, speed, date, and hemisphere indicators.
- **`$GPGGA` (Global Positioning System Fix Data)**:
  Contains 3D location fixes, fix quality, and the number of satellites tracked.

```
$GPRMC,123519.00,A,4807.0380,N,01131.0000,E,022.4,084.4,230326,,,A*6C
  ▲       ▲      ▲    ▲      ▲    ▲       ▲               ▲     ▲
Header   UTC    OK   Lat    N   Lon       E              Date Checksum
```

### 3. Checksum Validation Math
The two hexadecimal characters after the `*` represent the XOR checksum of all characters between the `$` and the `*`.
$$\text{Checksum} = C_1 \oplus C_2 \oplus C_3 \oplus \dots \oplus C_n$$
We calculate the XOR sum of all bytes in the string and compare it against the target checksum to filter out transmission noise.

### 4. Coordinate Conversion Mathematics
NMEA coordinates are formatted as `DDMM.MMMM` (Degrees and Minutes):
- **Latitude `4807.0380`**:
  - Degrees ($D$) = `48`
  - Minutes ($M$) = `07.0380`
  - Conversion to Decimal Degrees:
    $$\text{Decimal Degrees} = D + \frac{M}{60.0} = 48 + \frac{7.0380}{60.0} = 48.11730^\circ$$
- **longitude `01131.0000`**:
  - Degrees ($D$) = `11`
  - Minutes ($M$) = `31.0000`
  - Conversion:
    $$\text{Decimal Degrees} = 11 + \frac{31.0000}{60.0} = 11.51667^\circ$$
- **Hemispheres**: If the hemisphere indicator is **South** ($S$) or **West** ($W$), we multiply the result by $-1$.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Master Controller |
| NEO-6M GPS Module | 1 | GPS Receiver (built-in ceramic patch antenna) |
| 4.7 kΩ Resistor | 1 | Voltage divider (GPS RX pin protection) |
| 10 kΩ Resistor | 1 | Voltage divider (GPS RX pin protection) |
| Breadboard & Wires | 1 | Circuit assembly |

> ⚠️ **CRITICAL: The NEO-6M logic pins operate at 3.3V. While the GPS TX can directly drive the Arduino Pin 2 (Rx), the Arduino Pin 3 (Tx) outputs 5V. You MUST use a resistor voltage divider (4.7kΩ and 10kΩ) on the GPS RX line to protect it from damage.**

---

## 🔌 Pin-to-Pin Wiring

| NEO-6M Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** (or 3.3V) | Power supply |
| **GND** | **GND** | Ground |
| **TX** | **D2** | Software RX (direct connection) |
| **RX** | **D3** (via divider) | Software TX (D3 -> 4.7kΩ -> GPS RX -> 10kΩ -> GND) |

---

## 💾 Alternatives to NEO-6M GPS Module

| Module | Satellites Supported | Refresh Rate | Interface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **NEO-6M** | GPS only | 1 Hz - 5 Hz | UART | Cheap, standard ceramic antenna, widely available. |
| **U-Blox NEO-M8N** | GPS, GLONASS, BeiDou | 10 Hz | UART / I2C | High-precision dual-constellation module. |
| **Adafruit Ultimate GPS**| GPS, WAAS | 10 Hz | UART | MTK3339 chipset, built-in datalogger memory. |
| **Quectel L80-M39** | GPS, QZSS | 10 Hz | UART | Small patch-on-top integrated GPS module. |

---

## 💻 How to Test & Validate

1. Wire the NEO-6M GPS module to the Arduino Uno as shown in the table. **Ensure the resistor divider is in place on the RX line.**
2. Place the GPS antenna near a window or outside. Ceramic patch antennas need a direct line-of-sight to the sky. It can take up to 2-5 minutes to establish a lock (indicated by a blinking red LED on the GPS board).
3. Upload [Day_78_GPS_Parser.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_78_GPS_Parser/Day_78_GPS_Parser.ino).
4. Open the **Serial Monitor** at **9600 Baud**.
5. Once a satellite stream begins, raw decoded values will print to the console.
6. **Simulation Test (No Hardware Needed)**:
   - If no GPS is attached, copy this valid fix string:
     `$GPRMC,123519.00,A,1258.9876,N,07735.1234,E,000.0,000.0,040626,,,A*6C`
   - Paste it into the Serial Monitor input bar and press Enter.
   - The shell will parse it, validate the checksum `6C`, convert the coordinates, and print:
     ```text
     [SHELL] Injecting sentence: $GPRMC,123519.00,A,1258.9876,N,07735.1234,E,000.0,000.0,040626,,,A*6C
     ----------------- GPS DECODED TELEMETRY -----------------
      UTC Time   : 12:35:19
      Date       : 04-06-2026
      Fix Status : VALID (Lock Established)
      Latitude   : 12.983126°
      Longitude  : 77.585390°
      Satellites : 0
     ---------------------------------------------------------
     ```
   - Inject satellite count info: Paste `$GPGGA,123519.00,1258.9876,N,07735.1234,E,1,08,0.9,105.5,M,-30.0,M,,*41` and watch Satellites update to `8`.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Telemetry reports `VOID` indefinitely | No satellite lock | Ceramic patch antennas cannot see through concrete walls. Place the antenna outside or close to a window. |
| Console output is empty (no serial bytes) | SoftwareSerial pins swapped | Ensure GPS TX connects to D2 (RX) and GPS RX connects to D3 (TX). |
| Decoded latitude/longitude coordinates are zero | No active lock | Coordinate data is only parsed when the status byte in GPRMC is `'A'` (Active). Wait for the red LED on the GPS board to blink. |
| `Checksum failed!` warnings in console | Serial noise or baud rate mismatch | Keep wires short. Ensure the software baud rate is configured to `9600`. |

## 🧠 Code Explanation

Let's break down how we parse raw satellite data from the cosmos:

### 1. Serial Stream Buffering and Sentence Extraction
```cpp
if (c == '$') {
  isRecording = true;
} else if (isRecording && (c == '' || c == '
')) {
  // Parse complete sentence
}
```
- GPS modules blindly blast text data out of their TX pin at 9600 baud.
- Instead of using a library, we built a Ring Buffer. We ignore everything until we see the start character (`$`). We then record every character into a character array (`sentenceBuffer`) until we see a newline (`
`). 
- We now have a perfectly isolated NMEA sentence string ready for analysis!

### 2. Tokenizing and Coordinate Translation
```cpp
int degrees = (int)(rawValue / 100.0f);
float minutes = rawValue - (degrees * 100.0f);
float decimalDegrees = degrees + (minutes / 60.0f);
```
- The `$GPRMC` sentence gives us coordinates in "Degrees-Minutes" format (e.g., `4807.038`).
- Google Maps and modern APIs require "Decimal Degrees".
- Because the first two (or three) digits are Degrees and the remaining digits are Minutes, we divide by 100 and cast to an `int` to cleanly slice off the Degrees. We then divide the remaining Minutes by 60 to convert them to a decimal, yielding the exact Decimal Degree format!

# Day 49: GPS Telemetry Decoder (Raw NMEA Parser)

Welcome to Day 49 of the 100-Day Arduino Masterclass! Today, we transition to absolute positioning systems by interfacing a **NEO-6M GPS Receiver Module**. To build a deep understanding of standard geospatial telemetry protocols, we will write a **raw NMEA-0183 protocol parsing engine from scratch** (no external GPS libraries), implement a **hardware XOR checksum verifier**, and convert raw coordinate strings into standard decimal degrees.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/GPS_Module.jpg" alt="GPS Module" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

Robots using wheel encoders or IMUs suffer from cumulative drift over time (dead reckoning error). 
* **The Problem:** To travel long distances or navigate outdoor courses, a robot needs an absolute coordinate frame. While GPS modules process satellite signals internally and output standard coordinates, they transmit this data as a continuous, dense stream of ASCII text characters (NMEA sentences). We must parse this stream in real-time, non-blockingly, and verify its integrity before using it for steering.
* **The Solution:** We interface the GPS over a software serial port and build a byte-level state machine. The parser scans the incoming data stream, matches headers (`$GPRMC` and `$GPGGA`), calculates and verifies the checksum, and performs coordinate conversions to extract real-world decimal coordinates.

---

## 🔬 Physics & Mathematics

### 1. GPS Trilateration Physics
A GPS receiver determines its position by measuring the time delay of radio signals traveling at the speed of light ($c \approx 3 \times 10^8\,\text{m/s}$) from at least four overhead GPS satellites. The distance to satellite $i$ is:
$$\text{Distance}_i = c \cdot (t_{\text{receive}} - t_{\text{transmit}})$$

Using the known coordinates of the satellites, the receiver solves a set of sphere equations (trilateration) to find its Latitude, Longitude, Altitude, and clock bias.

---

### 2. XOR Checksum Verification
To prevent corrupted bits from feeding false coordinates to the navigation controller, every NMEA sentence concludes with an asterisk followed by a 2-character hexadecimal checksum (e.g., `*68`).
The checksum is calculated by performing a bitwise Exclusive-OR (XOR) on all characters in the sentence between the starting `$` and the ending `*`:
$$\text{Checksum} = \text{Char}_1 \oplus \text{Char}_2 \oplus \text{Char}_3 \dots \oplus \text{Char}_n$$

```
 NMEA Sentence:   $ G P R M C , 2 2 5 4 4 6 , A ... * 6 8
                  | \___________________________/ | \_/
                Start       Data Payload        End  Checksum
```

If the calculated XOR does not match the expected hexadecimal value converted to an integer, the packet is discarded as corrupted.

---

### 3. NMEA Coordinate Math
GPS modules output latitude and longitude in `DDMM.MMMM` (Degrees and Minutes) format. For example:
* **Raw Latitude:** `1302.3456, N`
* **Raw Longitude:** `08015.6789, E`

To plot this on standard mapping platforms (like Google Maps), we must convert this to **Decimal Degrees**:
$$\text{Decimal Degrees} = \text{Degrees} + \frac{\text{Minutes}}{60}$$

#### Step-by-step Latitude Calculation:
1. Split `1302.3456` into Degrees (`13`) and Minutes (`02.3456`).
2. Divide minutes by 60:
   $$\frac{02.3456}{60.0} = 0.039093$$
3. Add degrees:
   $$13 + 0.039093 = 13.039093^\circ$$
4. Since direction is `N` (North), the sign remains positive. (If it were `S` (South), we would multiply by $-1.0$).

---

## 🔄 GPS Config Comparison

| Positioning Tech | Accuracy (RMS) | Update Rate | Relative/Absolute | Best Used For |
| :--- | :--- | :--- | :--- | :--- |
| **Standard GPS (NEO-6M)** | **$2.5\,\text{m} - 5\,\text{m}$** | **$1\,\text{Hz} - 5\,\text{Hz}$** | **Absolute (Global)** | **Standard vehicle tracking, outdoor rovers (Our choice)** |
| **RTK GPS (Real-Time Kinematic)** | $1\,\text{cm} - 2\,\text{cm}$ | $5\,\text{Hz} - 20\,\text{Hz}$ | Absolute (Global) | Precision agriculture, autonomous mapping drones |
| **UWB (Ultra-Wideband)** | $10\,\text{cm}$ | $50\,\text{Hz}$ | Relative (Local anchors) | Indoor warehouse robot navigation |
| **Optical Flow / VIO** | Drift-prone | $>100\,\text{Hz}$ | Relative (Local) | GPS-denied drone flight stability |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x NEO-6M GPS Module (with external ceramic antenna)
* 1x Breadboard & Jumper wires
* **1x $1\,\text{k}\Omega$ Resistor** and **1x $2\,\text{k}\Omega$ Resistor** (to build a 3.3V logic shifter for the GPS Rx pin)

---

## 🔌 Pin-to-Pin Wiring

> [!IMPORTANT]
> The NEO-6M GPS receiver operates on **3.3V logic**. The Arduino TX pin outputs 5V. To protect the GPS RX line, you **MUST** install a voltage divider to step down the TX voltage to 3.3V.

```
 Arduino D10 (TX) ----[ 1k Ohm Resistor ]----+---- GPS RXD
                                             |
                                      [ 2k Ohm Resistor ]
                                             |
                                            GND
```

### Wiring Table
| GPS Module Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **5V** (or 3.3V) | Red | Power input (NEO-6M has onboard regulator) |
| **GND** | **GND** | Black | Common Ground |
| **TXD** | **D9** | Green | Transmit Data (5V safe for Arduino Rx) |
| **RXD** | **D10** (via Divider) | Yellow | Receive Data (Stepped down to 3.3V) |

---

## 💻 How to Test & Validate

1. **Setup the Antenna**:
   * Connect the active ceramic antenna to the GPS module.
   * **Indoor Signal Blocks**: GPS signals cannot penetrate thick concrete ceilings. Place the antenna on a windowsill or test outside with a laptop.
2. **Upload & Monitor**:
   * Upload [Day_49_GPS_Decoder.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_49_GPS_Decoder/Day_49_GPS_Decoder.ino).
   * Open the **Serial Monitor** at **9600 Baud**.
   * On boot, the monitor will show: `[GPS] Waiting for satellites 3D lock...`
3. **Observe Lock Indicators**:
   * The NEO-6M has a red status LED. When searching for satellites, the LED remains solid. Once it establishes a lock (needs 3-4 satellites), the LED will start blinking (once per second).
   * As soon as a lock is established, the Serial Monitor will print a formatted summary:
     ```
     ================ GPS TELEMETRY SUMMARY ================
       UTC Time:        12:35:19
       Date:            19/11/2026
       Lock Status:     3D FIX ACTIVE (Lock established)
       Latitude:        13.039093°
       Longitude:       80.261315°
       Altitude:        545.4 m
       Speed:           0.93 km/h
       Satellites:      8
     =======================================================
     ```

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The serial monitor keeps printing "NO FIX" indefinitely**:
  * The antenna has no clear view of the sky. Move closer to a window or outdoors. First lock can take up to 2-3 minutes from a cold start.
  * Check the red LED on the GPS module. If it is not flashing, the hardware does not have a satellite lock yet.
* **The serial monitor remains completely blank or outputs random characters**:
  * Verify the baud rate is set to 9600 in both the Serial Monitor and code.
  * Check the wiring. If RX and TX are swapped (D9/D10), no sentences will be received.
* **Warning outputs like "Checksum mismatch! Corrupted sentence discarded"**:
  * Occasional checksum warnings are normal due to electrical noise on breadboards.
  * If warnings occur continuously, the logic levels are unstable. Check that your GND connection is shared and solid, and ensure your voltage divider resistors are placed correctly.

## 🧠 Code Explanation

Let's break down how to decode raw satellite telemetry without libraries:

### 1. The Byte-Level State Machine
```cpp
if (c == '$') {
    isCapturing = true;
} else if (isCapturing) {
    if (c == '\n') { processCompleteSentence(); }
    else { sentenceBuffer[bufferIndex++] = c; }
}
```
- The GPS module screams raw text at the Arduino continuously at 9600 baud.
- A standard NMEA sentence looks like: `$GPRMC,225446,A,4916.45,N,12311.12,W...*68`
- Our parser ignores all garbage characters until it sees the Start Marker (`$`).
- It then saves every character into a buffer array until it hits the End Marker (`\n` Newline). It then instantly fires off the processing function!

### 2. Checksum Verification
```cpp
long calculatedChecksum = 0;
for (int i = 0; i < asteriskIndex; i++) {
    calculatedChecksum ^= sentenceBuffer[i];
}
```
- Radio waves get corrupted by atmospheric noise. If a coordinate digit gets flipped, our robot might think it's in the ocean!
- The GPS module calculates an XOR Hash of all the characters in the sentence and attaches it to the end (e.g., `*68`).
- Our code manually performs an XOR (`^=`) on all the characters we received. If our calculated hash doesn't perfectly match the hash attached to the sentence, we throw the corrupted data in the trash!

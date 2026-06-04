# Day 9: Temperature & Humidity Logger (DHT11 Interfacing)

Welcome to Day 9 of the 100-Day Arduino Masterclass! Today, we focus on environmental monitoring. We will learn how to interface the popular DHT11 digital sensor to capture temperature and humidity readings, parse them in code, and handle sensor errors.

You will learn the physics behind relative humidity and thermal resistances, study the DHT11's proprietary single-wire serial communication protocol, and implement a robust logging system.

---

## 🎯 Today's Learning Goals
1. Understand how capacitive elements measure relative humidity.
2. Study the behavior of Negative Temperature Coefficient (NTC) thermistors.
3. Decipher the 40-bit single-bus serial data protocol used by DHT sensors.
4. Wire the DHT11 and implement an external pull-up resistor configuration if needed.
5. Use Adafruit's DHT library and write non-blocking validation routines.

---

## 🧠 The "Why" and "What": Temperature & Humidity in Robotics

### What is the DHT11?
The DHT11 is a low-cost, basic digital sensor that outputs calibrated temperature and relative humidity values. It packages an analog humidity sensor, an NTC thermistor, and a 8-bit microcontroller inside a single blue plastic grid casing.

### Why is it Used in Robotics & Mechatronics?
Environmental factors directly affect mechanical systems, electronics, and algorithms:
- **Server Room / Battery Vent Systems:** Standard electronics enclosures and battery packs generate heat. Cooling fans are switched on via relays based on temperature/humidity thresholds.
- **Weather Stations & Agriculture:** Agricultural robots that deploy seeds, measure soil, or spray pesticides monitor air humidity and temperature to optimize growth schedules.
- **HVAC Smart Automation:** Regulating air conditioners and humidifiers based on the calculated **Heat Index** (how hot it actually feels to humans, incorporating moisture levels).
- **Sensor Drift Calibration:** High-precision gas sensors or optical sensors drift as temperature changes. Readings from a DHT11 are used as calibration parameters to mathematically correct sensor drift.

---

## ⚡ The Physics & Hardware Theory

### 1. Capacitive Relative Humidity (RH) Sensing
To measure humidity, the DHT11 uses a **capacitive humidity sensor**. This sensor consists of a thin polymer substrate (dielectric layer) sandwiched between two conductive electrodes.

```
       Capacitive Humidity Sensor Structure
       
            [ Electrode 1 (Gold/Platinum grid) ]
       --------------------------------------------
            Polymer Substrate (Moisture Absorbing)  ⬅️ Water Molecules (H2O)
       --------------------------------------------
            [ Electrode 2 (Glass substrate) ]
```

Water vapor ($\text{H}_2\text{O}$) in the air is absorbed by the polymer layer. Because water molecules have a very high dielectric constant ($\approx 80$ at room temperature) compared to the dry polymer, the capacitance ($C$) of the sensor increases proportionally as more water vapor is absorbed:

$$C = \epsilon_r \cdot \epsilon_0 \cdot \frac{A}{d}$$

Where:
* $\epsilon_r$ is the relative permittivity (dielectric constant) of the polymer/moisture mix.
* $\epsilon_0$ is the vacuum permittivity.
* $A$ is the area of the electrodes.
* $d$ is the distance between electrodes.

The DHT11's internal chip measures this capacitance change and maps it to Relative Humidity ($0\%\text{ to }100\%\text{ RH}$).

### 2. The NTC Thermistor (Temperature Sensing)
Temperature is measured using a **Negative Temperature Coefficient (NTC) thermistor**. This is a thermal resistor made of sintered metal oxides. 

As temperature increases, thermal energy excites electrons, freeing them to jump from the valence band to the conduction band. The density of charge carriers increases, causing the electrical resistance ($R$) of the thermistor to drop exponentially. This behavior is modeled by the Steinhart-Hart equation:

$$\frac{1}{T} = A + B\ln(R) + C(\ln(R))^3$$

Where $T$ is temperature in Kelvin, $R$ is resistance, and $A, B, C$ are material constants.

### 3. The DHT11 40-Bit Single-Bus Protocol
Unlike standard I2C or SPI digital protocols, the DHT11 communicates using a custom single-wire handshake protocol over a single data line.

```
DHT11 Data Line Handshake:

            Host Start                                 Sensor Response
    5V _____            ___________                 ________          ________
            \          /           \               /        \        /        \
  Data       \________/             \_____________/          \______/          \=== 40-Bit Data
    0V         18ms                     80µs          80µs
```

1. **Host Start Pulse:** The Arduino pulls the data line `LOW` for at least 18 milliseconds, then pulls it `HIGH` and waits 20-40 microseconds for the sensor to respond.
2. **Sensor Handshake:** The DHT11 pulls the line `LOW` for 80 microseconds, then lets it fly `HIGH` for 80 microseconds to indicate it is ready.
3. **Data Transmission:** The sensor transmits 40 bits (5 bytes) of data. Each bit begins with a 50µs `LOW` phase. The length of the subsequent `HIGH` phase determines if the bit is a 0 or a 1:
   - **Bit '0':** 26 to 28 microseconds `HIGH`.
   - **Bit '1':** 70 microseconds `HIGH`.
4. **Checksum Validation:** The 5th byte transmitted is the checksum. The Arduino verifies that:
   $$\text{Byte 1} + \text{Byte 2} + \text{Byte 3} + \text{Byte 4} = \text{Checksum (Byte 5)}$$

---

## 🔄 Alternatives: DHT11 vs. DHT22 vs. High-Precision Sensors

| Sensor Module | Temperature Range | Humidity Range | Accuracy (Temp/Humidity) | Sample Rate | Cost | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **DHT11** | $0\text{ to }50^{\circ}\text{C}$ | $20\text{ to }90\%\text{ RH}$ | $\pm 2.0^{\circ}\text{C} \ / \ \pm 5\%$ | $1\text{ Hz}$ (1 sec) | Very Low | **Chosen** for basic logging, educational setups, and indoor home monitoring. |
| **DHT22 (AM2302)** | $-40\text{ to }80^{\circ}\text{C}$ | $0\text{ to }100\%\text{ RH}$ | $\pm 0.5^{\circ}\text{C} \ / \ \pm 2\%$ | $0.5\text{ Hz}$ (2 sec) | Low | Outdoor logging, greenhouses, and mid-range weather tracking. |
| **BME280** | $-40\text{ to }85^{\circ}\text{C}$ | $0\text{ to }100\%\text{ RH}$ | $\pm 1.0^{\circ}\text{C} \ / \ \pm 3\%$ | Up to $100\text{ Hz}$ (I2C) | Moderate | Altmeters, drones, pressure tracking, and professional meteorological stations. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **DHT11 Sensor Module** (or raw 4-pin DHT11 + 10kΩ resistor).
3. **Breadboard & Jumper Wires**.
4. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

> [!NOTE]
> If you are using a raw 4-pin DHT11 sensor, Pin 3 is empty and not used. Connect Pin 1 to 5V, Pin 4 to GND, and Pin 2 to Arduino Pin 2. You **MUST** connect a 10kΩ pull-up resistor between Pin 2 (Data) and Pin 1 (5V). If using a 3-pin breakout module (recommended), the resistor is pre-soldered.

| Breakout Module Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** / (+) | **5V** | Red | Power supply (+5V) |
| **DATA** / OUT / S | **Pin 2** | Green / Yellow | Serial data line |
| **GND** / (-) | **GND** | Black | Ground reference |

---

## 🧪 How to Test and Validate

Follow these steps to run and calibrate your logger:

### 1. Library Installation
- In the Arduino IDE, go to **Sketch > Include Library > Manage Libraries**.
- Search for **"DHT sensor library"** by Adafruit.
- Install the library (it may ask to install dependencies like "Adafruit Unified Sensor"; select "Install All").

### 2. Telemetry Verification
- Upload `Day_09_DHT11_Logger.ino`.
- Open the Serial Monitor (**Tools > Serial Monitor**) at **9600 Baud**.
- Verify that environmental data begins printing every 2 seconds:
  ```text
  Humidity: 52.0% | Temp: 24.0 °C (75.2 °F) | Heat Index: 24.3 °C
  ```

### 3. Sensor Stimulus Testing
- **Test Humidity Rise:** Gently blow hot, humid breath directly into the grid of the DHT11 sensor (this simulates high humidity).
  - The humidity reading should rise rapidly to 80-90%.
  - The calculated Heat Index should rise higher than the raw temperature because of the added moisture.
- **Test Temperature Rise:** Place your finger firmly on the plastic grid casing to warm it up.
  - The temperature reading should slowly rise toward body temperature (approx. 30-33°C).

### 🔍 Troubleshooting Tips
* **The monitor prints "Failed to read from DHT sensor! Check connection.":**
  - Check your wiring. Ensure the DATA line goes to **Pin 2** (not Pin 3).
  - If using a raw 4-pin sensor, verify the 10kΩ pull-up resistor is placed between the Data line and 5V.
  - The DHT11 requires at least 2 seconds between samples. Check that your code scheduler is not running too fast.
* **The readings are constant and never change:**
  - DHT11s have a slow response rate. It can take up to 30 seconds for the internal sensor elements to settle when exposed to new conditions.
  - Check if the sensor is powered with 5V (3.3V is sometimes insufficient for cheap DHT11 clones).

# Day 18: Water Level Detection (Corrosion Gating)

Welcome to Day 18 of the 100-Day Arduino Masterclass! Today, we explore liquid sensing. We will learn how to interface a resistive water level sensor to measure depth and implement a critical mechatronic design pattern: **gated power switching** to prevent sensor destruction from electrolysis.

You will master the physical chemistry of electrochemical corrosion, study voltage divider resistance sweeps in conductive liquids, and configure safe analog sampling routines.

---

## 🎯 Today's Learning Goals
1. Understand how water level modules translate depth into analog resistance changes.
2. Master the chemistry of galvanic electrolysis corrosion in conductive liquids.
3. Program a gated power-switching sequence to reduce sensor wear by $99\%$.
4. Establish custom calibration thresholds for local tap water conductivity.
5. Identify alternative liquid depth sensing systems for industrial environments.

---

## 🧠 The "Why" and "What": Water Level Sensors in Robotics

### What is a Resistive Water Level Sensor?
A resistive water level sensor (liquid level sensor) is a small PCB module featuring a series of parallel, exposed copper traces. These traces are organized in alternating pairs: some connected to a power supply line, and others connected to an analog readout line.

### Why is it Used in Robotics & Environmental Control?
Liquid tracking is essential for resource management, safety, and closed-loop control:
- **Autonomous Agricultural Irrigators:** Tracking the reservoir level in water tanks to ensure pumps do not run dry and burn out.
- **Flood Detection Drones:** Amphibious rovers that trigger alarm states if water enters the electronics containment bay.
- **Sump Pump Systems:** Monitoring water accumulation in basements and automatically turning on discharge pumps.
- **Robotic Cocktail Dispensers / Baristas:** Monitoring liquid levels in supply bottles.

---

## ⚡ The Physics & Hardware Theory

### 1. Liquid Resistivity and Exposed Traces
Pure water ($\text{H}_2\text{O}$) is actually an excellent electrical insulator. However, standard tap water, rain, and environmental water contain dissolved minerals, salts, and metal ions (like $\text{Na}^+$, $\text{Cl}^-$, $\text{Ca}^{2+}$). These free ions act as charge carriers, making the water electrically conductive.

```
       Exposed Traces (Untouched)                Water Bridges Traces (Conductive)
       
         VCC Trace    Signal Trace                 VCC Trace    Signal Trace
            |             |                           |   Ion Flow  |
            |   (Air Gap) |                           |==➡️===➡️====|
            | (Insulator) |                           |  (Conductive) |
            v             v                           v  (Water)    v
```

The sensor contains alternating traces. 
* **Dry State:** The air gap between the VCC trace and the Signal trace has infinite resistance. The signal line is pulled to GND by a pre-soldered resistor on the sensor board, reading `0` on A0.
* **Wet State:** As the sensor is submerged, water bridges the gaps between the traces. The water acts as a variable resistor. The deeper the sensor is submerged, the more surface area of the copper traces is in contact with the water, effectively placing multiple water resistors in parallel. According to parallel resistance math:

$$\frac{1}{R_{total}} = \frac{1}{R_1} + \frac{1}{R_2} + \dots + \frac{1}{R_n}$$

This causes the overall electrical resistance between the VCC and Signal lines to decrease, raising the junction voltage which is read on analog Pin A0.

### 2. The Chemistry of Electrolysis and Corrosion Gating
If you connect the water level sensor's VCC pin directly to the Arduino's continuous 5V power line and submerge it in water, you create an **electrolytic cell**. 

Passing a direct current (DC) through water containing ions causes **electrolysis** and galvanic oxidation of the metal contacts:
* **Anode (Positive Trace):** The copper metal ($\text{Cu}$) undergoes oxidation, losing electrons and dissolving into the water as copper ions:
  $$\text{Cu}_{(s)} \rightarrow \text{Cu}^{2+}_{(aq)} + 2e^-$$
* **Result:** Within 24 to 48 hours of continuous powering, the positive copper traces on the sensor will completely dissolve, rendering the sensor useless and leaving copper oxide sludge in the water.

```
        Corrosion Gating Power Graph:

       Continuous Power (Bad)               Gated Power (Day 18 standard)
       
       Volt                                 Volt
        ^                                    ^   Pulse 10ms
       5V|======================            5V|  |  
         |                                    |  |  
       0V+----------------------➡️           0V+--+------------------➡️ Time
         (Corrodes in 48 hours)               (Lasts for years!)
```

**The Fix (Gated Power):** Instead of continuous power, we wire the sensor VCC to a digital I/O pin. We only pull this pin `HIGH` (5V) for 10ms when taking a reading, once every second.
* **Corrosion Duty Cycle Reduction:**
  $$\text{Duty Cycle} = \frac{10\text{ ms}}{1000\text{ ms}} = 1\%$$
* This decreases the active electrical current duration by **$99\%$**, extending the sensor's physical life from 2 days to years!

---

## 🔄 Alternatives: Resistive vs. Non-Contact Sensors

| Sensor Type | Operating Physics | Liquid Contact | Corrosion Risk | Cost | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Resistive Sensor** | Alternating traces conducting through liquid. | Yes | High (unless gated). | Very Low | **Chosen** for low-cost student projects and shallow containment basins. |
| **Ultrasonic Sensor (Day 7)** | Measures time-of-flight of sound from top of tank to liquid surface. | **No** (Non-contact). | Zero | Low | Deep water tanks, acid containers, and heavy industrial vats. |
| **Float Switch** | A magnet inside a floating collar triggers an internal reed switch. | Yes | Low (contacts sealed inside plastic tube). | Low | Simple high/low emergency overflow cutoff triggers. |
| **Capacitive Level Sensor** | Measures change in capacitance through container walls. | **No** (Mounted outside plastic pipe). | Zero | Moderate | Medical equipment tubes, high-purity chemical lines. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **Resistive Water Level Sensor Board**.
3. **Cup of Water** (for testing).
4. **Breadboard & Jumper Wires**.
5. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

| Sensor Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** / (+) | **Pin 7** | Red | Gated digital power line (not 5V!) |
| **GND** / (-) | **GND** | Black | Ground reference |
| **Signal** / S | **A0** | Yellow | Analog voltage signal |

---

## 🧪 How to Test and Validate

Follow these steps to run, calibrate, and verify your water sensor:

### 1. Dry Baseline Test
- Upload `Day_18_Water_Level.ino`.
- Open the Serial Monitor (**Tools > Serial Monitor**) at **9600 Baud**.
- While the sensor is dry on your desk, check the logs. It should read:
  ```text
  Raw ADC: 0 | Voltage: 0.00 V | Water Level: DRY (Empty)
  ```
- If the value is above 50, dry the sensor face with a towel.

### 2. Submersion Depth Calibration
- Prepare a cup of water.
- **Shallow Dip:** Dip only the very bottom tips (first 2-3mm) of the sensor into the water.
  - The reading should jump. The monitor should log:
    ```text
    Raw ADC: 180 | Voltage: 0.88 V | Water Level: LOW (Shallow)
    ```
- **Half Submerged:** Dip the sensor halfway into the cup (about 2cm deep).
  - The reading should increase to roughly 300-500. The monitor should log:
    ```text
    Raw ADC: 410 | Voltage: 2.01 V | Water Level: MEDIUM (Half-Full)
    ```
- **Deep Dip:** Submerge the sensor up to the maximum line (do not submerge the electronic components/resistors at the top header!).
  - The monitor should log:
    ```text
    Raw ADC: 750 | Voltage: 3.67 V | Water Level: HIGH (Full / Alert)
    ```

### 🔍 Troubleshooting Tips
* **The reading displays "DRY (Empty)" even when fully submerged:**
  - Make sure the sensor's VCC is wired to **Pin 7** (not Pin 8 or 5V).
  - Check that your `SENSOR_POWER_PIN` in code is set to `7`.
* **The readings are highly unstable, fluctuating by 100+ points:**
  - Ensure the Ground wire is connected firmly.
  - Dissolved salts change conductivity. If using distilled water (pure water), the sensor will not read correctly. Add a pinch of salt to the water to simulate tap water.
* **The sensor traces are turning green or black:**
  - You did not implement power gating and left the sensor connected to the continuous 5V pin. The green deposit is copper oxide/carbonate corrosion. Dry the sensor, clean with a toothbrush, and rewire it to Pin 7.

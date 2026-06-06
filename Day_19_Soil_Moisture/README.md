# Day 19: Soil Moisture Monitor (Electrolysis Mitigation)

Welcome to Day 19 of the 100-Day Arduino Masterclass! Today, we study soil electronics and agricultural automation. We will learn how to interface a resistive soil moisture sensor fork to monitor soil hydration levels and apply **gated digital powering** to protect the probes from rapid electrolysis decay.

You will master soil electrical resistivity physics, study the design of capacitive vs. resistive probes, and calibrate soil moisture thresholds.

---

## 🎯 Today's Learning Goals
1. Understand how soil moisture probes measure volumetric water content.
2. Study the physics of soil resistivity and moisture conduction.
3. Master the difference between resistive and corrosion-free capacitive soil sensors.
4. Program a non-blocking gated power loop to prevent probe dissolution.
5. Calibrate dry, moist, and wet boundaries using constrain and map functions.

---

## 🧠 The "Why" and "What": Soil Moisture in Robotics

### What is a Soil Moisture Sensor?
A soil moisture sensor measures the water content inside a soil sample. The standard resistive model consists of a two-pronged fork probe that is inserted directly into the dirt. A small comparator driver board translates the electrical resistance of the soil into an analog voltage.

### Why is it Used in Robotics & Smart Agriculture?
Automating agriculture is a major branch of mechatronics:
- **Smart Greenhouse Systems:** Monitoring plant beds and triggering water pumps (via relays, Day 16) only when the soil gets dry, preventing overwatering and saving water.
- **Agricultural Rovers:** Mobile autonomous robots that drive through crop fields, insert sensor probes into the soil, log coordinates (GPS) and moisture levels, and compile soil maps.
- **Landslide Prediction Systems:** Monitoring soil saturation on steep hillsides to warn of mudslides.

---

## ⚡ The Physics & Hardware Theory

### 1. The Physics of Soil Resistivity
Dry soil (silicon dioxide, clay, organic matter) is an excellent electrical insulator with very high resistance. 
However, when water is added, it fills the microscopic pockets of air between the soil particles. Tap water contains dissolved minerals and salts, creating an electrolytic path. 

```
         Dry Soil (Insulating)                      Wet Soil (Conductive)
         
          [Soil]   [Soil]   [Soil]                  [Soil] ~~~ [Soil] ~~~ [Soil]
            (Air Gap = Open Circuit)                   (Water Bridges Gaps)
         
          Resistance: Megaohms                      Resistance: Kiloohms
```

As soil moisture content increases:
* The cross-sectional area of the water paths between the probe prongs increases.
* The electrical resistance ($R_{soil}$) between the two fork prongs drops exponentially.
* By measuring the resistance, we can estimate the relative moisture percentage.

### 2. The Electrolysis Problem in Soil
Resistive soil probes are highly vulnerable to **galvanic corrosion**. Because soil is damp and contains mineral acids/salts, applying a continuous 5V DC voltage across the prongs triggers rapid electrolysis. 

* The positive copper prong acts as an anode, dissolving into the soil. 
* The negative prong accumulates hydrogen bubbles.
* Within a few days of continuous operation, the copper plating is completely stripped off the probe, leaving bare fiberglass and ruining the sensor.

**The Solution:** Just like the water level sensor (Day 18), we connect the sensor's VCC pin to an Arduino digital pin (e.g. Pin 7) and write it `HIGH` only during a 10ms read window once every 2 seconds. This reduces probe degradation by **$99.5\%$**.

### 3. Alternative: Capacitive Soil Moisture Sensors
To solve the corrosion problem entirely in commercial systems, engineers use **Capacitive Soil Moisture Sensors**.

```
    Resistive Probe (Exposed Copper)             Capacitive Probe (Insulated Lacquer)
    
           |  Current  |                                 |             |
           |===➡️===➡️====| (Direct Current)              | [Plate] [Plate] | (AC Field)
           | (Corrosion) |                               |     (No Direct Contact)
```

* **Resistive:** Exposed copper electrodes. Current passes directly through the soil. High corrosion risk.
* **Capacitive:** The electrodes are laminated inside the PCB under a protective solder mask layer. No copper touches the soil. It acts as a capacitor, measuring the soil's **dielectric permittivity** (which changes with water content). Since no current flows through the soil, it **never corrodes**.

---

## 🔄 Alternatives: Resistive vs. Capacitive Probes

| Sensor Type | Operating Physics | Probe Exposure | Corrosion | Cost | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Resistive Probe (YL-69)** | Soil resistance. | Exposed copper contacts. | High | Very Low | **Chosen** for basic tutoring, cheap garden monitors, and learning power gating. |
| **Capacitive Probe (v1.2)** | Soil dielectric capacitance. | Insulated (laminated). | None | Low | Long-term smart gardens, agricultural monitoring nodes. |
| **TDR/FDR Probe** | High-frequency electromagnetic pulse propagation. | Metal rods. | None | Very High | Scientific research, professional crop management. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **Resistive Soil Moisture Probe & Comparator Board** (YL-69 / YL-38 combo).
3. **Potted Plant with dry soil** (or cup of dry dirt + water).
4. **Breadboard & Jumper Wires**.
5. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

First, connect the two pins of the fork probe to the two pins on the driver module (polarity does not matter here). Then connect the driver module to the Arduino:

| Module Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** / (+) | **Pin 7** | Red | Gated digital power line |
| **GND** / (-) | **GND** | Black | Ground reference |
| **AO** (Analog Out) | **A0** | Yellow | Analog signal output |
| **DO** (Digital Out) | *Leave Empty* | - | Not used (onboard comparator threshold) |

---

## 🧪 How to Test and Validate

Follow these steps to upload your code, test, and calibrate your monitor:

### 1. Dry Air Calibration
- Upload `Day_19_Soil_Moisture.ino`.
- Open the Serial Monitor (**Tools > Serial Monitor**) at **9600 Baud**.
- Hold the probe clean in dry air.
  - The monitor should log a high ADC reading:
    ```text
    Raw ADC: 1021 | Est. Moisture: 0.0% | Status: DRY (Needs Watering!)
    ```

### 2. Wet Submersion Calibration
- Dip the probe prongs into a cup of water (submerge up to the plastic collar, do not submerge the driver board).
  - The reading should drop instantly. The monitor should log:
    ```text
    Raw ADC: 150 | Est. Moisture: 100.0% | Status: WET (Saturated)
    ```

### 3. Soil Moisture Calibration
- **Dry Soil Test:** Insert the probe into a pot of dry soil. Note the raw ADC reading (e.g. 800).
- **Moist Soil Test:** Slowly pour water around the plant. Watch the Serial Monitor logs update:
  - The Raw ADC value should drop.
  - As it crosses below `750` (our `VAL_DRY` limit), the status should change:
    ```text
    Raw ADC: 520 | Est. Moisture: 49.2% | Status: MOIST (Ideal)
    ```
- **Wet Saturated Test:** Pour excess water until the dirt is muddy. The ADC reading should drop below `350` (our `VAL_WET` limit), logging:
  ```text
  Raw ADC: 280 | Est. Moisture: 72.6% | Status: WET (Saturated)
  ```

### 🔍 Troubleshooting Tips
* **The reading fluctuates randomly between 0% and 100%:**
  - Check the two-pin cable connecting the fork probe to the driver module. If this connection is loose, the resistance jumps.
  - Ensure the GND wire is solid.
* **The values are inverted (Dry prints 100%, Wet prints 0%):**
  - In your mapping function, check the order of values. We use `map(sensorValue, 1023, 0, 0, 100)`. Because dry soil reads high ADC (1023) and wet reads low (0), we reverse the mapping to make 1023 correspond to 0% and 0 correspond to 100%.
* **The sensor fails to read and outputs 1023 continuously:**
  - Make sure the sensor's VCC is connected to **Pin 7** (and Pin 7 is configured as an `OUTPUT` in `setup()`).

## 🧠 Code Explanation

Let's break down the logic for measuring soil saturation:

### 1. Inverted Analog Logic
```cpp
const int VAL_DRY = 750;
const int VAL_WET = 350;

if (sensorValue > VAL_DRY) {
    soilStatus = "DRY (Needs Watering!)";
} else if (sensorValue < VAL_WET) {
    soilStatus = "WET (Saturated)";
}
```
- Resistive soil probes work inversely to what you might expect.
- When the soil is bone dry, it acts like an insulator. Resistance goes up, voltage stays high, and the ADC reports a huge number (e.g., `800+`).
- When the soil is soaked, the water acts as a conductor. Resistance drops, pulling the signal down to GND, and the ADC reports a tiny number (e.g., `250`).

### 2. Safe Mathematical Constraining
```cpp
float moisturePercent = map(sensorValue, 1023, 0, 0, 100);
moisturePercent = constrain(moisturePercent, 0.0, 100.0);
```
- We use the `map()` function to convert the `1023-0` scale into a `0%-100%` scale. 
- However, if the sensor reads higher than 1023 or lower than 0 (which can happen due to noise or varying calibrations), the map function will output weird values like `-5%` or `110%`.
- The `constrain(value, min, max)` function is a professional safety net. It strictly forces the variable to stay within the `0` to `100` boundary, preventing UI bugs down the road!

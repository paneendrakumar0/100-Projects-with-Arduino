# Day 59: Precision Voltage & Current Monitor (INA219 + ADC Resistor Divider)

Welcome to Day 59! Today we build a **precision power monitoring station** using two complementary sensing methods: the **INA219 I2C current-sense IC** for high-accuracy bus voltage/current/power readings, and a **resistor-divider circuit** feeding the Arduino's raw 10-bit ADC for general-purpose voltage rail surveillance. We read the INA219 without any external library — directly over I2C by writing calibration values and reading measurement registers.

---

## 🎯 The "Why" and "What"

Monitoring power is critical for:
* **Battery Management Systems (BMS):** Track state-of-charge and over-current faults.
* **Switching Regulators:** Monitor load current to detect inefficiency or overload.
* **Solar/Energy Harvest Systems:** Measure input and output power for MPPT algorithms.
* **Lab Power Supplies:** Display live voltage and current on a monitor.

Without a dedicated IC, accurate current measurement requires precise analog circuitry and differential amplifiers. The INA219 integrates a 12-bit ADC, programmable gain amplifier (PGA), and calibration-based I2C registers into a single $1 package.

---

## 🔬 Physics & Mathematics

### 1. Shunt Resistor Principle (Ohm's Law)
The INA219 measures the tiny voltage drop across a known **shunt resistor** $R_{shunt}$ in series with the load:
$$V_{shunt} = I_{load} \times R_{shunt}$$
$$I_{load} = \frac{V_{shunt}}{R_{shunt}}$$

With $R_{shunt} = 0.1\,\Omega$ and $I = 1\,\text{A}$: $V_{shunt} = 0.1\,\text{V}$ — easily measured by the INA219's high-gain differential ADC.

### 2. INA219 Calibration Register
The calibration register scales the current and power raw registers to physical units:
$$\text{Cal} = \text{trunc}\left(\frac{0.04096}{I_{LSB} \times R_{shunt}}\right)$$

With $I_{LSB} = 0.001\,\text{A}$ (1 mA resolution) and $R_{shunt} = 0.1\,\Omega$:
$$\text{Cal} = \text{trunc}\left(\frac{0.04096}{0.001 \times 0.1}\right) = \text{trunc}(409.6) = 4096 = \texttt{0x1000}$$

### 3. Converting Raw Registers to Physical Values

| Register | LSB | Conversion |
| :--- | :--- | :--- |
| **Bus Voltage** (Bits 15:3) | 4 mV | `BusV = (Raw >> 3) × 0.004` |
| **Shunt Voltage** (Signed) | 10 µV = 0.01 mV | `ShuntV_mV = Raw × 0.01` |
| **Current** (Signed, calibrated) | $I_{LSB}$ = 1 mA | `Current_mA = Raw × 1.0` |
| **Power** (Unsigned, calibrated) | $20 \times I_{LSB}$ = 20 mW | `Power_mW = Raw × 20` |
| **Load Voltage** | Derived | `LoadV = BusV + ShuntV` |

### 4. Resistor Divider for High-Voltage Rail Monitoring
To safely measure voltages above 5V, a resistor divider scales the voltage down:
$$V_{pin} = V_{rail} \times \frac{R2}{R1 + R2} < 5\,\text{V (safe for ADC)}$$

Reconstructing $V_{rail}$ from the ADC reading:
$$V_{rail} = \frac{ADC\_count \times V_{ref}}{1023} \times \frac{R1 + R2}{R2}$$

With $R1 = 47\,\text{k}\Omega$, $R2 = 5.1\,\text{k}\Omega$:
$$\text{Ratio} = \frac{47000 + 5100}{5100} \approx 10.22$$
$$V_{rail,max} = 5.0 \times 10.22 \approx 51\,\text{V}$$

The Arduino ADC at $V_{ref} = 5\,\text{V}$ provides $\frac{5\,\text{V}}{1023} \approx 4.88\,\text{mV}$ resolution per step.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Main controller |
| INA219 Breakout Module | 1 | Precision current/power sensing |
| 0.1 Ω Shunt Resistor (1W+) | 1 | In-series current sensing |
| 47 kΩ Resistor | 1 | Divider top (R1) |
| 5.1 kΩ Resistor | 1 | Divider bottom (R2) |
| Breadboard + Jumpers | 1 | Wiring |

### INA219 Alternatives

| Chip | Interface | Max Voltage | Max Current | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **INA219** | I2C | 26 V | ~3.2 A (with shunt) | Our choice, simple calibration |
| INA226 | I2C | 36 V | Same | 16-bit ADC, higher accuracy |
| INA3221 | I2C | 26 V/channel | Same | 3 channels simultaneously |
| ACS712 | Analog | Any | 5/20/30 A | Hall-effect, no shunt needed |

---

## 🔌 Pin-to-Pin Wiring

### INA219 Wiring

| INA219 Pin | Connect to | Note |
| :--- | :--- | :--- |
| **VCC** | **3.3V or 5V** | Both logic levels supported |
| **GND** | **GND** | |
| **SDA** | **A4** | I2C Data |
| **SCL** | **A5** | I2C Clock |
| **IN+** | **Supply Positive** | Before the shunt (upstream) |
| **IN-** | **Shunt → Load** | After the shunt (downstream) |

### External Rail Voltage Divider

```
Rail Voltage (+) ──── R1 (47k) ──┬──── R2 (5.1k) ──── GND
                                  │
                                 A0 (Arduino ADC)
```
> ⚠️ **CAUTION:** R2 must be chosen so that $V_{pin} < 5\,\text{V}$ at **maximum** expected rail voltage. Always add a 5.1V Zener clamp across R2 for protection against voltage spikes.

---

## 💻 How to Test & Validate

1. Upload [Day_59_Voltage_Monitor.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_59_Voltage_Monitor/Day_59_Voltage_Monitor.ino).
2. Open **Serial Monitor** at **9600 Baud**.
3. Connect the INA219 IN+ / IN- across a small load (e.g., LED + resistor on a 9V battery).
4. Observe live readings: Bus Voltage, Shunt Voltage, Current (mA), Power (mW).
5. Connect an adjustable power supply (0–30V) to the resistor divider input on A0. Observe `Rail:` column scaling correctly.
6. Exceed the `OVERVOLTAGE_THRESHOLD` (30V default) and watch the serial monitor flag `[OVER-VOLTAGE!]` and Pin 13 LED illuminate.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Current reads negative large value | Shunt polarity reversed | Swap IN+ and IN− on INA219 |
| BusVoltage = 0V always | I2C address wrong | Run I2C scanner sketch; confirm A0/A1 address pins |
| Rail voltage is 10x off | Wrong R1/R2 values in code | Update `R1` and `R2` constants to match your actual resistors |
| Over-current never triggers | `OVERCURRENT_THRESHOLD` too high | Lower threshold to match expected load |

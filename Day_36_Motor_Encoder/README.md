# Day 36: DC Motor Encoder Feedback (RPM & Odometry)

Welcome to Day 36 of the 100-Day Arduino Masterclass! Today, we study sensor feedback loops in mechatronics. We will interface a **digital optical slot encoder** (or magnetic Hall-effect encoder) with our Arduino Uno. We will write an **interrupt-driven pulse counter** and implement a non-blocking velocity calculator to measure motor shaft speed (RPM) and angular velocity ($\text{rad/s}$) in real-time.

---

## 🎯 The "Why" and "What"

Open-loop systems (like Days 33 and 34) send commands to actuators but have no way of verifying if the motor actually moved. If a robot wheel gets stuck in sand, or carrying a heavy payload uphill, open-loop PWM duty cycles will fail to maintain speed.
* **The Solution:** We introduce **Encoder Feedback**. Encoders generate digital pulses as the motor rotates, allowing the microcontroller to:
  1. **Measure Speed (RPM):** Verify how fast the motor is spinning.
  2. **Calculate Distance (Odometry):** Track how far a robot has traveled by counting wheel revolutions.
  3. **PID Close-Loop Control (Day 37):** Automatically adjust PWM power to maintain a constant speed under variable mechanical loads.

---

## 🔬 Physics & Hardware Theory

### 1. Encoder Technologies (Optical vs. Hall-Effect)
* **Optical Encoders:** Contain an infrared emitting diode and a photo-transistor receiver separated by a slotted disk attached to the motor shaft. As the disk rotates, the solid portions block the light, and the slots allow it to pass. This generates a digital square wave pulse train.
* **Magnetic Encoders:** Use a multi-pole magnetized ring attached to the shaft. A Hall-effect sensor detects changes in magnetic flux (North vs. South pole) and outputs digital pulses.

---

### 2. Velocity Math (RPM and $\text{rad/s}$ Calculations)
The number of pulses generated during one full revolution of the disk is the **PPR (Pulses Per Revolution)**.
If the Arduino records $N$ pulses during a time window $\Delta t$ (in milliseconds):
$$\text{Revolutions} = \frac{N}{\text{PPR}}$$
$$\text{Time in Minutes} = \frac{\Delta t}{60,000}$$
$$\text{RPM} = \frac{\text{Revolutions}}{\text{Time in Minutes}} = \frac{N \times 60,000}{\text{PPR} \times \Delta t}$$

For our code's configuration ($\Delta t = 200\text{ ms}$, $\text{PPR} = 20$):
$$\text{RPM} = \frac{N \times 60,000}{20 \times 200} = N \times 15$$

To convert RPM to angular velocity $\omega$ in **Radians Per Second** (used in robotics kinematic equations):
$$\omega = \text{RPM} \times \frac{2\pi}{60}$$

---

### 3. Data Tearing and Atomic Register Reads
The Arduino Uno uses an 8-bit microcontroller (ATmega328P). A `long` variable occupies 4 bytes (32 bits). 
* **The Risk:** When the main loop reads a 32-bit variable (like our pulse counter), it takes several CPU cycles to load all 4 bytes. If an encoder interrupt triggers *during* this read, it will modify the variable mid-operation. This results in corrupt data (known as **Data Tearing**).
* **The Solution:** We implement an **Atomic Read Block**. We disable interrupts, copy the volatile counter to a temporary variable, reset the counter to zero, and immediately re-enable interrupts:
```cpp
noInterrupts();
long pulsesCaptured = encoderPulseCount;
encoderPulseCount = 0;
interrupts();
```
This minimizes the time interrupts are disabled ($<1\text{ microsecond}$), ensuring we never miss incoming pulse edges.

---

## 🔄 Alternatives Comparison

When measuring rotational feedback:

| Feedback Sensor | Sensor Type | Output Interface | Measured Parameter | Cost | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Optical Slot Encoder** | **Optical Interrupt** | **Single Digital Pulse** | **Speed (RPM)** | **Low ($\approx \$1.50$)** | **Basic speed regulation, wheel tachometers (Our choice)** |
| **Quadrature Encoder** | **Dual Hall / Optical** | **Dual Channel (A & B)**| **Speed & Direction** | **Medium ($\approx \$3 - \$5$)** | **Robot wheel odometry, joint angles, servo loops** |
| **Analog Tachometer** | **DC Generator** | **Analog Voltage** | **Speed (RPM)** | **High** | **Legacy industrial analog control loops** |
| **Sensorless Back-EMF**| **Algorithmic** | **Phase Voltage ADC** | **Speed** | **None** | **High-speed Brushless DC (BLDC) motor ESC controllers** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x Brushed DC Motor (with slot disk or magnetic ring attached)
* 1x Optical Slot Encoder Module (e.g. LM393 slot photo-interrupter)
* 1x Motor Driver (e.g., L298N module or TIP120 transistor circuit)
* 1x External Battery Pack for the motor
* Jumper wires

---

## 🔌 Pin-to-Pin Wiring

| Sensor/Driver Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **Encoder VCC** | **5V** | Red | Sensor logic power |
| **Encoder GND** | **GND** | Black | Sensor ground return |
| **Encoder OUT** | **D2** (Interrupt 0) | Green | Sensor square wave pulse output |
| **Driver PWM / ENA** | **D9** (PWM pin) | Orange | Motor speed control signal |
| **Driver GND** | **GND** | Black | Common Ground connection |

---

## 💻 How to Test & Validate

1. Wire up the optical sensor and motor driver. Connect the motor to the driver and ensure the external battery ground is tied to the Arduino GND.
2. Upload `Day_36_Motor_Encoder.ino` to the Arduino.
3. Open the **Serial Monitor** at **9600 Baud**.
4. The DC motor will start spinning. Every 4 seconds, the motor speed increases as the PWM steps through: `0 -> 80 -> 130 -> 180 -> 255`.
5. Observe the Serial Monitor output:
   ```
   [COMMAND] -> Transitioned to Motor PWM Duty Cycle: 130 / 255
   [MEASUREMENT] PWM Out: 130 | Pulses: 18 | Speed: 270.0 RPM | 28.27 rad/s
   [MEASUREMENT] PWM Out: 130 | Pulses: 19 | Speed: 285.0 RPM | 29.85 rad/s
   ```
6. Verify that the calculated RPM increases proportionally with the commanded PWM values. When the PWM is `0`, the calculated RPM should drop to exactly `0.0`.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The motor is spinning at max speed, but the calculated RPM displays `0.0`:**
  * Ensure the sensor OUT pin is connected to **Pin 2**. External Interrupt 0 on the Uno is strictly mapped to digital Pin 2.
  * Check that the slotted disk is passing cleanly through the center slot of the optical sensor without touching the plastic casing.
* **The RPM readings fluctuate wildly (e.g., jumping from 100 to 2000 randomly):**
  * Check the alignment of the optical disk. If it is warped, it might create double triggers.
  * Ensure your sensor cable is away from high-current motor power cables to avoid electromagnetic noise coupling (crosstalk) on the interrupt line.
* **The Arduino resets as soon as the motor attempts to spin:**
  * You connected the motor's power pins directly to the Arduino's 5V pin. Always use an external battery and driver module.

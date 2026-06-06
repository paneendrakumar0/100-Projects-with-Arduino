# Day 44: Basic Line-Following Robot (2-Sensor Differential)

Welcome to Day 44 of the 100-Day Arduino Masterclass! Today, we study our first autonomous path-tracking mobile robot. We will interface a two-sensor **TCRT5000 Infrared (IR) reflection array** with our Arduino and use analog signals to steer a differential drive robot chassis along a black track on a white floor. We will learn about infrared absorption physics, software-defined hysteresis thresholds, and differential steering kinematics.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/DC_Motor.jpg" alt="DC Motor" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Differential_Drive.jpg" alt="Differential Drive" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

Line-following is the classic entry point for autonomous ground vehicle navigation. In industrial mechatronics, line-following is a highly reliable way to guide **Automated Guided Vehicles (AGVs)**:
1. **Warehouse Logistics:** AGVs carrying heavy pallets along designated pathways marked with magnetic or colored tape.
2. **Hospital Delivery Robots:** Delivering food and medicine to patient wards along preset tracks.

Using two IR sensors, we build a closed-loop system where the motor power is dynamically routed to keep the black track centered between the two sensors.

---

## 🔬 Physics & Sensing Theory

### 1. Infrared Reflection & Absorption Physics
The TCRT5000 sensor contains a paired transmitter and receiver:
* **Infrared Emitter (LED):** Emits invisible infrared light (wavelength $\approx 950\text{ nm}$).
* **Phototransistor Receiver:** Acts as a gate. When IR light hits the phototransistor, it allows current to flow. The voltage drop across a load resistor is read by the Arduino's Analog-to-Digital Converter (ADC).
* **Surface Properties:**
  * **White Floor:** Reflects high amounts of IR light. The phototransistor saturates, and the analog output voltage drops close to $0\text{V}$ (low ADC steps).
  * **Black Line (Carbon black tape):** Absorbs almost all incident IR light. Very little light bounces back. The phototransistor remains closed, and the output voltage rises close to $5\text{V}$ (high ADC steps).

```
        WHITE SURFACE (Reflective)             BLACK SURFACE (Absorbent)
        ┌───┐            ┌───┐                 ┌───┐            ┌───┐
        │LED│            │PT │                 │LED│            │PT │
        └─┬─┘            └─▲─┘                 └─┬─┘            └─▲─┘
          │   IR Light    │                      │   IR Light    
          └───► \   / ────┘                      └───►  █████████ (Absorbed)
                 \ /                                    █████████
            ==============                         ===================
             White Board                            Black Carbon Tape
```

---

### 2. Software vs. Hardware Threshold Gating
Typical cheap sensor modules have an onboard potentiometer connected to an LM393 voltage comparator. This outputs a simple digital HIGH/LOW signal:
* **The Problem:** Calibrating two separate hardware potentiometers to behave identically is tedious and highly sensitive to ambient room lighting.
* **The Solution:** We bypass the digital outputs and wire the **Analog outputs (AOUT)** directly to the Arduino's ADC pins (A1 and A2). This allows us to define a single software threshold (`LINE_THRESHOLD = 500`) in code. It also lets us implement **hysteresis** (offset filters) to prevent rapid output chattering at the boundaries of the line.

---

### 3. Differential Steering State Space
For a two-sensor robot tracking a black line on a white floor, the steering kinematics resolve into four logical states:

| Left Sensor | Right Sensor | Track Alignment | Motor Left | Motor Right | Navigation Action |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **White ($<500$)** | **White ($<500$)** | Line is centered | Forward | Forward | **Drive Straight** |
| **Black ($>500$)** | **White ($<500$)** | Robot drifted right | Stop / Rev | Forward | **Pivot Left** |
| **White ($<500$)** | **Black ($>500$)** | Robot drifted left | Forward | Stop / Rev | **Pivot Right** |
| **Black ($>500$)** | **Black ($>500$)** | Junction or Stop Line| Stop | Stop | **Halt Robot** |

---

## 🔄 Alternatives Comparison

When selecting line tracking sensors for autonomous mobile robots:

| Sensing Technology | Sensor Count | Steering Smoothness | Software Complexity | Best Used For |
| :--- | :--- | :--- | :--- | :--- |
| **2-Sensor IR Array** | **2 Sensors** | **Low (Sinuous wobble)** | **Very Low** | **Basic toy cars, simple paths (Our choice)** |
| **5-Sensor IR Array** | **5+ Sensors** | **High (Calculated offset)**| **Medium** | **High-speed line followers, grid maze solvers** |
| **Magnetic Tape Sensor** | **1-2 Hall arrays** | **Medium** | **Low** | **Heavy warehouse AGVs tracking magnetic floor strips** |
| **Camera Vision (OpenCV)**| **1 Camera** | **Extremely High** | **Very High** | **Self-driving cars tracking lane markers** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x 2WD Robot Chassis (2 DC motors + wheels + caster)
* 1x L298N Dual H-Bridge Driver Module
* 2x TCRT5000 IR Proximity Sensors (or a 2-channel line tracking board)
* 1x External Battery Pack (e.g. 2s LiPo or 6x AA battery holder to power motors)
* 1x Breadboard
* Jumper wires
* Black electrical tape (to build a track)

---

## 🔌 Pin-to-Pin Wiring

| TCRT5000 Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **Left Sensor AOUT** | **A1** | Yellow | Left reflection analog voltage |
| **Right Sensor AOUT**| **A2** | Green | Right reflection analog voltage |
| **VCC (Sensors)** | **5V** | Red | Sensor logic power |
| **GND (Sensors)** | **GND** | Black | Sensor ground |

| L298N Driver Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **ENA** | **D5** (PWM) | Green | Left Motor Speed |
| **IN1** | **D4** | Orange | Left Motor Direction 1 |
| **IN2** | **D3** | Yellow | Left Motor Direction 2 |
| **ENB** | **D6** (PWM) | Violet | Right Motor Speed |
| **IN3** | **D7** | Grey | Right Motor Direction 1 |
| **IN4** | **D8** | White | Right Motor Direction 2 |
| **GND** | **GND** | Black | Shared logic and power Ground |

---

## 💻 How to Test & Validate

1. Assemble the 2WD chassis. Mount the two TCRT5000 sensors side-by-side at the front of the robot.
   * **Sensor Gap:** The distance between the sensors should be slightly wider than the width of your black electrical tape (typically $1.9\text{ cm}$). This ensures that when centered, both sensors see white.
   * **Sensor Height:** Mount the sensors roughly **$5\text{mm} - 10\text{mm}$** above the floor. If they are too high, they will fail to register the reflection.
2. Wire up the components according to the wiring table. Keep the motor battery disconnected.
3. Upload `Day_44_Line_Follower_2Sensor.ino` to the Arduino.
4. **Bench Diagnostic:**
   * Open the **Serial Monitor** at **9600 Baud**.
   * Hold a piece of white paper in front of both sensors: Raw analog values should drop below `300`. The console will print: `State: STRAIGHT`.
   * Slide a piece of black tape under the Left Sensor: The raw left reading will rise above `700`. The right motor will spin forward, and the left motor stops. The console prints: `State: TURN_LEFT`.
   * Slide black tape under both sensors: Both readings go high, and both motors stop. The console prints: `State: STOP_LINE`.
5. **Track Testing:**
   * Build a smooth track on a light-colored floor using black electrical tape (avoid sharp $90^{\circ}$ turns for now; keep corners curved).
   * Place the robot on the track, connect the motor battery, and watch it steer along the line!

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The robot drives forward, hits the black line, and spins away from the track rather than turning onto it:**
  * The motor pins or the sensor pins are swapped. If the left sensor detects black, the robot should turn left. If it turns right instead, swap the sensor pins in code:
    `const int LEFT_SENSOR_PIN = A2; const int RIGHT_SENSOR_PIN = A1;`
* **The robot shakes and oscillates back and forth violently on straight segments:**
  * This is a fundamental limitation of basic 2-sensor differential (on/off) steering. The robot continuously over-corrects. You can reduce this wobble by lowering the `CRUISE_SPEED` and `TURN_SPEED` values in the code.
  * In Day 46, we will solve this using a proportional-derivative (PD) controller.
* **The robot completely ignores the black line and drives straight off the track:**
  * The sensor height is too high or the ambient sunlight is washing out the receivers. Mount the sensors closer to the ground and block strong overhead lighting.
  * Check the raw values printed to the Serial Monitor. If the black line reads less than `500`, lower the `LINE_THRESHOLD` constant in code (e.g. to `400`).

## 🧠 Code Explanation

Let's break down how to program analog hysteresis logic for line tracking:

### 1. Analog Hysteresis Gating
```cpp
int rawLeft = analogRead(LEFT_SENSOR_PIN);
bool leftOnLine = (rawLeft > LINE_THRESHOLD);
```
- Many cheap IR sensors have a little blue potentiometer you turn with a screwdriver to adjust sensitivity. Vibrations often knock these out of calibration.
- By using `analogRead()` (0-1023), we bring the raw voltage directly into the code.
- We set a software `LINE_THRESHOLD` (e.g. 500). Anything above is converted to a simple `True` (Black Line) or `False` (White Floor). This makes the robot infinitely more reliable!

### 2. Differential Drive Steer Logic
```cpp
else if (leftOnLine && !rightOnLine) {
    steerLeft(TURN_SPEED);
} 
```
- If the left sensor suddenly sees black, it means the robot has drifted too far to the right!
- `steerLeft()` corrects this by cutting power to the left wheel and running the right wheel forward. This creates a pivot point on the left wheel, swinging the front of the robot back over the center of the line.

# Day 38: Ultrasonic Radar Sweep (Servo + Range Finder)

Welcome to Day 38 of the 100-Day Arduino Masterclass! Today, we combine two key mechatronic components—a **servo motor** and an **HC-SR04 ultrasonic sensor**—to build a sweeping **2D Radar (Sonar) Obstacle Mapper**. We will write a non-blocking angle sweep scheduler, explore the physics of acoustic speed of sound propagation, and study Polar-to-Cartesian coordinate mapping.

---

## 🎯 The "Why" and "What"

Robots need to build maps of their surroundings to navigate autonomously without colliding with obstacles. 
* **The Problem:** A fixed distance sensor only detects obstacles directly in front of the robot. Placing multiple stationary sensors around the chassis is expensive and uses many pins.
* **The Solution:** We mount a single ultrasonic sensor on top of a servo motor. By rotating (sweeping) the servo back and forth while taking range readings, we scan a $150^{\circ}$ field of view, simulating a marine radar or LiDAR system.

The telemetry output is formatted as comma-separated values (`Angle,Distance`), which can be parsed by computer scripts to render a graphical radar scan sweep in real-time.

---

## 🔬 Physics & Sensing Theory

### 1. Active Sonar Acoustics (Speed of Sound)
The HC-SR04 uses active acoustic ranging. 
* It sends out a packet of eight ultrasonic pressure pulses at **$40\text{ kHz}$** (above human hearing limits).
* The sound wave propagates through the air. When it hits an obstacle, it bounces back as an echo.
* The echo pin outputs a digital pulse whose duration (width) corresponds to the time of flight of the sound wave.

The speed of sound in dry air at sea level at $20^{\circ}\text{C}$ is approximately $343\text{ m/s}$ ($0.0343\text{ cm/\mu s}$). Since the sound wave must travel to the target and back, the distance to the target is:
$$\text{Distance (cm)} = \frac{\text{Time of Flight (µs)} \times 0.0343}{2}$$

---

### 2. Time-of-Flight Timeout Gating
If no obstacle is in range, or the acoustic wave bounces off a wall at an angle and never returns, the `pulseIn()` function would wait indefinitely, blocking program execution. 
To prevent this, we specify a **gated timeout of $20\text{ ms}$ ($20,000\text{ µs}$)**. If no pulse returns in $20\text{ ms}$, the function aborts and returns `0`, which we map to `-1.0` (out of range).

---

### 3. Polar to Cartesian Coordinate Mapping
The radar sweeps in a rotational path, generating data points in **Polar Coordinates**:
$$\mathbf{P}_{\text{polar}} = [r, \theta]^T \quad (\text{where } r = \text{Distance, } \theta = \text{Angle})$$

To display this on a computer monitor or build a robotic grid map, we convert these points into **Cartesian Coordinates (X, Y)** using trigonometry:
$$x = r \cdot \cos(\theta)$$
$$y = r \cdot \sin(\theta)$$

```
                       90° (Y-axis)
                        ▲
                        │      • Obstacle [r, θ]
                        │     / 
                        │    /  r (Distance)
                        │   /
                        │  /    θ (Angle)
  180° ─────────────────┼─┴─────────────► 0° (X-axis)
                      (0,0)
```

---

## 🔄 Alternatives Comparison

When selecting spatial mapping sensors for robots:

| Sensor Type | Technology | Field of View | Max Range | Resolution / Detail | Hardware Cost | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Acoustic Sonar (HC-SR04)** | **$40\text{ kHz}$ Ultrasound** | **$15^{\circ}$ cone** | **$4\text{m}$** | **Low (Wide beam spread)** | **Very Low ($\approx \$2$)** | **Basic obstacle detection, flat walls (Our choice)** |
| **Infrared ToF (VL53L0X)** | **$940\text{nm}$ Laser** | **$25^{\circ}$ cone** | **$2\text{m}$** | **High (Focused beam)** | **Medium ($\approx \$5$)** | **Precision proximity, color-independent ranging** |
| **Rotary LiDAR (RPLIDAR)** | **Spinning Laser** | **$360^{\circ}$ plane** | **$12\text{m} - 25\text{m}$** | **Extremely High** | **Very High ($>\$100$)** | **SLAM navigation, 2D mapping, ROS robots** |
| **Stereo Depth Camera** | **Dual Cameras** | **$90^{\circ} \times 60^{\circ}$** | **$10\text{m}$** | **Extremely High** | **Very High ($>\$150$)** | **3D point clouds, object classification** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x HC-SR04 Ultrasonic Sensor Module
* 1x SG90 Micro Servo Motor (with plastic horns/mounts)
* 1x Breadboard
* Jumper wires
* Hot glue/tape (to temporarily mount the ultrasonic sensor onto the servo horn)

---

## 🔌 Pin-to-Pin Wiring

Make sure your servo connections match:
* Signal -> Yellow/Orange wire
* VCC -> Red wire
* GND -> Brown/Black wire

| Component Pin | Arduino Uno Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **Servo Signal** | **D9** (PWM-capable) | Orange | Servo angular positioning PWM |
| **Servo VCC** | **5V** | Red | Servo power supply |
| **Servo GND** | **GND** | Black | Servo ground return |
| **HC-SR04 VCC** | **5V** | Red | Ultrasonic sensor power |
| **HC-SR04 GND** | **GND** | Black | Ultrasonic sensor ground |
| **HC-SR04 TRIG** | **D11** | Green | Trigger start pulse signal |
| **HC-SR04 ECHO** | **D12** | Yellow | Echo return duration signal |

---

## 💻 How to Test & Validate

1. Wire up the components as detailed in the wiring table. Mount the HC-SR04 sensor onto the top of the servo motor shaft using tape or hot glue, ensuring the sensor wires do not tangle when rotating.
2. Upload `Day_38_Ultrasonic_Radar.ino` to your Arduino.
3. Open the **Serial Monitor** at **9600 Baud**.
4. You will observe the servo motor begin sweeping back and forth.
5. The Serial Monitor will output a rapid stream of data in `Angle,Distance` format:
   ```
   15,24.5
   16,24.8
   17,25.2
   ...
   90,-1.0   <-- Target out of range
   91,35.2
   ```
6. Place an object (like a mug or box) $20\text{ cm}$ in front of the sweep arc and watch the distance values drop at the angles pointing toward the object.
7. **Mapping Visualization:**
   * This output format is directly compatible with the classic **Processing IDE Radar sketch**.
   * By writing a simple script in Processing, you can capture this COM port data to draw a visual green radar display with sweeping sector trails on your computer screen!

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The servo jumps back and forth or resets the Arduino board:**
  * Servos draw high current spikes when starting motion. If powered from the Arduino 5V pin alongside the ultrasonic sensor, it can cause the supply rail to sag, triggering the Arduino's brownout reset. Connect a $100\ \mu\text{F}$ capacitor across the 5V and GND rails on the breadboard, or power the servo from an external battery.
* **The distance values always read `-1.0`:**
  * Check that the TRIG and ECHO pins are wired to pins 11 and 12, respectively.
  * Ensure the sensor is clear of the desk. If aimed at the floor, it will register the floor or return no echo.
* **The servo stops sweeping at the ends ($15^{\circ}$ or $165^{\circ}$) and hums loudly:**
  * SG90 servos occasionally have physical end-stops that differ from the nominal $0^{\circ}-180^{\circ}$ range. If it hits a mechanical limit, it will draw high currents and hum. Modify the code constants `MIN_ANGLE` to `20` and `MAX_ANGLE` to `160` to narrow the sweep limits.

## 🧠 Code Explanation

Let's break down how we use acoustic Time-of-Flight to measure distance:

### 1. The Trigger Pulse
```cpp
digitalWrite(TRIG_PIN, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);
```
- We fire a 10-microsecond 5V pulse into the HC-SR04's Trigger Pin. 
- This wakes up the chip, which then blasts an invisible 8-cycle ultrasonic sound wave (40 kHz) out of the left speaker cone.

### 2. Range-Gated Time of Flight
```cpp
unsigned long pulseDuration = pulseIn(ECHO_PIN, HIGH, 20000);
float distCm = (float)pulseDuration * 0.0343 / 2.0;
```
- `pulseIn()` listens to the Echo pin and times exactly how many microseconds it stays HIGH.
- **The Timeout (20000):** By default, `pulseIn` waits 1000ms (1 full second). If there are no obstacles, our robot will completely freeze for a second! We set a 20ms timeout. In 20ms, sound travels 3.4 meters. If we hear nothing by then, we abort and assume the path is clear.
- **The Math:** We multiply the microseconds by the speed of sound (`0.0343 cm/µs`). We then divide by `2` because the sound had to travel to the wall *and* back!

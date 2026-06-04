# Day 45: Calibrated 5-Sensor IR Array (Centroid Math)

Welcome to Day 45 of the 100-Day Arduino Masterclass! Today, we progress to advanced mobile robotics navigation by interfacing a **5-channel TCRT5000 Infrared (IR) Reflectance Sensor Array**. We will write an **automated calibration routine** to record surface reflections, implement **Min-Max Normalization** equations, and calculate the exact position of the track using **Weighted Average Centroid Math**.

---

## 🎯 The "Why" and "What"

Basic 2-sensor line followers (Day 44) suffer from continuous left/right wobble. They only know if they are completely ON or completely OFF the line, creating an unstable, oscillating trajectory.
* **The Problem:** Sensor heights, surface colors, and battery voltages vary. Hardcoded static analog thresholds fail if ambient lighting changes or the floor surface becomes slightly dirtier.
* **The Solution:** A 5-sensor array spanning the track allows the robot to read a gradient of reflection:
  1. **Auto-Calibration:** On boot, the robot pivots back and forth, recording the exact white and black limits for all five sensors.
  2. **Centroid Math:** Calculates a weighted average center-of-mass, producing a continuous position offset float ($-2.0$ to $+2.0$). 
  3. **Proportional Steering:** Motor speed is adjusted proportionally to the offset, allowing the robot to make micro-adjustments on straight tracks and smooth pivots on sharp corners.

---

## 🔬 Physics & Mathematics

### 1. Min-Max Sensor Normalization
Reflection values vary between individual sensor units. To calibrate them, we record the minimum reading (white floor, `sensorMin`) and maximum reading (black line, `sensorMax`) during setup.
We normalize the raw value to a standard scale of $0$ (white) to $1000$ (black):
$$\text{Normalized Value } (W) = \text{constrain}\left( \frac{\text{Raw Reading} - \text{sensorMin}}{\text{sensorMax} - \text{sensorMin}} \times 1000, \; 0, \; 1000 \right)$$

This filters out differences in sensor alignment and ambient lighting.

---

### 2. Weighted Centroid Math (Line Position)
We assign each sensor a physical coordinate representing its position on the horizontal array (relative to the robot's center axis):

```
       S0 (Far L)   S1 (Mid L)   S2 (Center)   S3 (Mid R)   S4 (Far R)
         -2.0         -1.0           0.0          +1.0         +2.0
```

Let $W_i$ be the normalized weight of sensor $i$ ($0 - 1000$). The **Centroid Position** ($X_c$) is calculated as:
$$X_c = \frac{\sum_{i=0}^{4} (W_i \times x_i)}{\sum_{i=0}^{4} W_i} = \frac{W_0(-2.0) + W_1(-1.0) + W_2(0.0) + W_3(1.0) + W_4(2.0)}{W_0 + W_1 + W_2 + W_3 + W_4}$$

* **Example A (Centered):** Only Center sensor (S2) sees the line ($W_2 = 1000$, others 0):
  $$X_c = \frac{1000 \times 0}{1000} = 0.0$$
* **Example B (Slightly Right):** Line is between center and mid-right ($W_2 = 800, W_3 = 800$, others 0):
  $$X_c = \frac{(800 \times 0) + (800 \times 1)}{1600} = 0.5$$
* **Example C (Far Left):** Line is under S0 ($W_0 = 1000$, others 0):
  $$X_c = \frac{1000 \times (-2)}{1000} = -2.0$$

This yields a smooth, continuous position float, allowing precision tracking.

---

## 🔄 Alternatives Comparison

When selecting line-tracking sensors:

| System Config | Calibration Requirements | Line Position Output | Steering Smoothness | Best Used For |
| :--- | :--- | :--- | :--- | :--- |
| **5-Sensor Analog Array** | **Yes (Auto-Calibration sweep)** | **Continuous Float ($-2.0$ to $+2.0$)** | **High (Proportional drive) (Our choice)** | **High-speed line followers, robotic maze solving** |
| **2-Sensor Analog Array** | **No (Static threshold)** | **Discrete states (3 options)** | **Low (Sinuous oscillation)** | **Low-cost educational toys** |
| **8-Sensor SPI Array (QTR-8D)** | **Yes (Calibration registers)** | **High-precision byte maps** | **Extremely High** | **Micromouse maze solvers, fast competition robots** |
| **OpenCV Camera** | **High (Color masking)** | **Pixel centroid vector** | **Extremely High** | **ROS-based automated forklift guidance** |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x 2WD Robot Chassis (2 DC motors + wheels + caster)
* 1x L298N Dual H-Bridge Driver Module
* 1x 5-Channel TCRT5000 IR Sensor Array Module (analog outputs)
* 1x External Battery Pack (e.g. 2s LiPo or 6x AA battery holder to power motors)
* 1x Breadboard
* Jumper wires
* Black electrical tape (to build a track)

---

## 🔌 Pin-to-Pin Wiring

| Sensor Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **OUT1 (Far Left)** | **A0** | Grey | Sensor 0 analog voltage |
| **OUT2 (Mid Left)** | **A1** | Blue | Sensor 1 analog voltage |
| **OUT3 (Center)** | **A2** | Purple | Sensor 2 analog voltage |
| **OUT4 (Mid Right)**| **A3** | Yellow | Sensor 3 analog voltage |
| **OUT5 (Far Right)**| **A4** | Orange | Sensor 4 analog voltage |
| **VCC (Array)** | **5V** | Red | Array logic power |
| **GND (Array)** | **GND** | Black | Array ground |

| L298N Driver Pin | Arduino Pin | Description |
| :--- | :--- | :--- |
| **ENA** | **D5** (PWM) | Left Motor Speed |
| **IN1 / IN2** | **D4 / D3** | Left Motor Direction |
| **ENB** | **D6** (PWM) | Right Motor Speed |
| **IN3 / IN4** | **D7 / D8** | Right Motor Direction |
| **GND** | **GND** | Shared logic/power Ground |

---

## 💻 How to Test & Validate

1. Wire the components as detailed in the wiring table. Mount the 5-sensor array at the front of the robot.
   * **Sensor Height:** Mount the array **$5\text{mm} - 8\text{mm}$** above the floor.
   * **Sensor Position:** Align the center sensor (A2) exactly with the chassis's center axis.
2. Build a black tape track on a light floor.
3. Place the robot **centered directly on the black line**.
4. Upload `Day_45_Line_Array_Calibrated.ino` to the Arduino.
5. **Observe Auto-Calibration:**
   * On boot, the onboard Pin 13 LED will light up.
   * The robot will slowly pivot back and forth in place for 5 seconds to sweep the sensors across the line.
   * Make sure the sensors cross the black line and white floor completely during this sweep.
   * Once finished, the LED turns off, and the robot halts. The Serial Monitor will print the recorded min/max values.
6. **Telemetry & Tracking:**
   * Open the **Serial Monitor** at **9600 Baud**.
   * It will display the normalized weight arrays and line offset:
     `[WEIGHTS] 1000 120 0 0 0 | Offset: -1.869`
   * Place the robot on the track; it will follow the line smoothly by making micro-speed adjustments!

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The robot pivots in circles indefinitely during calibration and never starts tracking:**
  * One of the sensors is failing to read the black line. Check the printed calibration limits in the Serial Monitor. If the min and max values for a sensor are almost equal, that sensor is broken or too far from the floor.
* **The robot drifts off the line and continues driving straight forward:**
  * Ensure the line-loss backup logic is active. If the `weightSum` drops below `350` (meaning no sensors see black), the robot should stop.
  * Adjust the `LINE_THRESHOLD` or check if ambient sunlight is interfering.
* **The robot steers in the wrong direction (turning away from the line):**
  * The motor pins or sensor coordinates are reversed. Swapping the OUT1 and OUT5 sensor pins or inverting the coordinates in code will correct this:
    `const float SENSOR_COORDINATES[5] = {2.0, 1.0, 0.0, -1.0, -2.0};`

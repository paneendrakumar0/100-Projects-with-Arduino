# Day 7: Ultrasonic Sensor Distance Measurer (HC-SR04)

Welcome to Day 7 of the 100-Day Arduino Masterclass! Today, we explore distance sensing. We will learn how to interface the HC-SR04 ultrasonic distance sensor to measure physical distances using sound waves (sonar).

You will master the physics of sound propagation, learn how to derive distance from time-of-flight measurements, and write robust code that prevents microcontroller lockups using timeout limits.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Servo_Motor.jpg" alt="Servo Motor" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Ultrasonic_Sensor.jpg" alt="Ultrasonic Sensor" width="200" style="margin:10px;" />
</p>

## 🎯 Today's Learning Goals
1. Understand the working principle of ultrasonic transducers and echolocation.
2. Master the physics of the speed of sound in air and its temperature dependence.
3. Wire the HC-SR04 sensor and trigger it with sub-millisecond precision.
4. Implement a time-of-flight calculation in C++.
5. Prevent CPU blocking by configuring `pulseIn()` timeouts.

---

## 🧠 The "Why" and "What": Ultrasonic Sensors in Robotics

### What is the HC-SR04?
The HC-SR04 is an active ultrasonic distance sensor that uses high-frequency sound waves (40 kHz, which is well above human hearing) to measure distance. It consists of two circular metal cylinders: one acts as a transmitter (speaker) and the other as a receiver (microphone).

### Why is it Used in Robotics?
In robotics, measuring the distance to objects is fundamental for spatial awareness and navigation:
- **Obstacle Avoidance:** Mobile robots (like vacuum cleaners or rovers) mount ultrasonic sensors on the front, left, and right to scan the path ahead and steer away from walls or furniture.
- **Mapping (SLAM):** Simple 2D mapping robots spin an ultrasonic sensor on a servo motor to measure distance at various angles, drawing a layout of the room.
- **Tank Level Sensing:** Measuring the distance from the top of a tank to the surface of liquid or grain inside to track volume.
- **Parking Sensors:** Back-up alarm systems that beep faster as a vehicle approaches a wall.

---

## ⚡ The Physics & Hardware Theory

### 1. The Physics of Sound Waves
Sound is a mechanical wave that travels through a medium (like air) by compressing and expanding air molecules. 

The speed of sound ($v$) is not constant; it depends primarily on the temperature ($T$) of the air. At sea level, the speed of sound can be calculated using the formula:

$$v = 331.3 + 0.6 \cdot T \quad (\text{in m/s})$$

Where $T$ is the temperature in degrees Celsius (°C).
* At **0°C (Freezing):** $v = 331.3\text{ m/s}$ (or $0.03313\text{ cm/µs}$)
* At **20°C (Room Temp):** $v = 331.3 + 0.6(20) = 343.3\text{ m/s}$ (or $0.03433\text{ cm/µs}$)

### 2. Time-of-Flight Derivation
The HC-SR04 measures the **round-trip time-of-flight** ($\Delta t$). The sound wave is sent from the transmitter, travels to the obstacle, bounces off it, and travels back to the receiver.

```
                  =============================================
                  |              HC-SR04 Sensor               |
                  |   [Transmitter]             [Receiver]    |
                  =============================================
                           \                         /
                            \                       /
                             \                     /
    Outbound Sound Wave (40kHz) \                   / Inbound Echo Wave
                                 \                 /
                                  v               /
                             ===========================
                             |     Obstacle Target     |
                             ===========================
```

Therefore, the distance ($d$) to the target is exactly **half** of the total distance traveled by the sound wave:

$$\text{Distance } (d) = \frac{\text{Speed } (v) \times \text{Time } (\Delta t)}{2}$$

Using the speed of sound at 20°C ($0.03433\text{ cm/\mu s}$):

$$d = \frac{0.03433 \times \Delta t}{2} = \frac{\Delta t}{2 / 0.03433} = \frac{\Delta t}{58.26} \quad (\text{in cm})$$

To convert to inches, knowing that $1\text{ inch} = 2.54\text{ cm}$:

$$d = \frac{\Delta t}{58.26 \times 2.54} = \frac{\Delta t}{148.0} \quad (\text{in inches})$$

### 3. Preventing Microcontroller Lockups
The Arduino uses the built-in function `pulseIn(pin, HIGH)` to measure how long the Echo pin stays high. 
By default, `pulseIn()` has a timeout of **1 second**. If you point the sensor into empty space where there is no obstacle, the sound wave will travel forever and never bounce back. 

Without a custom timeout, the Arduino will freeze and wait for 1 full second on every loop iteration, halting all motor controls and other sensors.

To prevent this, we calculate the maximum duration we care about. The HC-SR04 has a maximum reliable range of about **4 meters**. A round trip of 4.5 meters (450 cm) takes:

$$\Delta t_{max} = 450\text{ cm} \times 58.3 \approx 26,235\text{ µs}$$

By passing **26000** as the third parameter to `pulseIn(ECHO_PIN, HIGH, 26000)`, we limit the wait time to 26 milliseconds. If no echo returns in that time, the function immediately returns `0`, and the loop continues running smoothly.

---

## 🔄 Alternatives: Sonar vs. Infrared vs. LiDAR

| Sensor Type | Technology | Measuring Range | Accuracy | Effect of Ambient Light / Color | Cost | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **HC-SR04 Sonar** | Ultrasonic sound waves. | $2\text{ cm to }400\text{ cm}$ | $\approx 3\text{ mm}$ | None. Works in complete darkness; color/transparency of target does not matter. | Very Low | **Chosen** for obstacle detection, tank level sensing, and cost-effective mobile robotics. |
| **Sharp GP2Y0A21 IR** | Infrared light triangulation. | $10\text{ cm to }80\text{ cm}$ | Moderate | High. Dark targets absorb the IR light, giving faulty readings. Ambient sunlight degrades performance. | Moderate | Short-range proximity detection in indoor-only systems. |
| **VL53L0X LiDAR** | Laser Time-of-Flight (light speed). | $3\text{ cm to }200\text{ cm}$ | $\approx 1\text{ mm}$ | Low. Laser light is highly focused, giving millimeter-precision. | High | Precision mapping, height sensors for drones, exact positioning. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **HC-SR04 Ultrasonic Distance Sensor**.
3. **Breadboard & Jumper Wires**.
4. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

| Sensor Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **5V** | Red | Power supply (+5V) |
| **Trig** | **Pin 3** | Green / Yellow | Trigger input pulse line |
| **Echo** | **Pin 4** | Blue | Echo output pulse line |
| **GND** | **GND** | Black | Ground reference |

---

## 🧪 How to Test and Validate

Follow these steps to upload, run, and verify the distance measurer:

### 1. Physical Sensor Alignment
- Insert the HC-SR04 directly into the breadboard so the cylinders face outward.
- Ensure there are no loose wires dangling directly in front of the transmitter or receiver, as they will reflect the sound and cause false close-range readings.

### 2. Verification of Measurements
- Upload `Day_07_Ultrasonic_Distance.ino`.
- Open the Serial Monitor (**Tools > Serial Monitor**) at **9600 Baud**.
- Place a flat, solid object (like a notebook or box) directly in front of the sensor.
- **Move the target object:**
  - Hold the target at 10 cm, 20 cm, and 50 cm. Verify the printed distance in the monitor matches a physical ruler:
    ```text
    Echo Duration: 1166 us | Distance: 20.0 cm (7.9 in)
    ```
- **Test the boundaries:**
  - Move the object very close (less than 2 cm). The sensor will report inaccurate data or error because sound waves cannot bounce and settle within 2 cm.
  - Remove the obstacle completely and point the sensor at a distant wall. If the distance is greater than 4.5 meters, you should see:
    ```text
    [ERROR] Out of range or no obstacle detected.
    ```

### 🔍 Troubleshooting Tips
* **The reading fluctuates wildly (e.g. jumping between 10cm and 300cm):**
  - Sound waves reflect off flat surfaces at angles. If the sensor is pointing at an angle to the target, the sound wave will bounce away and not return, or bounce off a different object. Ensure the sensor is perpendicular to the target.
  - Fabric, sponge, and soft clothing absorb sound waves instead of reflecting them. Test using a hard, flat surface (like a book).
* **The serial monitor prints "Out of range" constantly:**
  - Check your Echo and Trig wiring. Verify Trig is connected to Pin 3 and Echo to Pin 4.
  - Make sure the sensor is getting exactly 5V. The HC-SR04 will fail to operate on 3.3V power.

## 🧠 Code Explanation

Let's break down how we measure distance using sound:

### 1. Triggering the Pulse
```cpp
digitalWrite(TRIG_PIN, LOW);
delayMicroseconds(2);
digitalWrite(TRIG_PIN, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);
```
- The sensor needs a very specific wake-up call. We hold the Trigger pin `LOW` to ensure it's clean, then blast it `HIGH` for exactly 10 microseconds. This tells the sensor: *"Fire the ultrasonic burst now!"*

### 2. Reading the Echo
```cpp
unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT);
```
- `pulseIn()` listens to a pin and waits for it to go `HIGH`. Once it goes HIGH, it starts a stopwatch and stops it when it goes `LOW`. It returns the duration in microseconds.
- `ECHO_TIMEOUT`: We set a hard timeout of 26,000µs. If we don't hear an echo by then, it means the sound travelled into the void (over 4.5 meters). This prevents our Arduino from freezing forever while waiting for a ghost echo.

### 3. The Math
```cpp
float distanceCm = duration / 58.3;
```
- Speed of sound = 343 m/s. 
- The sound travels TO the object and BACK, so we divide the total time by 2.
- After converting units from seconds/meters to microseconds/centimeters, the magic constant simplifies to `58.3`!

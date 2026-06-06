# Day 12: Potentiometer-Controlled Servo Motor (Deadband Filtering)

Welcome to Day 12 of the 100-Day Arduino Masterclass! Today, we combine our knowledge of analog inputs (Day 4) and servo actuators (Day 11) to build a manual steering system. 

We will create a control interface where rotating a potentiometer dial directly steers the angle of a servo motor. Most importantly, you will master **deadband noise filtering**—a professional coding technique used to eliminate electrical jitter, stop servo buzzing, and extend the lifespan of mechanical gears.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/DC_Motor.jpg" alt="DC Motor" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Joystick.jpg" alt="Joystick" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Robotic_Arm.jpg" alt="Robotic Arm" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Rotary_Encoder.jpg" alt="Rotary Encoder" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Servo_Motor.jpg" alt="Servo Motor" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Stepper_Motor.jpg" alt="Stepper Motor" width="200" style="margin:10px;" />
</p>

## 🎯 Today's Learning Goals
1. Interface a potentiometer and servo motor on a single breadboard.
2. Understand the concept of ADC jitter and electrical noise propagation.
3. Master the mathematics of a **deadband filter** using absolute differences.
4. Learn how decoupling capacitors stabilize voltage rails in mechatronic systems.
5. Calibrate the analog-to-angle travel limits.

---

## 🧠 The "Why" and "What": Potentiometer-Servo Control

### What is Potentiometer-Servo Control?
This is a manual positioning system where an analog sensor (the potentiometer) acts as a physical setpoint input, and a closed-loop actuator (the servo) acts as the output. The Arduino acts as the controller, mapping the sensor voltage to the actuator target angle.

### Why is it Used in Robotics?
This master-slave positioning concept is the foundation of robotic teleoperation and steering:
- **Robotic Arm Teleoperation:** A human wears a lightweight exoskeleton arm fitted with potentiometers at the joints. As the human moves, the Arduino reads the pot angles and commands high-power servos on a remote robot to mirror those exact movements.
- **Remote RC Steering Controllers:** The steering wheel on an RC transmitter rotates a potentiometer, which sends signals to steer the servo on the RC car.
- **Manual Camera Gimbals:** Adjusting camera tilt angle using a dial on a control panel.
- **Prototyping Joint Kinematics:** Testing joint ranges and checking mechanical clearances by manually driving a joint with a dial before writing automated path trajectories.

---

## ⚡ The Physics & Hardware Theory

### 1. The Physics of ADC Jitter and Electrical Noise
Why do servos buzz when connected to basic potentiometer codes?
Analog signals are vulnerable to electrical noise. Even if a potentiometer shaft is held completely still, the voltage measured at Pin A0 will fluctuate slightly due to:
* Electromagnetic interference (EMI) from nearby laptop power bricks or wires.
* Fluctuations in the Arduino's 5V reference voltage caused by the servo's own current draw.
* Thermal noise in the ADC.

This noise causes the 10-bit ADC reading to fluctuate by 1 or 2 steps (e.g., bouncing between `512` and `513`). Without a filter, this is mapped to a change in servo target angle. The servo's internal controller tries to correct this micro-step, driving the internal DC motor back and forth thousands of times a minute. This results in the high-pitched **buzzing** or humming sound, wastes battery, generates heat, and wears down the plastic gear teeth.

### 2. The Deadband Filter Algorithm
A **deadband** (or dead zone) is a range of input values where no action is taken. In our code, we define a deadband threshold of `4` (out of 1023, which is about $0.4\%$ of the total range).

```
                     The Deadband Filter Loop
                     
       New Sample (potValue) ➡️ [ Calculate abs(potValue - lastPotValue) ]
                                             |
                            +----------------+----------------+
                            |                                 |
                     Difference > DEADBAND            Difference <= DEADBAND
                            |                                 |
                            v                                 v
                 Update lastPotValue &                     [Ignore]
                  Write to Servo Motor                   Do nothing!
```

We calculate the absolute difference between the new raw reading and the last recorded stable reading using the `abs()` function:

$$\text{Difference} = | \text{potValue} - \text{lastPotValue} |$$

* If the difference is **$\le 4$**: We assume it is noise. We discard the reading and make no changes. The servo remains silent.
* If the difference is **$> 4$**: We assume it is a real human movement. We update `lastPotValue = potValue`, map the angle, and drive the servo.

### 3. Decoupling Capacitors (Hardware Filtering)
To further clean up the electrical noise caused by the servo motor's inductive spikes, we can place a **decoupling capacitor** ($10\text{ to }100\text{ µF}$) across the 5V and GND power rails on the breadboard.

```
       5V Rail ----------------+------------➡️ Servo VCC (Red)
                               |
                            [=] 100µF Capacitor (Electrolytic)
                               |
      GND Rail ----------------+------------➡️ Servo GND (Brown)
```

The capacitor acts as a tiny local reservoir of electrical charge. When the servo motor starts moving and draws a quick spike of current, the capacitor discharges to supply that current, preventing the 5V rail voltage from sagging. When the motor stops, the capacitor recharges. This stabilizes the reference voltage, leading to clean ADC readings.

---

## 🔄 Alternatives: Potentiometers vs. Encoders vs. Joysticks

| Control Input | Output Type | Rotation Limits | Jitter Sensitivity | Programming Complexity | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Potentiometer** | Analog (0-5V) | Limited (usually 270°) | High (needs deadband filter) | Low | **Chosen** for absolute angle tracking and simple steering dials. |
| **Rotary Encoder** | Digital (pulses) | Infinite rotation | Zero | Moderate (needs interrupt loops) | Stepper motor target adjusters, infinite menu wheels. |
| **2-Axis Joystick** | Analog (X and Y voltages) | Self-centering spring | High | Low | Game controllers, steering mobile rovers, pan-tilt gimbals. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **SG90 Micro Servo Motor**.
3. **10kΩ Rotary Potentiometer**.
4. **100µF Electrolytic Capacitor** (*Optional but recommended for stability*).
5. **Breadboard & Jumper Wires**.
6. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

Ensure the potentiometer and the servo do not share breadboard rows except for the common power rails.

| Component | Pin Label | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Potentiometer** | Pin 1 (Left Terminal) | **5V** | Red | Power rail connection |
| **Potentiometer** | Pin 2 (Middle Wiper) | **A0** | Yellow | Analog voltage output |
| **Potentiometer** | Pin 3 (Right Terminal) | **GND** | Black | Ground rail connection |
| **Servo** | Signal (Orange/Yellow) | **Pin 9** | Orange | PPM control line |
| **Servo** | VCC (+) (Red) | **5V** | Red | Power supply |
| **Servo** | GND (-) (Brown/Black) | **GND** | Black | Ground connection |

*Optional Capacitor:* Connect the longer leg (+) of a $100\text{µF}$ capacitor to the breadboard 5V rail, and the shorter marked leg (-) to the GND rail.

---

## 🧪 How to Test and Validate

Follow these steps to run, verify, and tune your steering system:

### 1. Verify Jitter Reduction (The Silence Test)
- Upload `Day_12_Pot_Servo.ino`.
- **Hold the potentiometer still.**
  - The servo horn should remain completely silent and motionless. Touch the servo casing; you should feel no internal motor vibrations.
  - If the servo is constantly humming and micro-vibrating, increase the `DEADBAND` value in the code to `6` or `8`.

### 2. Verify Coordinate Tracking
- Open the Serial Monitor at **9600 Baud**.
- Slowly rotate the potentiometer shaft all the way counterclockwise.
  - The servo horn should rotate smoothly to $0^{\circ}$.
  - The monitor should print: `Pot ADC: 0 | Target Angle: 0°`.
- Slowly turn the pot clockwise.
  - The servo horn should track your hand movement in real-time, reaching $180^{\circ}$ at the clockwise limit.
  - The monitor should print: `Pot ADC: 1023 | Target Angle: 180°`.

### 🔍 Troubleshooting Tips
* **The servo goes crazy, spinning back and forth randomly:**
  - Verify that the potentiometer middle wiper pin is connected to **Pin A0**. If connected to a digital pin or left floating, it will read noise and drive the servo erratically.
  - Ensure the Ground wires are connected properly. A missing common ground between the potentiometer and the servo will cause signal reference loss.
* **The servo sweeps but doesn't reach the full 180° range:**
  - Cheap SG90 servos often have physical ranges restricted to $160^{\circ}$ or $170^{\circ}$ due to internal mechanical endstops. If the servo grinds at the extremes, reduce the mapping output range in code, e.g., change `map(potValue, 0, 1023, 10, 170)`.
* **The servo movement direction is backward relative to my hand:**
  - To reverse the mapping direction, swap the two outer wires on the potentiometer, or change the map function in code to `map(potValue, 0, 1023, 180, 0)`.

## 🧠 Code Explanation

Let's look at the professional way to link an analog input to a servo output:

### 1. The Deadband Noise Filter
```cpp
const int DEADBAND = 4;
int potValue = analogRead(POT_PIN);

if (abs(potValue - lastPotValue) > DEADBAND) {
    // Process the new value
}
```
- Potentiometers are noisy. Even when you aren't touching it, the ADC reading might bounce between `511`, `512`, and `513`.
- If we sent every single bounce to the servo, it would hum, grind its internal gears, overheat, and draw massive current spikes from your power supply.
- We calculate the absolute difference `abs(current - last)`. If the change is smaller than our `DEADBAND` (4 units), we ignore it! The servo remains completely silent until you intentionally turn the knob.

### 2. Mapping the Angle
```cpp
int targetAngle = map(potValue, 0, 1023, 0, 180);
myServo.write(targetAngle);
```
- The ADC gives us `0` to `1023`. The Servo needs `0` to `180`. The `map()` function mathematically squishes the larger range into the smaller range instantly.

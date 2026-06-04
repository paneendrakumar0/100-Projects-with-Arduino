# Day 11: Servo Motor Sweep (PPM Control)

Welcome to Day 11 of the 100-Day Arduino Masterclass! Today, we enter the world of actuation. We will learn how to interface a hobby servo motor with the Arduino and program a smooth sweep across its physical range of motion.

You will master closed-loop control systems, study Pulse Position Modulation (PPM) waveforms, learn how to prevent board brownouts under heavy inductive loads, and write a completely non-blocking sweep algorithm.

---

## 🎯 Today's Learning Goals
1. Understand the difference between open-loop and closed-loop control systems.
2. Master Pulse Position Modulation (PPM) timing constraints (50 Hz, 1-2ms pulse width).
3. Identify the internal mechanics of a hobby servo (DC motor, gears, feedback pot, error amp).
4. Wire a servo safely and learn when to isolate power lines.
5. Program a non-blocking angle updates sweep using `millis()`.

---

## 🧠 The "Why" and "What": Servos in Robotics

### What is a Servo Motor?
A hobby servo motor is a self-contained rotary actuator that provides precise angular positioning. Unlike standard DC motors which spin continuously when given voltage, a servo motor can be instructed to rotate to a specific angle (typically between $0^{\circ}$ and $180^{\circ}$) and hold that position against external mechanical resistance.

### Why is it Used in Robotics?
Servos are the primary muscle behind joints and steerage in mechatronics:
- **Robotic Joints:** Controlling the shoulder, elbow, and wrist movement of robotic arms.
- **Steering Knuckles:** Directing the front wheel alignment of RC cars and autonomous rovers.
- **Flight Surfaces:** Operating the ailerons, elevators, and rudders on RC airplanes and fixed-wing drones.
- **Camera Pan-Tilt Gimbal Systems:** Steering camera angles to track objects or stabilize footage.
- **Grippers:** Opening and closing claws to grab and release objects.

---

## ⚡ The Physics & Hardware Theory

### 1. Open-Loop vs. Closed-Loop Control
To understand a servo, we must understand control theory.
* **Open-Loop Control:** You send a command to an actuator, but you have no sensor to confirm if the actuator actually moved. 
  - *Example:* A standard DC motor. If you write `analogWrite(pin, 128)`, you expect it to spin at half speed, but if it gets stuck, the controller does not know.
* **Closed-Loop Control (The Servo):** You send a command (e.g., go to $90^{\circ}$). The actuator moves, measures its actual position using an internal sensor (potentiometer), compares the actual position to the commanded position (error calculation), and automatically drives the motor until the error is zero.

```
       Closed-Loop Servo System Block Diagram
       
       Target Angle  +-------+  Motor Drive  +---------+   Actual Position
       ------------➡️ | Error | -------------➡️ |  Motor  | -----------------➡️ Output Shaft
         (Signal)    |  Amp  |                +---------+         |
                     +-------+                             |
                         ^                                 v
                         |--------------------------- [Feedback] (Internal Pot)
```

### 2. Pulse Position Modulation (PPM) Control Timing
Servos are controlled by sending a specific pulse train. This is technically **Pulse Position Modulation (PPM)** rather than standard duty-cycle PWM.

The Arduino sends a pulse every **20 milliseconds** ($50\text{ Hz}$ frequency). The width of the high pulse (ON time) determines the target shaft position:
* **1.0 millisecond (1000 µs) Pulse:** Target is **$0^{\circ}$**.
* **1.5 millisecond (1500 µs) Pulse:** Target is **$90^{\circ}$** (Center/Neutral).
* **2.0 millisecond (2000 µs) Pulse:** Target is **$180^{\circ}$**.

```
PPM Servo Timing Waves:

   0° (1.0ms):     |--|__________________||--|__________________||--| (Repeat 50Hz)
                   <------ 20ms Frame ---->
                   
  90° (1.5ms):     |---|_________________||---|_________________||---|
  
 180° (2.0ms):     |----|________________||----|________________||----|
```

If you send a pulse width outside the 1.0ms - 2.0ms range, the servo will try to rotate past its physical limits, hitting mechanical endstops, drawing maximum stall current, and stripping its gears.

### 3. Inductive Load and Power Supply Isolation
A servo contains a DC motor (an **inductive load** consisting of copper coils). When a motor starts up or fights a load, it draws a massive spike of electrical current (known as **inrush / stall current**).

An SG90 micro servo draws around $100\text{ to }250\text{ mA}$ under sweep, which is safe to power directly from the Arduino's 5V pin. 
However, standard and high-torque servos (like the MG996R) draw **$1.0\text{ to }2.5\text{ Amps}$** when stalling. If powered from the Arduino 5V pin:
- The current draw will exceed the Arduino voltage regulator's limits.
- The 5V line voltage will sag (drop below 4V).
- The Arduino microcontroller (ATmega328P) will experience a **brownout** and immediately reset.
- Inductive noise (voltage spikes) can travel back into the Arduino, damaging the digital pins.

> [!IMPORTANT]
> **Safety Rule:** For any project using more than one servo or using servos larger than an SG90, you **must** use an external 5V - 6V battery pack or power supply. Connect the external power positive wire to the servo VCC, and connect the external power Ground to BOTH the servo GND and the Arduino GND. This common ground is essential so the PPM signal has a reference path.

---

## 🔄 Alternatives: Servos vs. Steppers vs. DC Motors

| Motor Type | Control Type | Speed | Torque | Rotation Limits | Feedback | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Servo Motor** | Closed-Loop | Moderate | High | Limited ($0\text{ to }180^{\circ}$ standard) | Yes (internal potentiometer). | **Chosen** for robotic joints, steering linkages, and pan-tilt systems. |
| **Stepper Motor** | Open-Loop | Low to Moderate | High | Infinite rotation | No (requires homing sensor). | 3D printer axes, CNC machines, and precision positioning. |
| **Standard DC Motor** | Open-Loop | Very High | Low | Infinite rotation | No (requires external encoder). | Wheel drive shafts on rovers, fans, propellers. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **SG90 Micro Servo Motor** (standard 9g analog hobby servo).
3. **Breadboard & Jumper Wires**.
4. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

Look at your servo cable. It consists of three colored wires. Most hobby servos follow one of these color schemes:

| Connection Type | Servo Wire Color (Standard) | Servo Wire Color (Alt) | Connect To | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Signal (PPM)** | **Orange** | Yellow | Arduino **Pin 9** | PPM pulse command line |
| **VCC (+5V)** | **Red** | Red | Arduino **5V** (or external 5V) | Motor power supply |
| **GND (0V)** | **Brown** | Black | Arduino **GND** (shared GND) | Ground reference |

---

## 🧪 How to Test and Validate

Follow these steps to upload and verify the non-blocking sweep:

### 1. Mounting the Servo Horn
- Push a plastic arm (horn) gently onto the output spline of the servo. Do not screw it in yet.
- This horn acts as a visual indicator so you can clearly see the angle changes.

### 2. Verify Sweep Motion
- Upload `Day_11_Servo_Sweep.ino` to the Arduino.
- Once uploaded, the servo horn should begin rotating back and forth smoothly.
- **Verify Angle Range:** The horn should sweep exactly half of a circle ($180^{\circ}$) from one end to the other, pause briefly, reverse, and sweep back.
- **Verify Smoothness:** The motion should feel continuous and quiet. There should be no clicking or grinding sounds.

### 3. Verify Serial Telemetry
- Open the Serial Monitor at **9600 Baud**.
- Verify the angle logs update:
  ```text
  Target Angle: 45° | Direction: CW (+) | Est. PPM Pulse: 1250 us
  ```
- Check boundary logs:
  - When the angle reaches $180^{\circ}$, it should log: `>> Reached 180° Limit. Reversing... <<`
  - When the angle reaches $0^{\circ}$, it should log: `>> Reached 0° Limit. Reversing... <<`

### 🔍 Troubleshooting Tips
* **The servo vibrates at one end and makes a buzzing sound:**
  - This is called **binding**. The code is sending a pulse width ($1000\text{ µs}$ or $2000\text{ µs}$) that corresponds to an angle outside the physical range of your specific cheap servo clone.
  - **The Fix:** Adjust the calibration parameters in setup. In Arduino, you can limit the pulse range when attaching: `myServo.attach(SERVO_PIN, 1100, 1900);` to restrict the travel and protect the motor.
* **The Arduino resets (blinks off and on) as soon as the servo starts moving:**
  - The servo is drawing too much current, causing a voltage brownout. 
  - **The Fix:** Connect a $100\text{ µF}$ or larger electrolytic capacitor across the 5V and GND rails of your breadboard to buffer the current spikes, or switch to an external power supply.
* **The servo sweeps but is very slow or jerky:**
  - Make sure the wire colors are correct. If you swap GND and Signal, the motor will not function.
  - Check that the `stepDelay` in the code is set to `15`. If it's too high, the sweep will feel stepped rather than smooth.

# Day 76: Fall Detection System (MPU6050 FSM Delineation)

Welcome to Day 76! Today we build an **IMU-Based Fall Detection System** using the **MPU6050 accelerometer**. We implement a structured **Finite State Machine (FSM)** in software to track three distinct physical phases of a fall (Free-fall, Impact, and Inactivity/Tilt). We write the vector mathematics from scratch and implement a custom CLI tool to simulate falls on the command line.

---

## 🎯 The "Why" and "What"

Fall detection is a vital mechatronic subsystem in:
1. **Robotic Safety**: If a mobile humanoid robot, quadraped, or AGV loses balance and falls, the system must immediately detect the event, shut off high-power actuator currents to prevent motor burn-outs, retract sensitive appendages (like camera gimbals), and raise an alarm.
2. **Medical Wearables**: Personal emergency response systems (PERS) worn by elderly patients to automatically dispatch aid when a fall and subsequent lack of motion are detected.

A single accelerometer threshold is highly prone to false alarms (e.g., jumping or walking down stairs can trigger high-g flags). This project resolves this by requiring a **strict chronological sequence** of physical states before confirming a fall.

---

## 🔬 Physics & Hardware Theory

### 1. The Physics of a Fall (Three-Phase Sequence)
A true fall is characterized by three sequential physical events:

```
    Phase 1: Free-Fall          Phase 2: Impact         Phase 3: Inactivity
   ┌───────────────────┐     ┌──────────────────┐     ┌─────────────────────┐
   │ a_tot drops < 0.4g│ ──► │ a_tot spikes > 3g│ ──► │ Still & Tilted      │
   │ (due to descent)  │     │ (ground collision│     │ (subject lying down)│
   └───────────────────┘     └──────────────────┘     └─────────────────────┘
```

1. **Free-Fall Phase**: As an object begins to fall, it accelerates downwards under gravity. In the frame of reference of the accelerometer, the gravitational acceleration vector is cancelled out (weightlessness). The total acceleration magnitude drops close to $0.0g$. We set a threshold of **$a_{tot} < 0.4g$** lasting for at least **$80\,\text{ms}$**.
2. **Impact Phase**: When the object strikes the ground, it undergoes a massive, rapid deceleration. This causes a massive G-force spike. We look for **$a_{tot} > 2.8g$** within **$500\,\text{ms}$** of the free-fall trigger.
3. **Inactivity & Tilt Phase**: After an impact, a fallen subject/robot remains stationary. The acceleration magnitude settles back to exactly $1.0g$ (gravity), with very low variance (no motion, **$\text{variance} < 0.15g$**). Crucially, the sensor's physical orientation must be tilted, showing the subject is lying down. We require a tilt angle **$> 45^\circ$** for at least **$2.0\,\text{seconds}$**.

### 2. Mathematical Vector Analysis
- **Acceleration Magnitude ($a_{tot}$)**: Combines three orthogonal sensor channels into a single coordinate-independent vector representing total G-forces:
  $$a_{tot} = \sqrt{a_x^2 + a_y^2 + a_z^2}$$
- **Tilt Angle ($\theta_{tilt}$)**: Calculates the angle between the sensor's vertical axis ($Z$-axis) and the gravity vector:
  $$\theta_{tilt} = \arccos\!\left(\frac{a_z}{a_{tot}}\right) \cdot \frac{180}{\pi}$$
  - Upright position: $a_z \approx 1.0g \rightarrow \theta_{tilt} \approx 0^\circ$
  - Lying flat: $a_z \approx 0.0g \rightarrow \theta_{tilt} \approx 90^\circ$

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | FSM Controller |
| MPU6050 IMU Module | 1 | 3-axis Accelerometer sensor |
| Active Buzzer | 1 | Audible alarm indicator |
| Red LED | 1 | Visual alarm indicator |
| 220 Ω Resistor | 2 | Current-limiting resistors |
| Breadboard & Wires | 1 | Prototyping |

---

## 🔌 Pin-to-Pin Wiring

| MPU6050 Pin | Buzzer / LED Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | - | **5V** | Power supply |
| **GND** | LED Cathode / Buzzer (-) | **GND** | Common Ground |
| **SCL** | - | **A5** | I2C Serial Clock |
| **SDA** | - | **A4** | I2C Serial Data |
| **AD0** | - | **GND** | Sets I2C address to `0x68` |
| - | **Buzzer (+)** | **D8** | Buzzer control output |
| - | **Red LED Anode** | **D6** | LED control output (via 220Ω resistor) |

---

## 💾 Alternatives to IMU Fall Detection

| Method | Range/Accuracy | Computational Cost | Cost | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **IMU FSM (Our choice)** | Local / High | Low (algebraic) | Very Low | Highly reliable when sequentially processed in FSM. |
| **Barometric Pressure** | High accuracy | Very Low | Low | Measures altitude drops. Slow to react to short falls. |
| **Wearable Button** | Manual only | Zero | Very Low | Requires the user to remain conscious to press it. |
| **Computer Vision** | Room-scale | Extremely High | High | Camera checks for horizontal pose. Vulnerable to occlusion. |

---

## 💻 How to Test & Validate

1. Wire the MPU6050 and the indicators to the Arduino Uno.
2. Upload [Day_76_Fall_Detection.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_76_Fall_Detection/Day_76_Fall_Detection.ino).
3. Open the **Serial Monitor** at **9600 Baud**. Keep the board stationary.
4. **Physical Test**:
   - Hold the board upright ($X=0, Y=0, Z=1.0g$).
   - Drop the board onto a cushion or drop it from a short height ($15\,\text{cm}$) onto your hand.
   - You should see the console log `[FSM] -> FREE-FALL DETECTED`, then `[FSM] -> IMPACT DETECTED!`.
   - Keep the board still in a tilted position (tilted $>45^\circ$). After 2 seconds, the buzzer pulses and the red LED lights up: `!!! FALL DETECTED !!! ALARM TRIGGERED`.
5. **Console Simulator Test (No Hardware Required)**:
   - Type `m` in the Serial input line to force **Simulation Mode**.
   - Inject Free-Fall: Type `s 0.1 0.1 0.1`. Console changes to `FREE_FALL`.
   - Wait 100ms and inject Impact: Type `s 2.0 2.0 2.0` (magnitude $a_{tot} = 3.46g$). Console updates to `INACTIVITY_CHECK`.
   - Inject Still Lying down: Type `s 0.7 0.7 0.1` (magnitude $a_{tot} = 0.99g$, $a_z = 0.1g$, tilt $= 84^\circ$).
   - Keep sending `s 0.7 0.7 0.1` for 2 seconds. The alarm will trigger!
   - Type `r` to reset the FSM.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| False alarms occur during normal movements | Free-fall time too short or impact threshold too low | Increase `TIME_FREE_FALL_MIN` (e.g. 100ms) or raise `THRES_IMPACT` (e.g. 3.2g). |
| Alarm fails to trigger after a valid drop | Subject recovery reset the timer | Ensure the sensor is kept completely still after the drop. Any shake during inactivity check will reset the 2.0s timer. |
| Inactivity check times out without alarm | Sensor landed upright | The tilt threshold requires orientation tilt $>45^\circ$. If the sensor lands flat standing upright ($Z \approx 1g$), it is not flagged as a fall. |
| Buzzer click is quiet | Output current limit | The Arduino pin provides max 40mA. If using a high-draw buzzer, drive it using an NPN transistor (like 2N2222) with collector to 5V. |

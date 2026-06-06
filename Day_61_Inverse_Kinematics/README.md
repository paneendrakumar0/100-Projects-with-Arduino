# Day 61: Robotic Arm Inverse Kinematics (2-DOF Geometric IK Solver)

Welcome to Day 61! Today we implement **Inverse Kinematics (IK)** — the mathematical process of computing joint angles to reach a desired end-effector position in space. Using a **joystick to control a target point in Cartesian coordinates (x, y)**, the Arduino calculates the required shoulder and elbow servo angles in real-time using the geometric method (law of cosines + atan2). This is the fundamental algorithm behind all robotic arms from industrial welders to surgical robots.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Joystick.jpg" alt="Joystick" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Robotic_Arm.jpg" alt="Robotic Arm" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Servo_Motor.jpg" alt="Servo Motor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

* **Forward Kinematics (FK):** *"Given joint angles, where is the tip?"* — easy.
* **Inverse Kinematics (IK):** *"Given where I want the tip, what angles do I need?"* — much harder, requires math.

Without IK, a joystick would control each servo angle independently. The arm's tip would move in arcs, not straight lines, and the user would need to know "how much to rotate joint 1 AND joint 2 simultaneously to move in the X direction" — almost impossible to use intuitively. With IK, the joystick controls a **Cartesian (x, y) coordinate** and the math figures out the rest.

---

## 🔬 Physics & Mathematics — Step-by-Step Derivation

### Arm Geometry
```
      Shoulder                 Elbow              End Effector
         O ——————————————————— O ————————————————— *
         |        L1           |        L2         |
         |← θ₁ (shoulder angle)|← θ₂ (elbow angle)|
```

### Step 1: Distance to Target
Let the target be at $(x, y)$ relative to the shoulder. The straight-line distance from shoulder to target:
$$d = \sqrt{x^2 + y^2}$$

### Step 2: Elbow Angle via Law of Cosines
Triangle formed by Link 1, Link 2, and the straight line $d$:
$$d^2 = L_1^2 + L_2^2 - 2 L_1 L_2 \cos(\pi - \theta_2) = L_1^2 + L_2^2 + 2 L_1 L_2 \cos(\theta_2)$$

Wait — more directly:
$$\cos(\theta_2) = \frac{d^2 - L_1^2 - L_2^2}{2 L_1 L_2}$$
$$\theta_2 = \arccos\!\left(\frac{d^2 - L_1^2 - L_2^2}{2 L_1 L_2}\right)$$

### Step 3: Shoulder Angle via atan2
Define auxiliary values:
$$k_1 = L_1 + L_2 \cos(\theta_2), \quad k_2 = L_2 \sin(\theta_2)$$
$$\theta_1 = \text{atan2}(y, x) - \text{atan2}(k_2, k_1)$$

`atan2` is used instead of `atan` to correctly handle all four quadrants.

### Step 4: Workspace (Reachable Region)
The arm can only reach points within an annulus:
$$|L_1 - L_2| \leq d \leq L_1 + L_2$$

| Boundary | Condition | Pose |
| :--- | :--- | :--- |
| **Outer radius** | $d = L_1 + L_2$ | Arm fully extended (singular) |
| **Inner radius** | $d = |L_1 - L_2|$ | Arm fully folded back (singular) |
| **Unreachable** | $d > L_1 + L_2$ | Outside workspace |

### Step 5: Elbow-Up vs Elbow-Down
Two geometrically valid solutions exist for any non-singular point:
* **Elbow-Up:** $\theta_2 > 0$ (elbow bends upward, like a human arm)
* **Elbow-Down:** $\theta_2 < 0$ (elbow bends downward/behind)

The joystick button toggles between both solutions at runtime.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | IK computation |
| MG90S or SG90 Servo (or larger) | 2 | Shoulder + Elbow joints |
| Analog Joystick Module | 1 | Cartesian target input |
| 3D-printed / cardboard arm links | 2 | L1 and L2 physical linkages |
| External 5V / 2A power supply | 1 | Servos draw too much current for USB alone |

### Servo Alternatives

| Servo | Torque | Speed | Best For |
| :--- | :--- | :--- | :--- |
| **SG90** | 1.8 kg·cm | Fast | Light cardboard arm (our choice) |
| MG995 | 11 kg·cm | Medium | Acrylic/metal arm |
| Dynamixel AX-12 | 16.5 kg·cm | Controlled | Professional robotics |

---

## 🔌 Pin-to-Pin Wiring

| Component | Arduino Pin | Description |
| :--- | :--- | :--- |
| **Joystick VRx** | **A0** | X-axis (Cartesian X control) |
| **Joystick VRy** | **A1** | Y-axis (Cartesian Y control) |
| **Joystick SW** | **D2** | Button (elbow-up/down toggle) |
| **Shoulder Servo** | **D9** | Servo PWM signal |
| **Elbow Servo** | **D10** | Servo PWM signal |
| **Servo Power** | **External 5V** | Do NOT power both servos from Arduino 5V pin |

---

## 💻 How to Test & Validate

1. Set `L1` and `L2` in the code to match your physical arm link lengths in mm.
2. Upload [Day_61_Inverse_Kinematics.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_61_Inverse_Kinematics/Day_61_Inverse_Kinematics.ino).
3. Open **Serial Monitor** at **9600 Baud**.
4. Move the joystick in any direction. The serial monitor shows `Target=(x,y)` and computed `Theta1` and `Theta2` angles.
5. Observe the arm tip moving in roughly straight lines as you move the joystick — this is the IK working.
6. Move the joystick to the extreme corner. The serial monitor shows `[CLAMPED - Out of workspace]` when the target is beyond reach.
7. Press the joystick button to toggle elbow-up/elbow-down. The arm should flip to the mirror configuration.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Servos jitter violently at startup | Power supply underrated | Use dedicated 5V 2A supply for servos |
| `NaN` in serial output | acos receiving value outside [-1, 1] | The `constrain(cos_theta2, -1, 1)` line prevents this — if NaN occurs, the link lengths may be wrong |
| Arm tip doesn't reach corners | `L1`/`L2` constants don't match physical links | Measure links in mm and update code constants |
| Elbow moves in wrong direction | Servo horn installed backwards | Physically reverse the servo horn, or swap `theta2_deg` sign in code |

## 🧠 Code Explanation

Let's break down how we use Inverse Kinematics (IK) to control a robotic arm:

### 1. The Math: Law of Cosines
```cpp
float cos_theta2 = (d_sq - L1 * L1 - L2 * L2) / (2.0f * L1 * L2);
float theta2_rad = acos(cos_theta2);
```
- In Forward Kinematics, we know the joint angles and want to find the hand position. Inverse Kinematics is the opposite: we know the hand position (from the joystick) and must calculate the required joint angles!
- We use the Law of Cosines on the triangle formed by the upper arm (`L1`), the forearm (`L2`), and the straight-line distance (`d`) from the shoulder to the target.
- By isolating `theta2`, we discover exactly how much the elbow needs to bend to place the wrist at the correct distance from the shoulder.

### 2. Workspace Constraints
```cpp
float d_min = fabsf(L1 - L2) + 0.01f;
float d_max = L1 + L2 - 0.01f;
if (d < d_min) d = d_min;
if (d > d_max) d = d_max;
```
- A physical robot arm can't reach everywhere. If you push the joystick too far away, it can't stretch longer than `L1 + L2` (fully extended). If you pull it too close, it can't fold tighter than `L1 - L2`.
- Attempting to calculate angles outside this "donut-shaped" workspace results in imaginary numbers (mathematical singularities), which crash the Arduino.
- We "clamp" the target distance `d` so the arm simply stretches as far as it can and stops smoothly at its physical boundary.

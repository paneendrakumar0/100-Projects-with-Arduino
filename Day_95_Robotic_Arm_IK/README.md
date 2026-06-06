# Day 95: 3-DOF Robotic Arm Inverse Kinematics (Trigonometric & Geometric Solver)

Welcome to Day 95! Today we dive into the mathematics of robotic manipulation: **Inverse Kinematics (IK)**. We will build a geometric coordinate solver from scratch for a **3-Degree-of-Freedom (3-DOF) Articulated Robotic Arm**. We will study coordinate projections, solve joint angles using the **Law of Cosines**, filter out unreachable target singularities, and write smooth servo controller outputs.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Robotic_Arm.jpg" alt="Robotic Arm" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Servo_Motor.jpg" alt="Servo Motor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

In robotics, we describe a manipulator in two ways:
1. **Joint Space**: The angles of each motor (e.g. Base $= 45^\circ$, Shoulder $= 60^\circ$, Elbow $= 90^\circ$).
2. **Cartesian Space**: The position of the hand (end-effector) in 3D coordinates relative to the base (e.g. $X = 12.0\,\text{cm}$, $Y = 12.0\,\text{cm}$, $Z = 10.0\,\text{cm}$).

When programming a robot to draw, 3D print, or pick up an object:
- We know where the object is in **Cartesian Space** (from a camera or coordinate map).
- We cannot send Cartesian coordinates directly to standard servo motors; they only understand joint angles.
- **Inverse Kinematics (IK)** is the mathematical engine that converts the target $(X, Y, Z)$ coordinates into the required joint angles $(\theta_1, \theta_2, \theta_3)$ so the arm can reach that point.

---

## 🔬 Physics & Mathematics of Inverse Kinematics

Our articulated arm consists of:
- **Base Yaw ($\theta_1$)**: Rotates in the horizontal plane.
- **Shoulder Pitch ($\theta_2$)**: Controls vertical incline.
- **Elbow Pitch ($\theta_3$)**: Controls the angle of link 2.
- **Link Lengths**: $L_1$ (lower arm) and $L_2$ (upper arm).
- **Base Height**: $H_{\text{base}}$ (distance from ground to shoulder joint).

```
          [Elbow Joint]
             o
            / \
           /   \  L2 (Link 2)
  L1 (Link/     \
    1)   /       \
        /         \
 [Shoulder]        o [End-Effector] (X, Y, Z)
      o
      │
      │ H_base
      │
 [Base Rotation] (Theta 1)
```

### 1. Solving Base Rotation ($\theta_1$)
To align the arm with the target $(X, Y)$ coordinate in the horizontal plane, we look from the top down:
$$\theta_1 = \text{atan2}(Y, X)$$

### 2. Projecting to 2D Vertical Plane
We project the 3D target coordinates onto the vertical plane aligned with the arm. 
- Radial distance in the XY plane:
  $$r = \sqrt{X^2 + Y^2}$$
- Vertical height relative to the shoulder joint:
  $$Z_{\text{prime}} = Z - H_{\text{base}}$$

### 3. Solving Elbow Angle ($\theta_3$) via Law of Cosines
Let $S$ be the straight-line diagonal distance from the shoulder joint to the target:
$$S = \sqrt{r^2 + Z_{\text{prime}}^2}$$

Using the **Law of Cosines** on the triangle formed by $L_1$, $L_2$, and $S$:
$$S^2 = L_1^2 + L_2^2 - 2 \cdot L_1 \cdot L_2 \cdot \cos(\beta)$$

Where $\beta$ is the interior angle between $L_1$ and $L_2$. Rearranging for $\cos(\beta)$:
$$\cos(\beta) = \frac{L_1^2 + L_2^2 - S^2}{2 \cdot L_1 \cdot L_2}$$

For the "elbow-up" configuration, the joint angle $\theta_3$ is the supplementary angle:
$$\theta_3 = \pi - \beta$$

### 4. Solving Shoulder Angle ($\theta_2$)
The shoulder angle is the sum of two sub-angles:
$$\theta_2 = \phi_1 + \phi_2$$

Where:
- $\phi_1$ is the angle of the target line $S$ above the horizontal:
  $$\phi_1 = \text{atan2}(Z_{\text{prime}}, r)$$
- $\phi_2$ is the angle between link $L_1$ and line $S$, solved via the Law of Cosines:
  $$\cos(\phi_2) = \frac{L_1^2 + S^2 - L_2^2}{2 \cdot L_1 \cdot S}$$
  $$\phi_2 = \text{acos}(\cos(\phi_2))$$

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| 180-Degree Servo Motors | 3 | Base, Shoulder, and Elbow actuators |
| External Power Supply (5V/3A) | 1 | Powers servo motors (Uno USB cannot supply enough current) |
| Breadboard & Jumper Wires | 1 | Prototyping |

---

## 🔌 Pin-to-Pin Wiring

> [!IMPORTANT]
> **NEVER power multiple servos directly from the Arduino 5V pin!** Servos draw high current spikes that will reset the Arduino, or burn out its onboard 5V regulator. Always use an external 5V power supply and share the ground.

| Component Pin | Connect To | Description |
| :--- | :--- | :--- |
| **Base Servo Signal** (Orange/White) | **Digital Pin 9** | PWM control for Base |
| **Shoulder Servo Signal** | **Digital Pin 10** | PWM control for Shoulder |
| **Elbow Servo Signal** | **Digital Pin 11** | PWM control for Elbow |
| **Servos Power** (Red) | **External Power 5V (+)** | High-current power |
| **Servos Ground** (Black/Brown) | **External Power GND & Arduino GND** | Common Ground |
| **Arduino Power** | **PC USB / 9V Barrel Jack** | Logic power |

---

## 💾 Alternatives to Geometric IK Solvers

| Method | Joint Count (DOF) | CPU Overhead | Mathematical Difficulty | Singularity Safety | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Geometric Solver (Our code)** | **3 DOF maximum** | **Very Low** | **Medium** | **High (Direct Guards)** | Best for simple arms (3-DOF). Rapid execution. |
| **Denavit-Hartenberg (D-H)** | Any | Medium | High | Low | Standard matrix notation for systematic setups. |
| **Numerical Iterative (Jacobian)** | 4+ DOF | High | Very High | Low | Newton-Raphson approximation. Essential for 6-axis arms. |
| **FABRIK** (Heuristic) | Any | Low | Low | High | Forward And Backward Reaching Inverse Kinematics. Fast, but angles are less precise. |

---

## 💻 How to Test & Validate

1. Wire up the three servo motors to Pins 9, 10, and 11, using an external 5V power source.
2. Open the Arduino IDE, load [Day_95_Robotic_Arm_IK.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_95_Robotic_Arm_IK/Day_95_Robotic_Arm_IK.ino), and select the COM port.
3. Open the **Serial Monitor** at **9600 Baud**.
4. **Solve Cartesian Coordinates**:
   - Send `x 12` then `y 12` then `z 10` in the input bar.
   - Send `s` to execute the solver.
   - The monitor will calculate and display:
     - `Base (Servo D9)     : 135.0 degrees`
     - `Shoulder (Servo D10): 61.2 degrees`
     - `Elbow (Servo D11)   : 104.5 degrees`
   - The servos will smoothly move to position.
5. **Out-of-Reach Test (Singularity)**:
   - Send `x 25` then `y 25` then `z 20` (Target exceeds the arm's total length $L_1 + L_2 = 27\,\text{cm}$).
   - Send `s`.
   - The console will output: `[ERROR] Target out of range! (Exceeds physical reach)` and refuse to update the servos, protecting the hardware!
6. **Path Circle Trace (Serial Plotter)**:
   - Close the Serial Monitor and open **Tools > Serial Plotter** at **9600 Baud**.
   - Send `t` to toggle the path simulator.
   - You will see the joint angles and target positions plotted as smooth waves, tracing a circular orbit in 3D space!

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Servos twitch, jitter, or Arduino resets | Insufficient servo power | Ensure the servos are powered by an external 5V supply (minimum 2A) and that you have connected the external ground to the Arduino GND. |
| Arm moves to incorrect positions | Incorrect link length values or incorrect servo mounting offsets | Verify L1 and L2 match your arm's physical hinge-to-hinge lengths. If servos were mounted offset, adjust the offset degree constants in `solveInverseKinematics()`. |
| Target is reachable but solver errors | Angle exceeds servo limit | Standard hobby servos can only rotate 180 degrees. If the target coordinates require a joint to bend past its mechanical limit, the solver returns false to prevent binding. |

## 🧠 Code Explanation

Let's break down how the Geometric Inverse Kinematics solver works:

### 1. Base Yaw (XY Plane Projection)
```cpp
float theta1_rad = atan2(y, x);
```
- The first degree of freedom is the base rotating left and right. This is easily solved using `atan2()` to find the angle to the target coordinate on the 2D floor plane.

### 2. 2D Slice and Distance Calculation
```cpp
float r = sqrt(x*x + y*y);
float S = sqrt(r*r + z_prime*z_prime);
```
- We collapse the 3D problem into a 2D vertical slice. `r` is the horizontal distance from the base, and `z_prime` is the vertical height. 
- `S` calculates the direct diagonal distance from the shoulder joint to the target end-effector location using the Pythagorean theorem.

### 3. The Law of Cosines (Elbow & Shoulder)
```cpp
float cosBeta = (L1*L1 + L2*L2 - S*S) / (2 * L1 * L2);
float beta = acos(cosBeta);
```
- We now have a triangle formed by the upper arm (L1), the forearm (L2), and the diagonal distance (S).
- We apply the **Law of Cosines**, a fundamental trigonometric principle, to solve for the interior angles of this triangle!
- The angle `beta` directly dictates how far the elbow servo needs to bend to connect L1 to L2 at the exact target distance!

### 4. Workspace Singularities
- The solver checks if `S > (L1 + L2)`. If the target is physically further away than the two arm links stretched out straight, the code rejects the command. This prevents the mathematical `acos()` function from returning `NaN` (Not a Number) and crashing the robot!

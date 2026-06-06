# Day 99: Autonomous Path Tracking (The Pure Pursuit Algorithm)

Welcome to Day 99! Today we dive into the core steering logic of self-driving cars: the **Pure Pursuit Path Tracking Algorithm**. We will explore the geometry of look-ahead targets, derive the math of circular arc steering curvature, implement global-to-local coordinate frame transformations, and build a 20 Hz closed-loop path tracker.

---

## 🎯 The "Why" and "What"

If you command a robot to simply "Go to coordinate (X, Y)", it will turn towards the point, drive straight, and stop. However, in real-world navigation, robots must follow complex, curved paths (trajectories) to avoid obstacles, stay inside lanes, or sweep a room efficiently.

### What is Pure Pursuit?
**Pure Pursuit** is a geometric tracking method. The robot is modeled as a vehicle chasing a point on the path that is a fixed distance—the **Look-Ahead Distance ($L_d$)**—ahead of it. 
As the robot moves, it constantly calculates the steering angle (or differential wheel speeds) required to drive along a circular arc that passes exactly through this look-ahead point. 

```
                                            Look-Ahead Point (gx, gy)
                                                o
                                               / \
                                              /   \  L_d
                                     Arc     /     \
                                    . . . . o . . . .
                                  .         │         .
                                 .          │ ly       .
                                .           │           .
                               .            o ────────── .
                                            Robot (0,0) (Heading along X)
```

Pure Pursuit is highly robust: if the robot is bumped off course, it naturally calculates a new arc that steers it smoothly back onto the path, preventing sudden movements or crashes.

---

## 🔬 Physics & Mathematics of Pure Pursuit

### 1. Coordinate Frame Transformation
Waypoints are defined in the **Global Coordinate Frame** (relative to the room). However, steering calculations are much simpler in the **Robot's Local Frame** (where the robot is at $(0, 0)$ facing forward along the local $X$ axis).

Given the robot's position $(X_r, Y_r)$ and heading ($\theta_r$), we transform a global waypoint $(X_g, Y_g)$ to local coordinates $(x_l, y_l)$:
$$\Delta X = X_g - X_r$$
$$\Delta Y = Y_g - Y_r$$
$$x_l = \Delta X \cos(\theta_r) + \Delta Y \sin(\theta_r)$$
$$y_l = -\Delta X \sin(\theta_r) + \Delta Y \cos(\theta_r)$$

- $x_l$: Distance to target forward/backward.
- $y_l$: Distance to target left/right (lateral offset).

### 2. Deriving Steering Curvature ($\gamma$)
We want to steer along a circle of radius $R$ that starts at the robot and passes through the local target $(x_l, y_l)$. The center of this circle lies on the robot's lateral axis (at distance $R$ along $y$).
From the geometry of the circle:
$$x_l^2 + (y_l - R)^2 = R^2$$
$$x_l^2 + y_l^2 - 2 \cdot y_l \cdot R + R^2 = R^2$$

Since the chord length is the look-ahead distance ($L_d^2 = x_l^2 + y_l^2$):
$$L_d^2 - 2 \cdot y_l \cdot R = 0$$
$$R = \frac{L_d^2}{2 \cdot y_l}$$

The curvature $\gamma$ (defined as $1/R$) is:
$$\gamma = \frac{2 \cdot y_l}{L_d^2}$$

### 3. Differential Drive Mapping
For a differential drive robot with track width $W$, we map the curvature $\gamma$ to the left and right wheel velocities ($v_L, v_R$):
$$v_L = v_{\text{target}} \cdot \left(1 - \gamma \cdot \frac{W}{2}\right)$$
$$v_R = v_{\text{target}} \cdot \left(1 + \gamma \cdot \frac{W}{2}\right)$$

---

## 🔩 Components Needed

No external hardware components are required! The sketch runs a complete 2D kinematics simulation, updating coordinates at 20 Hz, which is fully plotted on the Arduino Serial Plotter.

To run on physical hardware:
- A differential drive robot chassis.
- Two DC motors + wheel encoders + motor driver (L298N).
- Arduino Uno.

---

## 🔌 Pin-to-Pin Wiring

- **No wiring required** for default simulation mode. Connect the Arduino Uno to your PC via USB.

---

## 💾 Alternatives to Pure Pursuit Path Tracking

| Method | Computational Cost | Trajectory Smoothness | Tuning Difficulty | Cross-Track Error | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Pure Pursuit** | **Low** | **Very High** | **Very Easy (Tuning Ld)** | Low | Industry baseline. Robust on rough terrain. |
| **Stanley Controller** | Low | High | Easy | Very Low | Standard for front-wheel steer vehicles (highway autopilot). |
| **Model Predictive (MPC)** | Extremely High | Optimal | Hard | Zero | Solves optimization matrix at every step. Requires SBC. |
| **PID Cross-Track Loop** | Very Low | Low | Medium | High | Tends to cut corners violently and oscillate. |

---

## 💻 How to Test & Validate

1. Open the Arduino IDE, load [Day_99_Pure_Pursuit_Path_Tracking.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_99_Pure_Pursuit_Path_Tracking/Day_99_Pure_Pursuit_Path_Tracking.ino).
2. Select your Arduino Uno board and upload the code.
3. Open **Tools > Serial Plotter** at **115200 Baud**. (Ensure the plotter speed matches!).
4. **Running the Simulation**:
   - Send `s` in the input bar. The robot starts moving.
   - You will see the coordinates $X$ and $Y$ trace out a smooth curve matching the target waypoints, showing the robot successfully converging on the path!
5. **Analyzing the Look-Ahead ($L_d$) Tuning**:
   - Close the plotter and open the **Serial Monitor** at **115200 Baud**.
   - Send `r` to reset coordinates.
   - Send `d` multiple times to reduce the Look-Ahead distance $L_d$ to $6\,\text{cm}$ (small look-ahead).
   - Send `s` and watch the curvature values. You will see high fluctuations, showing the robot is steering aggressively (wobbling).
   - Reset again (`r`), and send `l` multiple times to increase $L_d$ to $20\,\text{cm}$ (large look-ahead).
   - Send `s`. Notice how the steering curvature is very small and smooth, but the robot cuts the curves much wider.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Telemetry output displays garbage characters | Baud rate mismatch | Ensure the Serial Monitor / Serial Plotter baud rate is set to **115200**. |
| The robot steers away from the path | Incorrect lateral coordinate sign | Check the global-to-local transformation math. If $y_l$ calculation has the wrong sign, the robot will steer in the opposite direction of the path. |
| The robot cuts corners too heavily | Look-ahead distance $L_d$ is too large | Decrease $L_d$ (using `d` in the CLI) to make the robot track the segments more closely. |
| The robot oscillates violently left and right | Look-ahead distance $L_d$ is too small | Increase $L_d$ (using `l` in the CLI) to damp out the steering response. |

## 🧠 Code Explanation

Let's break down the industry standard path-tracking algorithm used by AGVs and self-driving cars:

### 1. Global to Local Frame Transformation
```cpp
float lx =  dx * cos(robotTheta) + dy * sin(robotTheta);
float ly = -dx * sin(robotTheta) + dy * cos(robotTheta);
```
- GPS or Odometry gives us the target waypoint in Global coordinates (e.g., X: 40, Y: 25).
- The robot doesn't know what to do with that. We use matrix rotation mathematics to transform the target into the Robot's *Local Coordinate Frame*. 
- `lx` is now "how far forward the target is", and `ly` is "how far to the left the target is" relative to the robot's hood!

### 2. The Look-Ahead Distance (Ld)
- A human driving a car doesn't stare at the bumper; they look 20 meters down the road.
- The algorithm searches the path array for the first waypoint that is further away than our designated "Look-Ahead Distance" (e.g., 12cm) and aims for that.

### 3. Calculating Steering Curvature (Gamma)
```cpp
float gamma = (2.0f * ly) / (Ld * Ld);
```
- The core of Pure Pursuit: we want to draw a perfect circular arc connecting the robot's rear axle to the Look-Ahead target.
- Using basic geometry, the curvature of that arc (`gamma`) is mathematically derived directly from the lateral offset (`ly`) and the look-ahead distance (`Ld`). 

### 4. Differential Drive Motor Mapping
```cpp
float vL = vTarget * (1.0f - (gamma * WHEEL_TRACK / 2.0f));
float vR = vTarget * (1.0f + (gamma * WHEEL_TRACK / 2.0f));
```
- Once we know the required curvature, we map it to the physical wheels.
- If the curve goes left (positive `gamma`), the math automatically slows down the left wheel (`vL`) and speeds up the right wheel (`vR`) perfectly, forcing the robot onto the circular arc intersecting the target!

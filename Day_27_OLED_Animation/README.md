# Day 27: OLED Bouncing Ball Animation (2D Physics & Frame Rate Control)

Welcome to Day 27 of the 100-Day Arduino Masterclass! Today, we progress from static vector drawing (Day 26) to a real-time **2D physics simulation** on our SSD1306 OLED display. We will learn how to write a simple game-loop engine featuring kinematic vector translation, wall collision detection, penetration resolution, and frame-rate gating.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/OLED_Display.jpg" alt="OLED Display" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Robotic_Arm.jpg" alt="Robotic Arm" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

In robotics and mechatronics, rendering animations or real-time simulation displays is crucial for human-machine interfaces (HMIs). For example, autonomous mobile robots (AMRs) might display scanning lasers (LiDAR sweeps), robotic arms might show live kinematic trajectories, and industrial controllers might display state machines.

Implementing an animation requires:
1. **Dynamic updates** to coordinates inside a loop.
2. **Deterministic timing** to ensure that movement looks identical regardless of how fast the main CPU is processing instructions.
3. **Collision boundaries** representing mechanical workspace limits or sensor thresholds.

Today's project simulates a bouncing ball in a closed boundary, serving as a blueprint for path planning, obstacle avoidance math, and custom animated UI screens.

---

## 🔬 Physics & Hardware Theory

To create smooth animation on a discrete pixel grid (128x64), we apply basic Newtonian kinematics and Cartesian coordinate mapping:

### 1. Vector Translation (Euler Integration)
A physical point in a 2D coordinate system is represented by its Position vector $\mathbf{P} = [x, y]^T$ and its Velocity vector $\mathbf{V} = [v_x, v_y]^T$. 
At discrete time intervals $\Delta t$, the position updates as follows:
$$x_{k+1} = x_k + v_x \cdot \Delta t$$
$$y_{k+1} = y_k + v_y \cdot \Delta t$$

In our code, we normalize $\Delta t = 1$ frame cycle. Therefore, at each frame, we update position simply by adding velocity components:
```cpp
ballX += ballVx;
ballY += ballVy;
```

### 2. Elastic Collision & Boundary Check
When a circular body of radius $R$ collides with a boundary, its velocity vector must reflect symmetrically. In physics, an **elastic collision** with an infinite-mass boundary (like a static wall) reverses the component of velocity parallel to the normal of the wall:
* **Vertical walls (left/right bounds):** The horizontal speed is negated: $v_x \leftarrow -v_x$.
* **Horizontal walls (top/bottom bounds):** The vertical speed is negated: $v_y \leftarrow -v_y$.

$$\text{Collision Check: } x - R \leq X_{\min} \quad \text{or} \quad x + R \geq X_{\max}$$

### 3. Penetration Resolution (Position Clamping)
Because time steps are discrete, the ball may bypass the boundary line slightly during a frame update (penetration). Simply reversing velocity is not enough: if the ball is still overlapping the wall in the next frame, it will trigger another collision, leading to the ball getting stuck and shaking back and forth on the wall (an infinite collision loop).
To prevent this, we execute **penetration resolution** by shifting the ball back to the exact point of contact:
```cpp
ballX = PLAY_X_MIN + ballRadius; // Shifting tangent to the left boundary
```

### 4. Frame Rate Gating (Determinism)
Without timing controls, the Arduino would compute physics and write to the display as fast as possible. The rendering speed would change depending on what else is running in the main loop, causing the animation to accelerate or slow down. 
We lock the simulation to a target **30 FPS (Frames Per Second)**. The loop calculates if at least $33.3\text{ ms}$ have passed before advancing the physics and redraw cycle:
$$\text{Frame Interval} = \frac{1000\text{ ms}}{30\text{ FPS}} \approx 33.3\text{ ms}$$

---

## 🔄 Alternatives Comparison

When selecting displays for real-time graphics and animation, developers must balance bus interface speed, controller RAM overhead, and power:

| Display Type | Controller | Interface | Color / Tech | Framerate / Performance | Memory Overhead | Best Used For |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **0.96" OLED** | **SSD1306** | **I2C ($400\text{ kHz}$)** | **Monochrome Self-lit** | **Medium ($\sim30$ FPS)** | **Low (1024 Bytes)** | **Compact diagnostics, vector animations, portable HMIs (Our choice)** |
| **0.96" OLED** | **SSD1306** | **SPI ($8\text{ MHz}$)** | **Monochrome Self-lit** | **High ($>60$ FPS)** | **Low (1024 Bytes)** | **High-speed UI animations, games** |
| **16x2 Character LCD** | **HD44780** | **Parallel / I2C** | **LCD (Backlit)** | **Extremely Low ($\sim5\text{Hz}$)** | **None** | **Static text readouts, simple status logging** |
| **1.8" TFT Color LCD** | **ST7735** | **SPI** | **18-bit Color TFT** | **Medium-High** | **High (20k+ Bytes)** | **Color menus, data plotting, complex graphics** |

---

## 🛠️ Components Needed

* 1x Arduino Uno (or Mega/Nano)
* 1x 0.96" I2C SSD1306 OLED Display (128x64 pixels)
* 1x Breadboard
* 4x M-F jumper wires

---

## 🔌 Pin-to-Pin Wiring

| OLED Module Pin | Arduino Uno Pin | Wire Color (Recommended) | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **5V** or **3.3V** (Verify module spec) | Red | Power Input |
| **GND** | **GND** | Black | Ground |
| **SDA** | **A4** (or dedicated SDA pin) | Green | I2C Serial Data |
| **SCL** | **A5** (or dedicated SCL pin) | Yellow | I2C Serial Clock |

> [!IMPORTANT]
> Some cheap 0.96" OLED modules run strictly on 3.3V, while others have onboard regulators and are 5V-tolerant. Check the markings on the back of your module before connecting VCC to avoid burning out the driver IC.

---

## 💻 How to Test & Validate

1. Connect the hardware according to the wiring table.
2. Install the **Adafruit SSD1306** and **Adafruit GFX Library** from the Arduino IDE Library Manager.
3. Open `Day_27_OLED_Animation.ino` in the Arduino IDE.
4. Select your Board (Arduino Uno) and COM Port, then click **Upload**.
5. Observe the OLED display:
   * A boundary box will be drawn.
   * A circular ball will bounce smoothly off all sides of the box.
   * A top dashboard header will display the live actual frame rate (FPS) and coordinates ($X$ and $Y$).
6. Open the **Serial Monitor** at **9600 Baud** to inspect real-time position vectors and performance logs.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The display is completely black:**
  * Double check the I2C wiring (SDA to A4, SCL to A5).
  * The default I2C address is `0x3C`. Some modules use `0x3D`. Try changing `#define SCREEN_ADDRESS 0x3D` in the code.
  * Run an I2C scanner sketch (Day 21) to confirm the address.
* **The ball gets stuck at the boundaries or behaves erratically:**
  * Verify that the radius and boundary values in `updatePhysics()` are correct.
  * Make sure penetration resolution code is not commented out. Without clamping, the ball will rapidly flip velocity signs.
* **The frame rate (FPS) is extremely low (below 10):**
  * Ensure there are no active `delay()` statements in the main loop or graphics functions.
  * Verify your Arduino IDE serial monitor baud rate is set to 9600 (printing too fast over slower serial streams can occasionally back up the system).

## 🧠 Code Explanation

Let's break down how we programmed a 2D physics engine for the OLED display:

### 1. Vector Kinematics
```cpp
ballX += ballVx;
ballY += ballVy;
```
- Every frame (30 times a second), the ball's position (`ballX`, `ballY`) is updated by adding its Velocity Vectors (`ballVx`, `ballVy`). This makes the ball fly smoothly across the screen diagonally!

### 2. Elastic Collision & Penetration Resolution
```cpp
if (ballX - ballRadius <= PLAY_X_MIN) {
    ballVx = -ballVx;                 // Reverse direction vector
    ballX = PLAY_X_MIN + ballRadius;  // Penetration resolution
}
```
- We check if the edge of the ball (`ballX - ballRadius`) has hit the left wall.
- If it has, we simply multiply the X-velocity by `-1`. If it was moving Left at `2px/frame`, it is now moving Right at `2px/frame`! A perfect bounce.
- **Pro Tip:** We also manually snap `ballX` exactly against the wall. Because our simulation moves in "chunks" of pixels, the ball might have accidentally sunken *into* the wall during the frame. Snapping it back prevents the ball from getting permanently stuck inside the wall boundary!

# Day 13: Joystick Module Value Mapper

Welcome to Day 13 of the 100-Day Arduino Masterclass! Today, we explore multi-dimensional input control. We will learn how to interface a dual-axis analog joystick module (equipped with an integrated select pushbutton) and translate its raw outputs into calibrated Cartesian coordinates.

You will master sensor calibration math, learn how to handle center spring tolerances using software filters, and map signals into coordinate formats suitable for driving robotic vehicles.

---

## 🎯 Today's Learning Goals
1. Understand the electrical layout of dual-axis potentiometric joysticks.
2. Master the physics of mechanical spring tolerances and center drift.
3. Write an auto-calibration boot routine to zero out sensor offsets.
4. Implement a two-dimensional deadband filter to eliminate signal drift at rest.
5. Map raw 10-bit analog signals to Cartesian coordinate frames ($-100\text{ to }+100$).

---

## 🧠 The "Why" and "What": Joysticks in Robotics

### What is a Joystick Module?
A standard analog joystick module (such as the PS2 gamepad joystick) is a two-dimensional input device that measures mechanical movement along the X (horizontal) and Y (vertical) axes. It combines two rotary potentiometers mounted at right angles (orthogonal) to each other, plus a momentary tactile switch that triggers when the joystick shaft is pressed downward.

### Why is it Used in Robotics & Mechatronics?
In robotics, dual-axis inputs are essential for manual vehicle piloting and joint control:
- **Differential Mobile Drive (Rover Control):** Pushing the stick forward/backward controls speed (Y-axis), while pushing it left/right controls steering/rotation rate (X-axis).
- **Pan-Tilt Gimbal Systems:** Steering camera angles (pitch and yaw) simultaneously with a single thumb.
- **Robotic Arm Steering:** Driving the tool-center point of a robotic claw in 2D space (e.g., forward-back, left-right).
- **Industrial Remote Controls:** Operating cranes, excavators, and heavy machinery with tactile precision.

---

## ⚡ The Physics & Hardware Theory

### 1. Orthogonal Potentiometer Layout
Inside the joystick assembly, a plastic gimbal pivot mechanical structure is connected to two independent potentiometers:
* **X-Axis Potentiometer:** Measures horizontal rotation.
* **Y-Axis Potentiometer:** Measures vertical rotation.

```
       Joystick Internals (Gimbal Mechanism)
       
                     [Y-Axis Pot]  (Vertical)
                          |
                          v
                   +--------------+
                   |  Pivot Shaft |
                   +--------------+
                          ^
                          |
      [X-Axis Pot] ➡️ [Gimbal Cup]
      (Horizontal)
```

When you move the thumbstick, it rotates the pivot shaft. The gimbal cup translates this 2D diagonal motion into separate rotation components, adjusting the wiper position of both potentiometers simultaneously.

### 2. Spring Tolerances and Center Drift
A mechanical spring returns the joystick to its center position when released. In an ideal world, this center resting position would output exactly $2.5\text{V}$, translating to an ADC value of `512`.

However, due to manufacturing tolerances:
* The spring might not return the wiper to the exact center.
* The resistive track inside the pots is not perfectly linear.
* The center resistance drifts over time as the switch is used and carbon tracks wear.

A resting joystick will typically read values like `505` or `522` instead of `512`. If you map this directly to motor control speeds, a resting rover will drift or creep slowly in one direction.

### 3. Auto-Calibration and Deadband Math
To solve center drift, we implement two software solutions:
1. **Auto-Calibration:** During the first second of startup, the Arduino averages 10 readings from the resting joystick to measure the actual resting values ($centerX$ and $centerY$).
2. **Subtraction Offset:** We subtract this center offset from subsequent readings:
   $$\text{calX} = \text{rawX} - \text{centerX}$$
3. **Deadband Filter:** We establish a deadband threshold (e.g., $15$ units). If the calibrated value falls inside this threshold ($-\text{DEADBAND} \le \text{cal} \le \text{DEADBAND}$), we force it to zero.

```
              Joystick ADC Readings Scale (0 - 1023)
              
   0 ___________________[--- Deadband Range ---]___________________ 1023
   |                     -15     0    +15                          |
   (Left/Down)                 (Center)                     (Right/Up)
                        Reading forced to 0
```

### 4. Cartesian Mapping
Once we confirm the stick is pushed past the deadband, we map the values:
- Left / Down motions are mapped from $0\text{ to }center$ ➡️ $-100\text{ to }0$.
- Right / Up motions are mapped from $center\text{ to }1023$ ➡️ $0\text{ to }+100$.

---

## 🔄 Alternatives: Joysticks vs. Trackballs vs. Touch pads

| Input Device | Technology | Degrees of Freedom | Durability | Noise Vulnerability | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Potentiometric Joystick** | Dual analog variable resistors. | 2 Axis + 1 Button | Moderate (mechanical carbon tracks wear). | Moderate (analog lines). | **Chosen** for budget RC controllers, rover navigation, and joint steering. |
| **Hall Effect Joystick** | Magnets and Hall-effect magnetic field sensors. | 2 Axis + 1 Button | Extremely High (contactless, zero physical wear). | Zero | Professional aviation controllers, heavy industrial equipment, high-end gamepads. |
| **Trackball** | Optical scanning of a rotating ball. | 2 Axis | High | Low | Precision mouse interfaces, marine console navigation. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **Dual-Axis Joystick Module** (KY-023 or standard breakout).
3. **Breadboard & Jumper Wires**.
4. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

| Joystick Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **+5V** / VCC | **5V** | Red | Power supply (+5V) |
| **GND** | **GND** | Black | Ground reference |
| **VRx** / X-Out | **A0** | Yellow | X-axis analog output |
| **VRy** / Y-Out | **A1** | Green | Y-axis analog output |
| **SW** / Key | **Pin 2** | Blue | Select button switch (active low) |

---

## 🧪 How to Test and Validate

Follow these steps to upload, calibrate, and verify the joystick readings:

### 1. Calibration Preparation
- Place your Arduino and breadboard on a flat surface.
- **Do not touch the joystick** when connecting the USB cable or uploading the code. The resting positions must be undisturbed to calibrate correctly.

### 2. Startup Verification
- Upload `Day_13_Joystick_Mapper.ino`.
- Open the Serial Monitor (**Tools > Serial Monitor**) at **9600 Baud**.
- Verify the auto-calibration outputs:
  ```text
  DO NOT TOUCH JOYSTICK. Calibrating center point...
  Calibration Complete. Center X: 508 | Center Y: 515
  System armed. Move the stick or click the button.
  ```

### 3. Coordinate Sweep Testing
- **Test Center Deadband:** Leave the stick untouched. The coordinate telemetry logs should remain quiet. No coordinates should print, confirming the deadband filter works.
- **Test X-Axis (Horizontal):**
  - Push the stick all the way to the **left**: Mapped coordinates should read: `X = -100 | Y = 0`.
  - Push all the way to the **right**: Coordinates should read: `X = 100 | Y = 0`.
- **Test Y-Axis (Vertical):**
  - Push the stick all the way **UP**: Coordinates should read: `X = 0 | Y = 100`.
  - Push all the way **DOWN**: Coordinates should read: `X = 0 | Y = -100`.
- **Test Diagonal Combinations:** Push stick diagonally top-right. Mapped values should read positive values for both axes, e.g., `X = 72 | Y = 68`.

### 4. Button Testing
- Press down on the joystick shaft.
- You should hear a mechanical click, and the monitor should immediately log:
  ```text
  [SW] Joystick Button Pressed!
  ```
- Release it, and it should log:
  ```text
  [SW] Joystick Button Released.
  ```

### 🔍 Troubleshooting Tips
* **The coordinates change rapidly even when I don't touch the joystick:**
  - The `DEADBAND` threshold is set too low for your joystick. In the code, change `const int DEADBAND = 15;` to `20` or `25`.
* **The X or Y axes are swapped (pushing up increases X instead of Y):**
  - Swap your wiring connections at the Arduino: connect the VRx pin to A1 and the VRy pin to A0, or change the pin constants in code.
* **The button prints "Pressed" and "Released" dozens of times per click:**
  - Your button switch wiring is loose, or the debounce timer is failing. Check that Pin 2 is firmly plugged in.

## 🧠 Code Explanation

Let's break down how to read a joystick and map its coordinates:

### 1. Boot Auto-Calibration
```cpp
long sumX = 0;
long sumY = 0;
for (int i = 0; i < 10; i++) {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
}
centerX = sumX / 10;
```
- In a perfect world, a resting joystick outputs exactly `512`. In reality, cheap potentiometers usually rest around `490` or `530`. 
- When the Arduino boots, we take 10 rapid measurements and average them to find the true "center". We subtract this offset from all future readings.

### 2. Custom Dual-Zone Mapping
```cpp
if (calX < 0) {
    mappedX = map(rawX, 0, centerX, -100, 0);
} else if (calX > 0) {
    mappedX = map(rawX, centerX, 1023, 0, 100);
}
```
- To make a useful controller (like for a drone or robot car), we want values from `-100` (full reverse) to `100` (full forward), with exactly `0` in the center.
- We map the left half `[0 to centerX]` to `[-100 to 0]`, and the right half `[centerX to 1023]` to `[0 to 100]`.

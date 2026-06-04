# Day 50: Digital Compass Heading Lock (Magnetometer Navigation)

Welcome to Day 50 of the 100-Day Arduino Masterclass! Today, we reach the halfway point of the masterclass by building an absolute navigation system utilizing a **QMC5883L 3-Axis Digital Magnetometer (Compass)**. We will configure the module over direct I2C, write an automated **Hard-Iron bias calibration routine**, account for local geographic declination, and implement a **closed-loop heading-lock steering controller** to drive a robot chassis along a precise compass bearing.

---

## 🎯 The "Why" and "What"

Robots navigating with encoders or IMU gyroscopes drift over time.
* **The Problem:** Gyroscopes drift continuously, and wheels slip on physical floor surfaces. Without an absolute global reference, a robot cannot travel in a straight line or head towards a specific cardinal direction.
* **The Solution:** We utilize the Earth's magnetic field. By reading a magnetometer, the robot behaves like a real compass. However, metal chassis parts, batteries, and active motor coils create strong local magnetic fields that distort the sensor. We must calibrate the sensor dynamically to filter out these "Hard-Iron" distortions and write a closed-loop controller that adjusts wheel speeds to lock onto a target compass heading.

---

## 🔬 Physics & Mathematics

### 1. Earth's Magnetic Vector Physics
The Earth's magnetic field is a weak vector field (ranging from $25$ to $65\,\mu\text{T}$ or $0.25 - 0.65\,\text{Gauss}$). A magnetometer detects these magnetic vectors along three axes ($X, Y, Z$). When held horizontally flat, the heading angle ($\theta$) is calculated relative to magnetic North using trigonometry:
$$\theta_{\text{magnetic}} = \text{atan2}(Y_{\text{calibrated}}, X_{\text{calibrated}})$$

---

### 2. Geographic Declination Correction
The Magnetic North Pole does not align perfectly with the geographic True North Pole. This angular offset is called **Magnetic Declination** and changes based on your position on Earth.
To find your local declination, consult [magnetic-declination.com](http://www.magnetic-declination.com/).

$$\theta_{\text{true}} = \theta_{\text{magnetic}} + \text{Declination Angle}$$

If the declination is East, it is positive. If West, it is negative.
* Example: London has a declination of $+1.28^\circ \rightarrow +0.022\,\text{radians}$.
* Tokyo has a declination of $-7.9^\circ \rightarrow -0.138\,\text{radians}$.

---

### 3. Hard-Iron Bias Correction
Ferrous materials on the robot frame shift the magnetometer readings by a static offset. 
To remove this, we sweep the robot in a full circle ($360^\circ$), recording the maximum and minimum raw values on the $X$ and $Y$ axes. The offsets are calculated as the center point of the resulting circle:
$$X_{\text{offset}} = \frac{X_{\text{max}} + X_{\text{min}}}{2}, \quad Y_{\text{offset}} = \frac{Y_{\text{max}} + Y_{\text{min}}}{2}$$

During operation, we subtract these offsets:
$$X_{\text{calibrated}} = X_{\text{raw}} - X_{\text{offset}}, \quad Y_{\text{calibrated}} = Y_{\text{raw}} - Y_{\text{offset}}$$

```
       Raw Circle Offset                     Calibrated Centered Circle
              Y                                         Y
              |   * * *                                 |   * * *
              | *       *                               | *       *
         x- - - - - - - *                  - - - - - - -x- - - - - - -
              | *       *                               | *       *
              |   * * *                                 |   * * *
              +------------ X                           +------------ X
             (Shifted)                               (Centered at Origin)
```

---

### 4. Heading Error & Shortest Path Navigation
When steering, if the target is $350^\circ$ and the current heading is $10^\circ$, a simple subtraction yields an error of $340^\circ$, causing the robot to execute a wide turn in the wrong direction.
We normalize the error to the shortest angular path within $[-180^\circ, 180^\circ]$:
$$\text{Error} = \text{Target} - \text{Current}$$
$$\text{If Error} > 180^\circ \Rightarrow \text{Error} = \text{Error} - 360^\circ$$
$$\text{If Error} < -180^\circ \Rightarrow \text{Error} = \text{Error} + 360^\circ$$

---

## 🔄 Magnetometer Comparison

| Sensor Chip | I2C Address | Calibration Dependency | Sensor Resolution | Best Used For |
| :--- | :--- | :--- | :--- | :--- |
| **QMC5883L (GY-271)** | **`0x0D`** | **Mandatory (Fast setup)** | **16-bit ADC (Low Noise) (Our choice)** | **Affordable robotic heading control** |
| **HMC5883L** | `0x1E` | Mandatory | 12-bit ADC | Legacy DIY navigation systems |
| **BNO055** | `0x28` | Onboard automatic | 16-bit (with ARM Cortex M0 fusion) | High-precision drones, advanced ROS robots |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x GY-271 Module (featuring the QMC5883L chip)
* 1x 2WD Robot Chassis (2 DC motors + wheels + caster)
* 1x L298N Dual H-Bridge Driver Module
* 1x Battery Pack (e.g. 2s LiPo or 6x AA battery holder to power motors)
* 1x Breadboard & Jumper wires

---

## 🔌 Pin-to-Pin Wiring

### 1. QMC5883L to Arduino Uno (I2C)
| Magnetometer Pin | Arduino Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **5V** (or 3.3V) | Red | Logic Power |
| **GND** | **GND** | Black | Ground |
| **SDA** | **A4** | Blue | Serial Data |
| **SCL** | **A5** | Yellow | Serial Clock |

### 2. L298N Driver to Arduino Uno & Power
| L298N Driver Pin | Arduino Pin / Battery | Description |
| :--- | :--- | :--- |
| **ENA** | **D5** (PWM) | Left Motor Speed |
| **IN1 / IN2** | **D4 / D3** | Left Motor Direction |
| **ENB** | **D6** (PWM) | Right Motor Speed |
| **IN3 / IN4** | **D7 / D8** | Right Motor Direction |
| **12V** | **Battery positive (+)** | High-voltage motor power |
| **GND** | **GND (Arduino & Battery -)** | Shared logic/power Ground |

---

## 💻 How to Test & Validate

1. **Physical Assembly (Sensor Elevation)**:
   * > [!CAUTION]
     > Do **NOT** mount the magnetometer directly on top of the L298N motor driver or the battery pack. The high currents running to the motors generate large magnetic fields that overwhelm the Earth's field. Mount the sensor on a stand or tower **$10\text{cm} - 15\text{cm}$** above the chassis.
2. **Configure Declination**:
   * Look up your geographic location declination and update the `LOCAL_DECLINATION_RAD` in [Day_50_Compass_Heading_Lock.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_50_Compass_Heading_Lock/Day_50_Compass_Heading_Lock.ino).
3. **Upload & Calibrate**:
   * Upload the code.
   * On boot, the onboard Pin 13 LED lights up. Place the robot flat on the floor; it will automatically pivot in place for 6 seconds to record the X/Y magnetic limits.
   * When finished, the LED turns off, the calculated offsets are logged to the Serial Monitor, and heading tracking starts.
4. **Heading Lock Testing**:
   * Set your target direction (e.g. `targetHeading = 90.0` for East) in code.
   * Open the **Serial Plotter** at **9600 Baud**.
   * Pick up the robot and rotate it manually. You will see the motor speeds adjust:
     - If you point the robot North ($0^\circ$), it will spin the motors to steer Left/Right back towards East ($90^\circ$).
     - Once aligned at $90^\circ$, the motor correction drops to zero, and the wheels drive forward symmetrically.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The robot oscillates wildly back and forth around the target direction**:
  * The proportional gain $K_p$ is set too high. Reduce $K_p$ in steps of 0.5 until the robot approaches the target direction smoothly.
* **When the motors start spinning, the heading reading jumps erratically**:
  * Magnetic interference from the motor coils is affecting the sensor. Move the magnetometer further away from the motors, battery wires, and chassis frames. Use twisted-pair jumpers for the motors to cancel out EMI.
* **The serial monitor prints "QMC5883L communication failed!"**:
  * Double check your I2C connections on pins A4/A5. Ensure you have the correct pull-up resistors if your module does not have them built-in.
  * Check if your sensor chip is actually a QMC5883L (common on modern GY-271 boards) or an original HMC5883L (different register maps and I2C address `0x1E`).

# Day 100: Micro-ROS / Serial ROS Bridge (The Capstone Integration Node)

Welcome to Day 100! Today we build the **Grand Capstone Project** of our 100-day Arduino Masterclass: a **Micro-ROS Serial Bridge Node**. We will explore how modern autonomous robots bridge low-level microcontrollers with high-level robotic brains running ROS (Robot Operating System) over UART. We will design a custom subscriber for Twist velocity commands (`/cmd_vel`) and a publisher for coordinate odometry data (`/odom`), wrapped in a robust, checksum-verified binary frame.

---

## 🎯 The "Why" and "What"

In professional robotics (such as self-driving cars, industrial manipulators, or autonomous drones), we do not run path planning, computer vision, or mapping directly on an Arduino. 8-bit microcontrollers lack the processing power and RAM for these tasks. 

Instead, high-level code runs on a powerful Single Board Computer (SBC) like a Raspberry Pi, NVIDIA Jetson, or PC running **ROS (Robot Operating System)**. 

However, high-level computers cannot generate precise real-time motor pulses, read raw quadrature encoder ticks, or handle microsecond interrupts. 
- The **ROS PC** acts as the **Brain** (computes *what* the robot should do).
- The **Arduino** acts as the **Spinal Cord** (coordinates motor outputs and counts raw wheel ticks).

To connect them, we establish a **Serial Bridge Node** that translates structured ROS topic commands into motor actions and reports sensor odometry states back to the PC.

---

## 🔬 Physics & Mathematics of the Serial Bridge

```
┌──────────────────────────┐                  ┌───────────────────────────┐
│       ROS HOST PC        │                  │     ARDUINO EMBEDDED      │
│  [Navigation Stack]      │                  │  [FSM Parser]             │
│   └── publishes /cmd_vel │ ──Twist Packet──►│   ├── decodes velocities  │
│                          │                  │   └── drives motors       │
│  [SLAM / Coordinate Map] │                  │  [Odometry Solver]        │
│   ▲── subscribes /odom   │ ◄── Odom Packet──│   └── integrates encoders │
└──────────────────────────┘                  └───────────────────────────┘
```

### 1. Subscribing to `/cmd_vel`
In ROS, the `/cmd_vel` topic uses a `geometry_msgs/Twist` message type:
- **Linear X ($v_x$ in m/s)**: Desired speed forward/backward.
- **Angular Z ($\omega_z$ in rad/s)**: Desired rotational speed (yaw rate).

To translate this Twist message into left and right wheel RPM targets, the Arduino executes **Inverse Differential Kinematics**:
$$v_L = v_x - \omega_z \cdot \frac{W}{2}$$
$$v_R = v_x + \omega_z \cdot \frac{W}{2}$$
where $W$ is the wheel track width ($0.15\,\text{m}$ in our setup).

To convert m/s wheel speeds ($v$) to actuator Rotations Per Minute ($\text{RPM}$):
$$\text{RPM} = \frac{v}{\pi \cdot D_{\text{wheel}}} \cdot 60$$

### 2. Publishing `/odom`
Every 100 milliseconds (10 Hz), the Arduino calculates its accumulated coordinate state $(X, Y, \theta)$ from wheel speeds (or encoder ticks) and packages the data into a binary frame to publish back to `/odom`.

### 3. Binary Value Scaling
AVR microcontrollers have limited serial bandwidth, and float variables are 4 bytes. To minimize packet size:
- We multiply floats by scaling factors (e.g. $1000$ for velocities, $100$ for coordinates) and pack them into **16-bit signed integers** (2 bytes).
- A linear velocity of $0.25\,\text{m/s}$ scales to $250$.
- In hex representation, $250$ is `0x00FA` (2 bytes), which we send over the wire as `0x00` and `0xFA`.
- The receiver divides by $1000$ to restore the float. This saves $50\%$ of the serial data bandwidth!

---

## 🔩 Components Needed

No external components required! The sketch contains an integrated ROS PC master simulator. Connect your Arduino Uno to the PC using a USB cable to execute communication tests.

---

## 🔌 Pin-to-Pin Wiring

- **No wiring required** for default simulation mode. Connect the Arduino Uno to your PC via USB.

---

## 💾 Alternatives to Custom Serial Bridges

| Method | Bandwidth Efficiency | MCU RAM Overhead | Configuration Difficulty | Real-Time Safety | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Custom Binary Bridge (Our project)** | **Extremely High** | **Very Low (< 100 bytes)** | **Low** | **High (Non-blocking FSM)** | Self-contained, fits on 2KB RAM Uno easily. |
| **rosserial_arduino** | Medium | High (~1.2KB RAM) | Medium | Moderate | Older XML-RPC based bridge. Prone to synchronization dropouts. |
| **micro-ROS** | Low | Extremely High | High | Very High | Uses native Agent/Client DDS middleware. Requires 32-bit MCU (ESP32/Teensy). |
| **ASCII Command Line** | Poor | Low | Very Easy | Low | High latency, difficult to parse without blocking. |

---

## 💻 How to Test & Validate

1. Open the Arduino IDE, load [Day_100_ROS_Serial_Bridge.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_100_ROS_Serial_Bridge/Day_100_ROS_Serial_Bridge.ino), and select the COM port.
2. Select your Arduino Uno board and upload the code.
3. Open the **Serial Monitor** at **115200 Baud**. (Ensure "No line ending" is selected in the bottom dropdown).
4. **Observe the `/odom` Publisher**:
   - The Arduino immediately starts transmitting binary `/odom` packets at 10 Hz:
     `[ROS PUBLISH /odom] Binary Packet: 02 07 20 00 00 00 00 00 00 20 03  | Decoded -> X:0.00m, Y:0.00m, Yaw:0.0deg`
   - Observe the components: `02` (SOF), `07` (Len), `20` (MsgType), six payload bytes (X, Y, Theta), checksum, and `03` (EOF).
5. **Simulate a ROS `/cmd_vel` Command**:
   - Send `v 0.30 -0.50` in the input bar (simulates ROS publishing a Twist velocity command: linear speed $0.30\,\text{m/s}$ and angular speed $-0.50\,\text{rad/s}$).
   - The simulation generates and parses the binary packet.
   - The Arduino logs the decodes and outputs the calculated wheel targets:
     `Decoded Msg -> Linear X: 0.300 m/s | Angular Z: -0.500 rad/s`
     `Actuator Target -> Left Wheel: 108.5 RPM | Right Wheel: 65.1 RPM`
   - Look at the `/odom` publisher logs. The coordinates X and Y will start incrementing and Yaw will drift to the right, showing the robot is successfully simulating movement in 2D space!
6. **Stop/Resume Publishing**:
   - Send `s` to pause the `/odom` publisher loop. Send `s` again to resume.
   - Send `r` to reset the coordinate tracking system back to `(0, 0, 0)`.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Telemetry output displays garbage characters | Baud rate mismatch | Ensure the Serial Monitor baud rate is set to **115200**. |
| `[BRIDGE ERROR] Checksum mismatch` | Serial bit flips or noise | If running close to high-power DC motors, shield the USB cables. Check the FSM checksum generation on the host PC side. |
| Memory usage is too high on compile | Excessive string literals | All debugging string prints in this project are wrapped in the `F()` macro (e.g. `F("...")`) which stores them in flash memory, saving RAM for the FSM buffers. |
| Publisher does not start | Publisher paused | Send `s` in the Serial Monitor input bar to resume the 10 Hz automatic timer loop. |

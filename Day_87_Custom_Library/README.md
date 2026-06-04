# Day 87: Creating a Custom Arduino Library (Exponential Moving Average Filter)

Welcome to Day 87! Today we learn how to decouple our code and write a professional, reusable **Custom C++ Library** for the Arduino IDE environment. As our practical example, we will implement an **Exponential Moving Average (EMA) Filter** library, which is the industry-standard algorithm for smoothing noisy sensor readings on resource-constrained microcontrollers.

---

## 🎯 The "Why" and "What"

In large embedded systems, keeping all code in a single `.ino` file leads to unmanageable code (spaghetti code) that is difficult to test, debug, and reuse across projects. 

### Why Write Custom Libraries?
1. **Encapsulation & Modularity**: Grouping related variables and functions into a C++ class hides internal mechanics and presents a clean, simple API to the main sketch.
2. **Reusability**: Once written, you can import this library into any project without copy-pasting code.
3. **Namespace Safety**: Prevents naming collisions between global variables in different files.

### What is an EMA Filter?
Sensors (analog inputs, IMUs, ultrasonic sensors) are prone to high-frequency electrical or environmental noise. To smooth the signal, we use filters.
An **Exponential Moving Average (EMA) Filter** is a first-order Infinite Impulse Response (IIR) filter. Unlike a Simple Moving Average (SMA) which requires storing a buffer of the last $N$ readings in SRAM, the EMA only needs to remember **one value**: the last filtered output. This makes it extremely memory-efficient for 8-bit AVR microcontrollers.

---

## 🔬 Physics & Mathematics of EMA

### 1. The EMA Equation
The filter calculates a weighted sum of the current raw sensor reading and the previous filtered value:
$$y[n] = \alpha \cdot x[n] + (1 - \alpha) \cdot y[n-1]$$

Where:
- $y[n]$ = current filtered value
- $x[n]$ = current raw input value
- $y[n-1]$ = previous filtered value
- $\alpha$ (alpha) = smoothing factor ($0.0 \le \alpha \le 1.0$)

### 2. Tuning the $\alpha$ Parameter
The behavior of the filter is entirely controlled by the smoothing coefficient $\alpha$:
- **Small $\alpha$ (e.g., $0.05$ or $0.01$)**: Heavy smoothing. The filter heavily relies on past history. It rejects high-frequency noise very effectively, but introduces a large **phase lag** (delay in tracking the true signal).
- **Large $\alpha$ (e.g., $0.80$ or $0.90$)**: Light smoothing. The filter responds rapidly to sudden changes with minimal lag, but passes most high-frequency noise through.

```
Low Alpha (0.05):    ────── Noise blocked, but high delay (lag)
Medium Alpha (0.3):  ─── Balanced smoothing and delay
High Alpha (0.9):    ─ No delay, but noise passes through
```

---

## 🔩 Components Needed

No external hardware is strictly required! We run this sketch using the Arduino **Serial Plotter** which simulates a noisy analog sensor reading.
To test on real-world inputs:
- Any analog sensor (e.g., Potentiometer, LDR, or LM35 Temperature Sensor).

---

## 🔌 Pin-to-Pin Wiring

For the default simulation mode:
- **No wiring required**. Connect your Arduino Uno to the PC via USB.

For real-world sensor testing (Optional):
- Connect the signal pin of your analog sensor to **Analog Pin A0** of the Arduino.

---

## 💾 Alternatives to EMA Filtering

| Filter Type | SRAM Overhead | CPU Overhead | Use Case | Limitations |
| :--- | :--- | :--- | :--- | :--- |
| **EMA (Exponential)** | **O(1) - 4 bytes** | **O(1) - Very Low** | General sensor smoothing, low-memory boards. | Introduces phase lag. |
| **SMA (Simple Moving)** | O(N) - Needs buffer | O(1) with running sum | Step response smoothing, equal weighting. | High memory usage for large window sizes ($N$). |
| **Median Filter** | O(N) - Buffer & sorting | O(N log N) - Sorting | Removing sudden spikes (e.g. Ultrasonic spikes). | Modifies signal shapes, sorting takes CPU cycles. |
| **Kalman Filter** | O(1) - State space | High (Matrix/Floating math) | Sensor fusion (e.g., IMU + GPS). | Mathematically complex, difficult to tune. |

---

## 💻 How to Test & Validate

1. Open the Arduino IDE, load [Day_87_Custom_Library.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_87_Custom_Library/Day_87_Custom_Library.ino).
2. The library files [EMAFilter.h](file:///d:/Downloads/100%20days%20of%20Arduino/Day_87_Custom_Library/EMAFilter.h) and [EMAFilter.cpp](file:///d:/Downloads/100%20days%20of%20Arduino/Day_87_Custom_Library/EMAFilter.cpp) must be in the same folder as the `.ino` file. They will open as tabs in the Arduino IDE.
3. Select your Arduino Uno board and upload the code.
4. **Visualizing the Filter (Serial Plotter)**:
   - In the Arduino IDE, go to **Tools > Serial Plotter** (ensure baud rate is set to **115200**).
   - You will see three lines:
     - `Raw` (Blue): The noisy signal.
     - `Filtered` (Red): The output of your EMA filter.
     - `Clean` (Green): The underlying noiseless sine wave.
5. **Tuning parameters (Serial Monitor)**:
   - Close the Serial Plotter and open the **Serial Monitor** at **115200 Baud**.
   - You will see the CLI output showing instructions.
   - Send `+` to increase alpha (makes the filter faster, less smooth).
   - Send `-` to decrease alpha (makes the filter slower, smoother).
   - Send `n` to toggle white noise generation on or off.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `fatal error: EMAFilter.h: No such file or directory` | Library files are not in the same directory | Ensure `EMAFilter.h` and `EMAFilter.cpp` are inside the folder `Day_87_Custom_Library`. |
| Serial Monitor prints garbage characters | Baud rate mismatch | Change the Serial Monitor/Plotter baud rate setting to **115200**. |
| Filter output starts at 0.0 and lags heavily on startup | Filter memory initialized to zero | In the library constructor or `reset()`, we check if it is the first sample (`_isInitialized == false`) and seed the memory with the raw input value to eliminate startup lag. |

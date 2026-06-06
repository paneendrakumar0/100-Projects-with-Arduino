import os

content_day11 = """
## 🧠 Code Explanation

Let's break down how we sweep a servo without blocking the Arduino:

### 1. The Servo Library
```cpp
#include <Servo.h>
Servo myServo;
myServo.attach(SERVO_PIN);
```
- A servo motor needs a very specific 50Hz pulsing signal (PPM). Instead of calculating these exact microsecond pulses ourselves, the `<Servo.h>` library does all the heavy lifting in the background using hardware timers.

### 2. The Non-Blocking Sweep
```cpp
if (currentTime - lastStepTime >= stepDelay) {
    currentAngle += sweepDirection;
    
    if (currentAngle >= 180) {
        sweepDirection = -1; // Reverse direction
    } else if (currentAngle <= 0) {
        sweepDirection = 1; // Reverse direction
    }
    
    myServo.write(currentAngle);
}
```
- Standard tutorials use `for` loops and `delay()`. The problem is that while the `for` loop runs, the Arduino is frozen.
- Instead, every `15ms`, we simply increment the `currentAngle` by `sweepDirection` (+1 or -1) and write it to the servo. 
- When it hits `180` or `0`, we multiply the direction by `-1` to reverse the sweep!
"""
with open("Day_11_Servo_Sweep/README.md", "a", encoding="utf-8") as f:
    f.write(content_day11)


content_day12 = """
## 🧠 Code Explanation

Let's look at the professional way to link an analog input to a servo output:

### 1. The Deadband Noise Filter
```cpp
const int DEADBAND = 4;
int potValue = analogRead(POT_PIN);

if (abs(potValue - lastPotValue) > DEADBAND) {
    // Process the new value
}
```
- Potentiometers are noisy. Even when you aren't touching it, the ADC reading might bounce between `511`, `512`, and `513`.
- If we sent every single bounce to the servo, it would hum, grind its internal gears, overheat, and draw massive current spikes from your power supply.
- We calculate the absolute difference `abs(current - last)`. If the change is smaller than our `DEADBAND` (4 units), we ignore it! The servo remains completely silent until you intentionally turn the knob.

### 2. Mapping the Angle
```cpp
int targetAngle = map(potValue, 0, 1023, 0, 180);
myServo.write(targetAngle);
```
- The ADC gives us `0` to `1023`. The Servo needs `0` to `180`. The `map()` function mathematically squishes the larger range into the smaller range instantly.
"""
with open("Day_12_Pot_Servo/README.md", "a", encoding="utf-8") as f:
    f.write(content_day12)


content_day13 = """
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
"""
with open("Day_13_Joystick_Mapper/README.md", "a", encoding="utf-8") as f:
    f.write(content_day13)


content_day14 = """
## 🧠 Code Explanation

Let's break down how the Matrix Keypad library saves us massive headaches:

### 1. The Keypad Layout Map
```cpp
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
```
- We create a 2D array that visually matches our physical keypad. This tells the library exactly what character to return when a specific Row and Column intersect.

### 2. The Library Instantiation
```cpp
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
```
- We map the 8 physical wires to our Arduino pins.
- The `Keypad()` function binds our layout (`hexaKeys`) to our physical pins (`rowPins`, `colPins`).

### 3. Reading the Key
```cpp
char customKey = customKeypad.getKey();

if (customKey) {
    // A key was pressed!
}
```
- `getKey()` handles all the complex row/column multiplexing and button debouncing in the background. It returns `0` (null) if nothing is pressed, or the `char` (like 'A' or '5') if a button was cleanly pushed.
"""
with open("Day_14_Keypad_Interfacing/README.md", "a", encoding="utf-8") as f:
    f.write(content_day14)


content_day15 = """
## 🧠 Code Explanation

Let's break down the professional **Finite State Machine (FSM)** pattern used for this door lock:

### 1. Defining the States
```cpp
enum LockState {
  STATE_LOCKED,
  STATE_ENTERING,
  STATE_UNLOCKED,
  STATE_DENIED
};
LockState currentLockState = STATE_LOCKED;
```
- An `enum` (enumeration) creates custom named variables. Instead of remembering that "1 means locked" and "3 means denied", we give them clear human-readable names.

### 2. The Switch-Case Core
```cpp
switch (currentLockState) {
    case STATE_LOCKED:
        // Wait for a keypress...
        break;
    case STATE_UNLOCKED:
        if (millis() - stateTransitionTime >= 5000) {
            transitionToState(STATE_LOCKED);
        }
        break;
}
```
- A `switch` statement is like a giant `if/else` block. Every time the `loop()` runs, it checks our current state and *only* executes the code for that specific state. 
- When we are `STATE_UNLOCKED`, it just watches the `millis()` timer. Once 5 seconds pass, it switches the state back to `STATE_LOCKED`, locking the door automatically.

### 3. Password Buffer & Verification
```cpp
char inputBuffer[5]; // Stores "1234" + null terminator

if (strcmp(inputBuffer, CORRECT_PIN) == 0) {
    transitionToState(STATE_UNLOCKED);
} else {
    transitionToState(STATE_DENIED);
}
```
- As you type, the characters are added to `inputBuffer`. 
- When you press `#`, we use the C++ `strcmp()` (string compare) function. If it returns `0`, the strings are a perfect match, and we unlock the door!
"""
with open("Day_15_Password_Door_Lock/README.md", "a", encoding="utf-8") as f:
    f.write(content_day15)

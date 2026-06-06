import os

content_day1 = """
## 🧠 Code Explanation

Let's break down the actual code used in this project line-by-line:

### 1. Variables
```cpp
unsigned long previousMillis = 0; 
const long interval = 1000;       
bool ledState = LOW;
```
- `unsigned long`: A massive variable type. `millis()` gets huge very fast (up to 4 billion), so a standard `int` would crash your Arduino in 32 seconds! We use this to remember the last time we blinked.
- `interval`: The delay we want (1000ms = 1 second).
- `ledState`: A boolean variable (`true/false` or `HIGH/LOW`) to remember if the LED is currently ON or OFF.

### 2. The Setup
```cpp
void setup() {
    pinMode(ledPin, OUTPUT);
    Serial.begin(9600);
}
```
- `pinMode`: Tells the Arduino to push electricity OUT to the LED.
- `Serial.begin(9600)`: Opens the communication line to your computer at 9600 bits per second so we can print messages.

### 3. The Non-Blocking Logic
```cpp
unsigned long currentMillis = millis();

if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
}
```
- `currentMillis = millis()`: We check our stopwatch.
- `if (current - previous >= interval)`: This is the magic formula. It asks: "Has the time passed since my last action exceeded 1 second?"
- `previousMillis = currentMillis`: If yes, we immediately reset our memory of the "last action" to right now.
- `ledState = !ledState`: The `!` means "NOT". It flips the state. If it was ON, it becomes OFF.
- `digitalWrite`: Finally, we physically turn the pin ON or OFF based on our new `ledState`.
"""

with open("Day_01_Millis_Blink/README.md", "a", encoding="utf-8") as f:
    f.write(content_day1)


content_day2 = """
## 🧠 Code Explanation

Let's break down the Morse Code Generator:

### 1. The Struct and Array
```cpp
struct MorseEvent {
  bool state;
  unsigned int units;
};
const MorseEvent sosPattern[] = { {true, 1}, {false, 1}, ... };
```
- `struct`: A struct is a way to group related variables together. Here, we pair a `state` (ON/OFF) with a `duration` (how many time units).
- `sosPattern[]`: Instead of writing 50 lines of `digitalWrite`, we store our sequence in an array. This makes the code drastically cleaner and easy to modify for other words.

### 2. State Machine Logic
```cpp
if (currentTime - stepStartTime >= currentStepDuration) {
    currentStep++;
    if (currentStep >= totalSteps) currentStep = 0;
    startStep(currentStep);
}
```
- Just like Day 1, we use non-blocking `millis()` math to check if the current event is over.
- If it is over, we increment `currentStep` to move to the next event in our array.
- If we reach the end of the array, we reset `currentStep` to 0 to loop the SOS message forever!

### 3. Executing a Step
```cpp
void startStep(int stepIndex) {
    stepStartTime = millis();
    bool state = sosPattern[stepIndex].state;
    // ... turn pins HIGH or LOW based on 'state'
}
```
- This custom function reads the instruction from our array, applies the ON/OFF state to the LED and Buzzer, and most importantly, resets `stepStartTime` so the `loop()` knows exactly when to trigger the next event!
"""

with open("Day_02_SOS_Morse/README.md", "a", encoding="utf-8") as f:
    f.write(content_day2)


content_day3 = """
## 🧠 Code Explanation

Let's break down how we clean up the messy, bouncing signals from the pushbutton:

### 1. INPUT_PULLUP
```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```
- Normally, an unpressed button leaves the pin "floating" (picking up random static electricity). `INPUT_PULLUP` turns on a tiny resistor inside the Arduino that connects the pin to 5V. 
- Because of this, an UNPRESSED button reads as `HIGH`.
- When you press the button, it connects directly to Ground, overriding the weak resistor, causing it to read `LOW`.

### 2. Detecting a Change
```cpp
int reading = digitalRead(BUTTON_PIN);

if (reading != lastButtonState) {
    lastDebounceTime = millis();
}
```
- If the button physically bounces, the reading fluctuates wildly between HIGH and LOW.
- Every single time it fluctuates, it is "different" than the last state, so we reset our `lastDebounceTime` stopwatch back to 0.

### 3. Confirming a Stable State
```cpp
if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
        buttonState = reading;
        if (buttonState == LOW) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
    }
}
```
- The code only reaches inside this `if` statement when the button has stopped bouncing for 50 uninterrupted milliseconds.
- Once stable, we check if it transitioned to `LOW` (pressed!). If so, we toggle the `ledState` and write it to the LED pin.
"""

with open("Day_03_Pushbutton_Toggle/README.md", "a", encoding="utf-8") as f:
    f.write(content_day3)


content_day4 = """
## 🧠 Code Explanation

Let's look at how we translate analog voltages into LED brightness:

### 1. Reading the Potentiometer
```cpp
int adcValue = analogRead(POT_PIN);
```
- `analogRead()` asks the Arduino's built-in Analog-to-Digital Converter (ADC) to read the voltage on Pin A0.
- It converts 0V to 5V into a number between `0` and `1023`.

### 2. The Map Function
```cpp
int pwmValue = map(adcValue, 0, 1023, 0, 255);
```
- We cannot send `1023` to the LED because the LED pin only accepts 8-bit values (from `0` to `255`).
- `map()` is a brilliant math function that scales ranges. It takes our `adcValue` and mathematically shrinks the `0-1023` range down to a perfect `0-255` range for the LED!

### 3. PWM Output
```cpp
analogWrite(LED_PIN, pwmValue);
```
- Despite the name, `analogWrite()` does not output a true analog voltage. It outputs **PWM** (Pulse Width Modulation).
- It turns the LED fully ON and fully OFF almost 500 times a second. 
- A value of `127` means the LED is ON 50% of the time, making it appear half as bright to our eyes!

### 4. Telemetry Logging
```cpp
float voltage = adcValue * (5.0 / 1023.0);
```
- We do a bit of math to convert the raw `1023` back into real-world Volts so we can print it nicely to the Serial Monitor!
"""

with open("Day_04_Potentiometer_Fade/README.md", "a", encoding="utf-8") as f:
    f.write(content_day4)


content_day5 = """
## 🧠 Code Explanation

Let's break down the math behind the smooth Rainbow cycle:

### 1. The Sine Wave Math
```cpp
angle += angleIncrement;

int rVal = (sin(angle) + 1.0) * 127.5;
int gVal = (sin(angle + (2.0 * PI / 3.0)) + 1.0) * 127.5;
int bVal = (sin(angle + (4.0 * PI / 3.0)) + 1.0) * 127.5;
```
- To make a smooth rainbow, we treat the colors like a wave.
- `sin(angle)` generates a math wave that goes from `-1.0` to `1.0`. 
- Since PWM can't be negative, we add `1.0` to make it range from `0.0` to `2.0`.
- We then multiply by `127.5`. The result is a perfect wave oscillating between `0` and `255`!
- The secret to the rainbow is **phase shifting**. We shift the Green wave by 120 degrees (`2*PI/3` radians) and the Blue wave by 240 degrees. This ensures that when Red is fading out, Green is perfectly fading in!

### 2. Outputting the Colors
```cpp
analogWrite(RED_PIN, rVal);
analogWrite(GREEN_PIN, gVal);
analogWrite(BLUE_PIN, bVal);
```
- We use PWM on all three pins simultaneously.
- If `rVal = 255`, `gVal = 255`, and `bVal = 0`, the LED will shine Yellow. The smooth sine waves do all this blending for us automatically!
"""

with open("Day_05_RGB_Mixer/README.md", "a", encoding="utf-8") as f:
    f.write(content_day5)

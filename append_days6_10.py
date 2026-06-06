import os

content_day6 = """
## 🧠 Code Explanation

Let's look at how we implemented professional Hysteresis:

### 1. The Thresholds
```cpp
const int DARK_THRESHOLD = 350;
const int LIGHT_THRESHOLD = 450;
```
- If we only used one threshold (e.g., `400`), what happens when the room light fluctuates between `399` and `401` really fast? The LED would strobe rapidly! 
- By using two thresholds, we create a "dead band". The light must cross a solid 100-point margin to trigger a state change, making it extremely stable.

### 2. The Hysteresis Logic
```cpp
if (!nightLightActive && lightLevel < DARK_THRESHOLD) {
    nightLightActive = true;
    digitalWrite(LED_PIN, HIGH);
} 
else if (nightLightActive && lightLevel > LIGHT_THRESHOLD) {
    nightLightActive = false;
    digitalWrite(LED_PIN, LOW);
}
```
- `!nightLightActive`: If the light is currently OFF...
- `&& lightLevel < DARK_THRESHOLD`: AND the room drops below `350`...
- Then we turn the LED ON and update our memory (`nightLightActive = true`).
- The reverse happens when the sun comes up and the value rises past `450`!
"""

with open("Day_06_LDR_Night_Light/README.md", "a", encoding="utf-8") as f:
    f.write(content_day6)


content_day7 = """
## 🧠 Code Explanation

Let's break down how we measure distance using sound:

### 1. Triggering the Pulse
```cpp
digitalWrite(TRIG_PIN, LOW);
delayMicroseconds(2);
digitalWrite(TRIG_PIN, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);
```
- The sensor needs a very specific wake-up call. We hold the Trigger pin `LOW` to ensure it's clean, then blast it `HIGH` for exactly 10 microseconds. This tells the sensor: *"Fire the ultrasonic burst now!"*

### 2. Reading the Echo
```cpp
unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT);
```
- `pulseIn()` listens to a pin and waits for it to go `HIGH`. Once it goes HIGH, it starts a stopwatch and stops it when it goes `LOW`. It returns the duration in microseconds.
- `ECHO_TIMEOUT`: We set a hard timeout of 26,000µs. If we don't hear an echo by then, it means the sound travelled into the void (over 4.5 meters). This prevents our Arduino from freezing forever while waiting for a ghost echo.

### 3. The Math
```cpp
float distanceCm = duration / 58.3;
```
- Speed of sound = 343 m/s. 
- The sound travels TO the object and BACK, so we divide the total time by 2.
- After converting units from seconds/meters to microseconds/centimeters, the magic constant simplifies to `58.3`!
"""

with open("Day_07_Ultrasonic_Distance/README.md", "a", encoding="utf-8") as f:
    f.write(content_day7)


content_day8 = """
## 🧠 Code Explanation

Let's see how we handled the PIR sensor's unique quirks:

### 1. Calibration Warm-up
```cpp
for (int i = 0; i < CALIBRATION_TIME; i++) {
    // ... blink the LED and wait
}
```
- A PIR sensor takes 30-60 seconds to "learn" the background infrared heat signature of the room. If you check it too early, it will trigger false alarms. This `for` loop forces the Arduino to wait before arming the system.

### 2. Edge Triggering
```cpp
int currentPirState = digitalRead(PIR_PIN);

if (currentPirState != lastPirState) {
    if (currentPirState == HIGH) {
        // Motion Started!
    } else {
        // Motion Stopped!
    }
    lastPirState = currentPirState;
}
```
- Just like the pushbutton debounce from Day 3, we only want to act when the state **changes**. 
- If someone is dancing in front of the sensor, we don't want the Serial Monitor to print "Motion Detected!" 1,000 times a second. 
- By checking `if (current != last)`, we only print the message exactly once when motion *starts*, and once when it *stops*.
"""

with open("Day_08_PIR_Alarm/README.md", "a", encoding="utf-8") as f:
    f.write(content_day8)


content_day9 = """
## 🧠 Code Explanation

Let's break down how we use a Library to read the DHT11:

### 1. Loading the Library
```cpp
#include "DHT.h"
#define DHTTYPE DHT11 
DHT dht(DHT_PIN, DHTTYPE);
```
- `#include` tells the compiler to grab the Adafruit DHT code.
- We define what type of sensor we have (DHT11, DHT22, etc.).
- We create an *object* called `dht` linked to pin 2. 

### 2. Reading Data
```cpp
float humidity = dht.readHumidity();
float tempC = dht.readTemperature();
```
- Because we included the library, all the complex 40-bit timing protocols are handled for us behind the scenes. We just ask the library for the numbers!

### 3. Validation Checks (NaN)
```cpp
if (isnan(humidity) || isnan(tempC)) {
    Serial.println("[ERROR] Failed to read from DHT sensor!");
    return;
}
```
- `isnan()` stands for "Is Not A Number".
- If the sensor is unplugged or broken, the library returns `NaN`. If we tried to do math with `NaN`, our program would crash. Always validate your sensor readings!
"""

with open("Day_09_DHT11_Logger/README.md", "a", encoding="utf-8") as f:
    f.write(content_day9)


content_day10 = """
## 🧠 Code Explanation

Let's look at how we control a passive buzzer using `tone()`:

### 1. The Pre-processor Directives
```cpp
#define IS_PASSIVE_BUZZER true

#if IS_PASSIVE_BUZZER
    // Passive buzzer code...
#else
    // Active buzzer code...
#endif
```
- The `#define` and `#if` statements are **Pre-processor Directives**. Before your code is even uploaded to the Arduino, the compiler looks at this.
- If `IS_PASSIVE_BUZZER` is true, it literally deletes the active buzzer code from memory. This saves valuable space on the microcontroller!

### 2. Playing the Melody
```cpp
int note = melody[stepIndex];
currentStepDuration = 1000 / noteDurations[stepIndex];
```
- We store the frequencies (e.g. 262 Hz for Middle C) in the `melody` array.
- We store the note type in the `noteDurations` array. A "4" means a quarter note, which lasts `1000ms / 4 = 250ms`.

### 3. The `tone()` Function
```cpp
tone(BUZZER_PIN, note, currentStepDuration);
```
- `tone()` automatically generates a square wave at the exact frequency specified by the `note` variable, effectively playing music!
"""

with open("Day_10_Buzzer_Melody/README.md", "a", encoding="utf-8") as f:
    f.write(content_day10)

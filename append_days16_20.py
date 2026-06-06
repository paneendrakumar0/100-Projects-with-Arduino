import os

content_day16 = """
## 🧠 Code Explanation

Let's break down how we control high-voltage appliances safely using a Relay:

### 1. Active-Low vs Active-High
```cpp
#define RELAY_ACTIVE_STATE LOW

const int RELAY_ON = RELAY_ACTIVE_STATE;
const int RELAY_OFF = (RELAY_ACTIVE_STATE == LOW) ? HIGH : LOW;
```
- Most Arduino relay modules use an optical isolator (an LED inside a chip) for safety. To turn on this LED, we actually have to pull the Arduino pin `LOW` (GND), creating a path for the current to flow *into* the Arduino.
- Because `LOW = ON` is very confusing to read in code, we use macros and logic to define `RELAY_ON` and `RELAY_OFF`. This makes the main loop incredibly easy to read.

### 2. Boot Safety
```cpp
digitalWrite(RELAY_PIN, RELAY_OFF);
```
- We write the `OFF` state to the pin immediately in `setup()`. If we didn't do this, a high-power motor or heater might briefly jerk to life the moment the Arduino turns on, which is extremely dangerous!

### 3. Non-Blocking Switching
```cpp
if (currentTime - lastSwitchTime >= switchInterval) {
    relayArmedState = !relayArmedState;
    if (relayArmedState) {
        digitalWrite(RELAY_PIN, RELAY_ON);
    } else {
        digitalWrite(RELAY_PIN, RELAY_OFF);
    }
}
```
- Just like our LED blink from Day 1, we toggle the `relayArmedState` every 5 seconds. The Arduino is free to check sensors or buttons during those 5 seconds because we aren't using `delay(5000)`!
"""
with open("Day_16_Relay_Control/README.md", "a", encoding="utf-8") as f:
    f.write(content_day16)


content_day17 = """
## 🧠 Code Explanation

Let's break down how we solved the "Echo" problem using a software lockout:

### 1. The Raw Input
```cpp
int soundReading = digitalRead(SOUND_PIN);
```
- A clap generates a massive spike in air pressure. The sound sensor's onboard amplifier detects this spike and pulls its output pin `LOW`.
- However, the sound wave bounces off your walls, creating echoes. The sensor might pull the pin `LOW` 15 times in the span of a single second!

### 2. The Lockout Filter (Anti-Echo)
```cpp
if (soundReading == LOW && (currentTime - lastClapTime >= lockoutDuration)) {
    lastClapTime = currentTime;
    ledState = !ledState;
}
```
- We check two things: "Did I hear a sound?" AND "Has it been at least 250 milliseconds since the LAST time I acted on a sound?"
- If we hear a clap, we immediately log the `lastClapTime`. 
- Over the next `250ms`, the echoes trigger the `soundReading == LOW` condition, but they completely fail the second condition. The echoes are ignored, and the LED toggles perfectly just once!
"""
with open("Day_17_Sound_Clap_Switch/README.md", "a", encoding="utf-8") as f:
    f.write(content_day17)


content_day18 = """
## 🧠 Code Explanation

Let's look at the professional technique used to prevent our sensor from rusting away:

### 1. Power Gating
```cpp
const int SENSOR_POWER_PIN = 7; 
digitalWrite(SENSOR_POWER_PIN, HIGH);
delay(10); 
int sensorValue = analogRead(SENSOR_SIGNAL_PIN);
digitalWrite(SENSOR_POWER_PIN, LOW);
```
- In beginner tutorials, the sensor's VCC pin is plugged directly into the Arduino's 5V pin. This means electricity flows through the water 24/7. This causes **Galvanic Electrolysis**, literally ripping the copper off the sensor until it dissolves into green mush in just a few days.
- **The Fix:** We plug VCC into Digital Pin 7. We only turn Pin 7 `HIGH` right before we take a reading. 
- We wait `10ms` for the electricity to stabilize, grab our `analogRead`, and immediately turn Pin 7 `LOW`. 
- Electricity is now flowing through the water for only 10 milliseconds out of every 1000 milliseconds. The sensor will now last years instead of days!

### 2. String Translation
```cpp
String statusString = "";
if (sensorValue < THRESHOLD_DRY) {
    statusString = "DRY (Empty)";
}
```
- Raw ADC numbers (like `304`) mean nothing to a user. We use an `if/else` ladder against our calibrated thresholds to convert those numbers into highly descriptive English text for our telemetry logs.
"""
with open("Day_18_Water_Level/README.md", "a", encoding="utf-8") as f:
    f.write(content_day18)


content_day19 = """
## 🧠 Code Explanation

Let's break down the logic for measuring soil saturation:

### 1. Inverted Analog Logic
```cpp
const int VAL_DRY = 750;
const int VAL_WET = 350;

if (sensorValue > VAL_DRY) {
    soilStatus = "DRY (Needs Watering!)";
} else if (sensorValue < VAL_WET) {
    soilStatus = "WET (Saturated)";
}
```
- Resistive soil probes work inversely to what you might expect.
- When the soil is bone dry, it acts like an insulator. Resistance goes up, voltage stays high, and the ADC reports a huge number (e.g., `800+`).
- When the soil is soaked, the water acts as a conductor. Resistance drops, pulling the signal down to GND, and the ADC reports a tiny number (e.g., `250`).

### 2. Safe Mathematical Constraining
```cpp
float moisturePercent = map(sensorValue, 1023, 0, 0, 100);
moisturePercent = constrain(moisturePercent, 0.0, 100.0);
```
- We use the `map()` function to convert the `1023-0` scale into a `0%-100%` scale. 
- However, if the sensor reads higher than 1023 or lower than 0 (which can happen due to noise or varying calibrations), the map function will output weird values like `-5%` or `110%`.
- The `constrain(value, min, max)` function is a professional safety net. It strictly forces the variable to stay within the `0` to `100` boundary, preventing UI bugs down the road!
"""
with open("Day_19_Soil_Moisture/README.md", "a", encoding="utf-8") as f:
    f.write(content_day19)


content_day20 = """
## 🧠 Code Explanation

Let's look at how we built a highly responsive, non-blocking fire alarm:

### 1. Dual-Path Reading
```cpp
int rawAnalog = analogRead(FLAME_ANALOG_PIN);
int digitalTrigger = digitalRead(FLAME_DIGITAL_PIN); 

if (rawAnalog < FLAME_THRESHOLD || digitalTrigger == LOW) {
    isAlarmActive = true;
}
```
- We read both the raw Analog intensity of the infrared light AND the instantaneous Digital trigger from the sensor's onboard amplifier.
- We use the `||` (Logical OR) operator. If *either* the analog drops past our custom threshold OR the digital pin snaps LOW, we instantly arm the alarm. This guarantees we don't miss a flash fire!

### 2. Adaptive Telemetry Logging
```cpp
unsigned long currentInterval = isAlarmActive ? alertLogInterval : idleLogInterval;

if (currentTime - lastLogTime >= currentInterval) {
    // print logs
}
```
- This is a clever use of the Ternary Operator (`Condition ? True : False`). 
- If the alarm is active, the `currentInterval` becomes `100ms` so we can rapidly monitor the fire's growth. 
- If the room is safe, it defaults back to `500ms` so we don't spam the Serial Monitor with useless "Safe" messages.

### 3. Non-Blocking Siren
```cpp
if (isAlarmActive) {
    if (currentTime - lastPulseTime >= alarmPulseDelay) {
        alarmPulseState = !alarmPulseState;
        if (alarmPulseState) tone(ALARM_BUZZER_PIN, 2500);
        else noTone(ALARM_BUZZER_PIN);
    }
}
```
- While the alarm is active, we use our `millis()` stopwatch to toggle the Buzzer and LED on and off every `150ms`. 
- Because we didn't use `delay(150)`, the Arduino is still instantly checking the flame sensor thousands of times a second in the background. The second the fire goes out, the siren immediately stops!
"""
with open("Day_20_Flame_Sensor/README.md", "a", encoding="utf-8") as f:
    f.write(content_day20)

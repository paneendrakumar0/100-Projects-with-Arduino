# Day 1: The Non-Blocking Blink (Ditch the delay!)

Welcome to Day 1 of your Arduino journey.

If you look at most beginner Arduino tutorials, the very first thing they teach you is how to blink an LED using the `delay()` function. **We are not going to do that.** Why? Because `delay()` is a trap. It is a "blocking" function. When you tell an Arduino to `delay(1000)`, it literally freezes for one full second. It cannot read sensors, it cannot stop a motor, and it cannot listen to your commands. If you want to build robots, drones, or smart home systems, a frozen microcontroller is useless.

Today, you will learn the professional way to time events: the **Non-Blocking Blink** using `millis()`.

## 🎯 Today's Learning Goals

1. Understand the difference between blocking and non-blocking code.
2. Learn how to use the `millis()` function as an internal stopwatch.
3. Wire a basic LED circuit (or use the onboard LED).

## 🔌 Circuit Diagram & Wiring

You have two options for this project:

**Option 1: Use the Onboard LED (No wiring needed)**
Your Arduino already has a tiny LED built into the board, internally connected to Pin 13. You can just plug the Arduino into your computer and run the code!

**Option 2: Wire an External LED**
If you want to build the circuit yourself on a breadboard:

1. Connect a jumper wire from the **GND** pin on the Arduino to the blue negative rail on your breadboard.
2. Connect a **220Ω or 330Ω resistor** from the GND rail to a row on the breadboard.
3. Insert the **LED**. The short leg (cathode) goes to the resistor.
4. Connect the long leg (anode) of the LED to **Pin 13** on the Arduino.

## 🧠 How the Code Works

Instead of freezing the Arduino, we use `millis()`. Think of `millis()` as a stopwatch that starts ticking the millisecond your Arduino powers on.

In our code, we constantly check this stopwatch: _"Has one second passed since I last changed the LED?"_

- If **NO**: Keep running the rest of the code.
- If **YES**: Flip the LED on (or off), record the new time, and keep going.

This allows the Arduino's `loop()` to run thousands of times a second without ever stopping! Upload the code below to see it in action.

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

# Chapter 16: The Secret to Multitasking (`millis()` vs `delay()`)

This is the most important concept in advanced Arduino programming. If you learn this, you graduate from "beginner" to "intermediate."

When you learned to blink an LED, you used the `delay()` function:
```cpp
digitalWrite(13, HIGH);
delay(1000); // Wait for 1 second
digitalWrite(13, LOW);
delay(1000); // Wait for 1 second
```

## The Problem with `delay()`

`delay()` is a **blocking function**. It literally stops the Arduino's brain. 
If you tell the Arduino to `delay(1000);`, it cannot do *anything* else for that 1 second. It cannot read a button, it cannot check a sensor, it cannot steer a robot. It is paralyzed.

If you try to build a robot that blinks an LED *and* checks for obstacles using `delay()`, the robot will crash into a wall while waiting for the LED to blink.

## The Solution: `millis()`

The `millis()` function is a built-in stopwatch. The moment your Arduino turns on, the stopwatch starts counting in milliseconds (1000 milliseconds = 1 second).

Instead of paralyzing the Arduino, you let it run as fast as possible, constantly checking the stopwatch: *"Is it time to do the thing yet?"*

### The `millis()` Template (Blink Without Delay)

Here is how you blink an LED *without* stopping the Arduino:

```cpp
const int ledPin = 13;
int ledState = LOW;

// We need a variable to store the LAST time we blinked the LED
unsigned long previousMillis = 0; 
// We want to blink every 1000 milliseconds
const long interval = 1000; 

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Check the stopwatch right now
  unsigned long currentMillis = millis();

  // If the current time minus the last time we blinked is greater than our interval...
  if (currentMillis - previousMillis >= interval) {
    // Save the last time you blinked the LED!
    previousMillis = currentMillis;

    // Toggle the LED
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(ledPin, ledState);
  }

  // Look at this! The Arduino is totally free down here!
  // It is not blocked! You can read buttons, steer servos, or talk to sensors 
  // millions of times a second while the LED blinks perfectly in the background.
}
```

## Why `unsigned long`?
The `millis()` stopwatch counts very high, very fast. If you try to store the time in a standard `int`, the variable will "overflow" (run out of space) after just 32 seconds! 
An `unsigned long` is a massive variable that can count for 50 days before overflowing. Always use `unsigned long` for timers!

---

[<-- Back to Main Guide](./README.md)

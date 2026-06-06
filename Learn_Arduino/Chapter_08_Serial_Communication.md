# Chapter 8: Serial Communication

When your Arduino is running on a battery in a robot, you can't easily tell what numbers the sensors are reading, or if it's stuck in an infinite loop. 
To look inside the "brain" of the Arduino, we use **Serial Communication** over the USB cable.

## Opening the Line of Communication

Before the Arduino can talk to your PC, you have to tell it how fast to talk. We do this in the `setup()` function using `Serial.begin()`.

```cpp
void setup() {
  // 9600 is the Baud Rate (bits per second). 
  // Both the Arduino and the PC must agree on this speed!
  Serial.begin(9600); 
}
```

## Talking to the PC

We have two main commands to send data back to the computer:

1. **`Serial.print()`:** Prints the text/data.
2. **`Serial.println()`:** Prints the text/data, and then hits "Enter" to move to a new line.

```cpp
void loop() {
  Serial.print("The temperature is: ");
  Serial.println(75);
  delay(1000);
}
```
If you upload this code, nothing happens on the board itself. You need to open the **Serial Monitor**.

## The Serial Monitor

In the top right corner of your Arduino IDE, click the icon that looks like a magnifying glass (or go to `Tools > Serial Monitor`).

A window will open at the bottom of the screen. 
Make sure the dropdown menu in the corner of the Serial Monitor says **"9600 baud"**. If it says something else, you will just see random garbage characters like `⸮⸮$!` because the PC is listening at the wrong speed!

If set correctly, you will see:
```text
The temperature is: 75
The temperature is: 75
The temperature is: 75
```

## Debugging with Serial

The greatest use of Serial communication for beginners is **Debugging** (finding out why your code doesn't work).

If your robot isn't turning left when it hits a wall, how do you know if the motor is broken, or if the ultrasonic sensor isn't reading the wall properly?

Print the sensor value!

```cpp
void loop() {
  int distance = analogRead(A0);
  
  // This helps you see exactly what the Arduino is "seeing"
  Serial.print("Distance Sensor = ");
  Serial.println(distance);

  if (distance < 100) {
    turnLeft();
  }
}
```

By looking at the Serial Monitor, you can instantly tell if the sensor is outputting 50, 900, or just staying at 0.

---

Serial communication is powerful. You can even use `Serial.read()` to send commands *from* your PC keyboard *to* the Arduino to control things manually! (You'll see this in Day 39: Bluetooth Control).

Now, let's look at how to expand your Arduino's capabilities without writing millions of lines of code.

**[Next Chapter: Libraries and Sensors ->](./Chapter_09_Libraries_and_Sensors.md)**

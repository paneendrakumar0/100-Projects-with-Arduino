# Chapter 6: Digital and Analog I/O

I/O stands for Input / Output. 
This is how your Arduino interacts with the world. Inputs are how it *listens* (reading sensors, buttons), and Outputs are how it *speaks* (turning on motors, LEDs, buzzers).

The Arduino Uno operates purely on electrical voltage. 

## Digital vs. Analog

Before writing code, we need to understand the difference between Digital and Analog signals.

- **Digital:** Think of a simple light switch. It is either completely ON or completely OFF. There is no in-between. In Arduino terms, this is `HIGH` (5V) or `LOW` (0V).
- **Analog:** Think of a volume knob or a dimmer switch. It can be anywhere from 0 to 100%. In Arduino terms, it can read a smooth curve of voltages anywhere between 0V and 5V.

---

## 1. Setting the Pin Mode: `pinMode()`

Before you can read or write to a pin, you MUST tell the Arduino what you plan to do with it. You do this inside the `setup()` function using `pinMode()`.

```cpp
void setup() {
  pinMode(13, OUTPUT); // Tells the Arduino we want to send voltage OUT of pin 13
  pinMode(2, INPUT);   // Tells the Arduino we want to read voltage coming IN to pin 2
}
```

### The Magic of `INPUT_PULLUP`
If you connect a push-button to pin 2 and ground, what is the voltage on pin 2 when the button is NOT pressed? It's not 0V, it's actually "floating" (picking up random static electricity from the air). 
To fix this, we need a pull-up resistor. The Arduino has them built-in! Just use:
`pinMode(2, INPUT_PULLUP);`
This forces the pin to read `HIGH` until you press the button, connecting it to ground and making it read `LOW`.

---

## 2. Digital Output: `digitalWrite()`

Use this to turn things fully ON or fully OFF.

```cpp
void loop() {
  digitalWrite(13, HIGH); // Send 5V out of pin 13 (LED turns ON)
  delay(1000);            // Wait for 1000 milliseconds (1 second)
  digitalWrite(13, LOW);  // Send 0V out of pin 13 (LED turns OFF)
  delay(1000);
}
```
*Note: `delay()` stops the Arduino from doing anything else for that amount of time. In the 100 Days of Arduino challenge, you will learn why professionals rarely use `delay()`!*

---

## 3. Digital Input: `digitalRead()`

Use this to see if a button is pressed, or if a digital sensor has triggered.

```cpp
int buttonPin = 2; // We connected a button to pin 2

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  int buttonState = digitalRead(buttonPin); // Check the voltage on pin 2
  
  // buttonState will be HIGH if not pressed, and LOW if pressed
}
```

---

## 4. Analog Input: `analogRead()`

The Arduino Uno has 6 dedicated Analog pins (A0 through A5).
When you read these pins, they have a 10-bit resolution. This means instead of `HIGH` or `LOW`, they return a number from **0 to 1023**.
0 = 0 Volts.
1023 = 5 Volts.
512 = ~2.5 Volts.

```cpp
void setup() {
  // Analog pins do not technically need pinMode declared, but it's good practice.
}

void loop() {
  int sensorValue = analogRead(A0); // Read a potentiometer or light sensor
}
```

---

## 5. Analog Output (Faking it): `analogWrite()`

Microcontrollers can't actually output a true analog voltage (like 2.5V). They can only output 5V or 0V.
So how do we dim an LED or slow down a motor? We cheat using **PWM (Pulse Width Modulation)**.

PWM turns the 5V pin ON and OFF incredibly fast. If it's ON 50% of the time and OFF 50% of the time, the LED looks like it is at 50% brightness.

You can only use `analogWrite()` on pins that have a tilde `~` next to them on the board (Pins 3, 5, 6, 9, 10, 11).
Unlike `analogRead()` which goes up to 1023, `analogWrite()` only goes from **0 to 255**.

```cpp
void setup() {
  pinMode(9, OUTPUT);
}

void loop() {
  analogWrite(9, 127); // 127 is exactly half of 255. The LED will be at 50% brightness.
}
```

Now you know how to read and write to the outside world. But how do we tell the Arduino to make decisions based on what it reads?

**[Next Chapter: Control Flow ->](./Chapter_07_Control_Flow.md)**

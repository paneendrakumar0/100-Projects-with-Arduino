# Chapter 7: Control Flow

Code normally executes linearly—top to bottom, line by line. 
But a smart robot needs to make decisions. If the temperature is too hot, turn on a fan. If an obstacle is detected, turn left. 

We change the path of our code using **Control Flow**.

## 1. The `if` Statement

This is the most basic decision-making tool. The code inside the `{ }` only runs if the condition inside the `( )` is true.

```cpp
int temp = 80;

void loop() {
  if (temp > 75) {
    // This code ONLY runs if temp is greater than 75
    digitalWrite(13, HIGH); 
  }
}
```

### Comparison Operators
How do we compare things inside the `if` statement?
- `==` : Equal to (Notice it is TWO equals signs! One `=` assigns a value, two `==` compares values).
- `!=` : Not equal to
- `<` : Less than
- `>` : Greater than
- `<=` : Less than or equal to
- `>=` : Greater than or equal to

## 2. `else` and `else if`

What if you want to do something else when the condition is false? 

```cpp
int buttonState = digitalRead(2);

if (buttonState == LOW) {
  // The button is being pressed
  digitalWrite(13, HIGH); // Turn LED on
} else {
  // The button is NOT being pressed
  digitalWrite(13, LOW);  // Turn LED off
}
```

You can chain multiple conditions together using `else if`:

```cpp
int analogSensor = analogRead(A0);

if (analogSensor > 800) {
  // Do something for high values
} else if (analogSensor > 400) {
  // Do something for medium values
} else {
  // Do something for low values
}
```

## 3. The `for` Loop

If you want to repeat a specific block of code a certain number of times, use a `for` loop. It's great for fading LEDs or stepping motors.

A `for` loop has three parts:
1. **Initialization:** Make a counter variable (usually `i`).
2. **Condition:** Keep looping as long as this is true.
3. **Increment:** What to do to `i` after every loop.

```cpp
// This loop runs 10 times. i goes from 0 to 9.
for (int i = 0; i < 10; i++) {
  // i++ means "add 1 to i"
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
}
```

## 4. The `while` Loop

A `while` loop runs continuously as long as its condition remains true.

> [!WARNING]  
> **Infinite Loops!**
> If the condition never becomes false, the Arduino gets trapped in the `while` loop forever and stops responding. Always make sure the condition can eventually be met!

```cpp
int count = 0;

while (count < 5) {
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
  
  count = count + 1; // If we didn't do this, it would loop forever!
}
```

## 5. The `switch...case` Statement

If you have a variable that can have many specific values (like a menu state, or a button press), writing 10 `if / else if` statements gets messy. `switch` is cleaner.

```cpp
int myState = 2;

switch (myState) {
  case 1:
    // Do thing 1
    break; // You MUST put break, or it will fall through to case 2!
  case 2:
    // Do thing 2
    break;
  default:
    // If it doesn't match any case, do this
    break;
}
```
State Machines (which you will see a lot in the 100 Days of Arduino) rely heavily on `switch...case` statements.

We know how to make the Arduino think. But how do we know what it's thinking? We need it to talk to us!

**[Next Chapter: Serial Communication ->](./Chapter_08_Serial_Communication.md)**

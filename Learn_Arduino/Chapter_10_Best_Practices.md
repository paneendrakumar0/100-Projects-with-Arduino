# Chapter 10: Best Practices

Congratulations on making it to the final chapter of the beginner's guide! 
You now have the foundation required to start building. But before you dive into **Day 1 of the 100 Days of Arduino**, there are a few architectural rules you need to know.

If you follow these rules, you will write professional-level code. If you ignore them, your projects will eventually become buggy, tangled messes.

---

## 1. The `delay()` Function is Evil

When you write `delay(1000);`, you are telling the microcontroller: "Stop everything you are doing, stare at a wall, and ignore the entire world for 1 second."
If a user presses a button during that 1 second, the Arduino will not register it. If a robot is driving towards a wall during that 1 second, it will crash.

**The Solution: Non-Blocking Code with `millis()`**

`millis()` is a built-in stopwatch that starts counting up from 0 the moment the Arduino turns on. Instead of pausing the whole program, we just check the stopwatch!

```cpp
unsigned long previousTime = 0;
const long interval = 1000; // 1 second

void loop() {
  unsigned long currentTime = millis();

  // If 1 second has passed...
  if (currentTime - previousTime >= interval) {
    previousTime = currentTime; // Reset the timer
    
    // Do the thing you want to do!
    blinkLED();
  }
  
  // The Arduino immediately continues doing other things here!
  // It never stopped to wait.
  readButtons();
}
```
*Note: You will master this exact concept in **Day 1** of the challenge!*

---

## 2. Magic Numbers are Bad

If you have an LED on pin 13, do not write `digitalWrite(13, HIGH);` everywhere in your code. 
Why? Because if you later decide to move the LED to pin 9, you have to hunt down every single `13` in your code and change it.

**The Solution: Use Constants**

At the very top of your file, declare a constant variable. 

```cpp
const int LED_PIN = 13;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
}
```
Now, if you move the LED, you only have to change the number in **one place**.

---

## 3. Formatting and Commenting

A messy codebase is impossible to debug.
- **Indent your code!** Everything inside `{ }` brackets should be pushed in by 2 or 4 spaces. (The Arduino IDE can do this automatically for you: Press `Ctrl + T` or `Cmd + T`).
- **Comment the "Why", not the "What".**
  - Bad comment: `digitalWrite(13, HIGH); // Turns pin 13 HIGH` (We already know that from reading the code).
  - Good comment: `digitalWrite(VALVE_PIN, HIGH); // Open the water valve to start irrigation` (This explains the *intent* of the code).

---

## 4. State Machines

As your projects get complex (like a digital clock with a setting menu), using hundreds of `if` statements will break your brain.

Get used to the concept of **Finite State Machines (FSM)**. You use an integer (or an `enum`) to track what "State" the machine is in, and a `switch...case` to only run the code for that state.

```cpp
int robotState = 0; // 0 = idle, 1 = driving, 2 = turning

void loop() {
  switch (robotState) {
    case 0:
      // Code to wait for a button press
      break;
    case 1:
      // Code to drive forward
      break;
    case 2:
      // Code to turn
      break;
  }
}
```
You will see State Machines frequently from **Day 15** onward.

---

# You Are Ready.

You have completed the **Learn Arduino** guide. 
You know the hardware, the physics, the programming grammar, and the architecture.

It is time to begin the journey. 
Head back to the main repository, open the **Day 1** folder, and start building!

[<-- Back to Main Repository](../README.md)

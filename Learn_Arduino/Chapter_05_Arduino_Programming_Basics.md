# Chapter 5: Arduino Programming Basics

Arduino is programmed using a language heavily based on **C++**. 

If you have never coded before, C++ can look a little intimidating. It requires strict formatting. Forgetting a single semicolon `;` will break the whole program! Let's break down the anatomy of an Arduino sketch.

## The Anatomy of a Sketch

Every Arduino program (called a "sketch") MUST have two specific functions: `setup()` and `loop()`.

```cpp
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
```

### 1. `setup()`
When you power on the Arduino (or press the reset button), the microcontroller reads the code inside the `{ }` brackets of the `setup()` function exactly **one time**.
We use this area to configure the Arduino. (e.g., "Set pin 13 as an output", "Start communicating with the computer").

### 2. `loop()`
Immediately after `setup()` finishes, the Arduino jumps into the `loop()` function. It reads the code line by line, top to bottom. When it reaches the bottom `}`, it immediately jumps back to the top and does it again. 
It will do this millions of times a second until you unplug the power.

---

## Syntax Rules (The Grammar of Code)

1. **Case Sensitivity:** `digitalwrite` is NOT the same as `digitalWrite`. Arduino code is case-sensitive. Most built-in functions use "camelCase" (the first word is lowercase, the second word is capitalized).
2. **Semicolons `;`:** Almost every single line of instruction must end with a semicolon. Think of it as a period at the end of a sentence.
3. **Curly Braces `{ }`:** These group lines of code together. Every open brace `{` must have a closing brace `}`.
4. **Comments `//`:** If you type `//`, the Arduino will ignore everything after it on that line. This is used to leave notes for humans to read.

---

## Variables and Data Types

A variable is a named bucket where you can store data in the Arduino's memory. 
Because C++ is "strongly typed," you have to tell the Arduino exactly what *kind* of bucket you are making before you can put data in it.

Here are the most common data types you will use:

1. **`int` (Integer):** Whole numbers (e.g., -5, 0, 10, 400).
2. **`float` (Floating-point):** Numbers with decimals (e.g., 3.14, -0.5).
3. **`boolean` or `bool`:** Can only hold two values: `true` or `false` (or `HIGH`/`LOW`, `1`/`0`).
4. **`char`:** A single character, enclosed in single quotes (e.g., `'A'`, `'z'`).
5. **`String`:** Text, enclosed in double quotes (e.g., `"Hello World"`).

### How to use variables:

```cpp
int myAge = 25;       // Created an integer named myAge, set it to 25
float temperature = 72.5; // Created a float named temperature
bool isLightOn = false;   // Created a boolean set to false

void setup() {
  // You can change the value of variables later!
  myAge = 26; 
}
```

---

## Global vs. Local Scope

Where you create your variable matters!

If you create a variable **outside** of all functions (usually at the very top of your file), it is a **Global Variable**. Any function can see it and change it.

If you create a variable **inside** the curly braces `{ }` of a function (like inside `loop()`), it is a **Local Variable**. It only exists inside those braces. Once the function ends, the variable is destroyed and forgotten.

```cpp
int counter = 0; // GLOBAL variable. setup() and loop() can both see this.

void loop() {
  int tempValue = 5; // LOCAL variable. Only loop() can see this.
  
  counter = counter + 1; // This works!
}
```

Now that we know how to write basic code and store data, let's learn how to make the Arduino interact with the physical world!

**[Next Chapter: Digital and Analog I/O ->](./Chapter_06_Digital_and_Analog_IO.md)**

# Chapter 9: Libraries and Sensors

So far, we've used basic functions like `digitalWrite()` and `analogRead()`. But what happens when you buy a complex sensor, like an OLED screen, or a GPS module?

These devices communicate using complex protocols (like I2C or SPI). Writing the raw code to tell an OLED screen "turn on pixel at X: 10, Y: 40" would take hundreds of lines of extremely confusing math and timing logic.

This is where **Libraries** come in.

## What is a Library?

A library is a package of pre-written code created by other developers. It abstracts away the insanely complex hardware math and gives you simple, easy-to-use commands.

Instead of writing 500 lines of code to initialize an OLED screen, a library lets you just write:
`display.print("Hello");`

## How to Install Libraries

The Arduino IDE has a built-in "app store" for libraries called the **Library Manager**.

1. In the Arduino IDE, go to the left-hand sidebar and click the icon that looks like a stack of books (or go to `Sketch > Include Library > Manage Libraries...`).
2. Search for the component you are using. For example, search "DHT11" if you have a temperature sensor.
3. You will see several options. Look for the one made by "Adafruit" or the community standard.
4. Click **Install**. (If it asks to install dependencies, always click "Install All").

## Including a Library in your Code

Once installed, you must tell your sketch to use it. You do this at the very top of your file using the `#include` directive.

```cpp
#include <DHT.h> // We are telling the Arduino to load the DHT code

// Now we can use the special objects and functions the library provides!
DHT mySensor(2, DHT11); 

void setup() {
  Serial.begin(9600);
  mySensor.begin(); // Complex startup logic hidden behind one word!
}

void loop() {
  float temp = mySensor.readTemperature(); // Easy!
  Serial.println(temp);
  delay(2000);
}
```

## Finding the Examples

How do you know what commands a library gives you? You don't have to guess!

Almost every good library comes with example sketches. 
After installing a library, go to **File > Examples**. Scroll down to the section "Examples from Custom Libraries". 
Find the folder for your new library and open the "Basic" or "Hello World" example. 

This is the absolute best way to learn how to use a new sensor. Read their example code, see how they set it up, and then copy/paste the parts you need into your own project.

---

You now possess the knowledge to read inputs, write outputs, make decisions, debug, and use complex sensors. You are officially ready to start building. 

But before you dive into the 100 Days of Arduino, let's cover some critical best practices that separate beginners from experts.

**[Next Chapter: Best Practices ->](./Chapter_10_Best_Practices.md)**

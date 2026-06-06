# Chapter 3: Setting Up Your Environment

You have your Arduino board, and a USB cable. Now what? You need a way to write code and send it to the board. 

For this, we use the **Arduino IDE** (Integrated Development Environment).

## Step 1: Download the Arduino IDE

![Arduino IDE](https://upload.wikimedia.org/wikipedia/commons/6/60/Arduino_IDE_2.0.0.png)

1. Go to the official Arduino Software page: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Download the **Arduino IDE 2.x** for your operating system (Windows, Mac, or Linux). 
3. Run the installer and follow the default prompts.

> [!TIP]
> **Why IDE 2.x?**
> The older Arduino IDE 1.8 is still available and very stable, but the 2.x version introduces modern features like autocompletion (IntelliSense) and a dark mode, which makes writing code much easier!

## Step 2: Connect your Arduino

Plug your Arduino Uno into your computer using the USB cable. 
You should see a green LED on the board light up (indicating it has power). You might also see an orange LED blinking.

## Step 3: Dealing with Drivers (CH340)

If you bought a genuine Arduino board, it should be recognized by your computer instantly.
However, if you bought a cheap clone from Amazon/AliExpress (which is totally fine!), it likely uses a different USB-to-Serial chip called the **CH340**.

**If your computer does not recognize the Arduino:**
You will need to download and install the CH340 driver. 
1. Search online for "CH340 Driver Download" (SparkFun has a great guide and safe download links for this).
2. Install the driver and restart your computer.

## Step 4: Configure the IDE

Open the Arduino IDE. Before you can upload code, you must tell the IDE **what** board you are using and **where** it is plugged in.

1. **Select the Board:** In the top menu bar, there is a dropdown. Click it and click "Select other board and port...". Type "Arduino Uno" in the board search bar and select it.
2. **Select the Port:** In the same menu, select the COM port (Windows) or `/dev/cu.usbserial` (Mac) that your Arduino is plugged into.
   - *How do I know which port is my Arduino?* If you see `COM1`, that is usually your computer's internal hardware. Look for a higher number like `COM3`, `COM4`, or `COM7`. If you aren't sure, unplug the Arduino, see which COM port disappears from the list, plug it back in, and select that one!

## Step 5: Your First Code - Blink

It is tradition in programming that your first program is called "Hello World." In the hardware world, the equivalent is making an LED blink.

The Arduino IDE comes with built-in examples. Let's load the blink example!

1. Go to **File > Examples > 01.Basics > Blink**.
2. A new window will open with some code.
3. Click the **Upload** button (the arrow pointing right `→` at the top left of the IDE).

### What is happening?
1. The IDE "Compiles" the code (translates human-readable C++ into machine code the microcontroller can understand).
2. The TX and RX LEDs on your Arduino board will flash rapidly. This means the code is being transferred over the USB cable.
3. The IDE will say "Done uploading."

**Look at your board!** The built-in LED (next to pin 13) should now be turning on for one second, and off for one second, in a continuous loop.

Congratulations! You have just programmed a microcontroller.

---

Before we start modifying that code, we need to understand a tiny bit of electronics so we don't accidentally blow anything up.

**[Next Chapter: Basic Electronics Crash Course ->](./Chapter_04_Basic_Electronics_Crash_Course.md)**

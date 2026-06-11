# Chapter 06: Troubleshooting

Even the most experienced engineers encounter bugs, compilation failures, and hardware issues. When you are working with microcontrollers, problems usually fall into one of three categories: **Code**, **Hardware/Wiring**, or **The Toolchain (USB/Drivers)**.

This guide covers the most common errors you will see during your 100-day journey and how to solve them permanently.

---

## 1. Upload Errors: `avrdude: stk500_getsync()`

This is the most notorious error in the Arduino ecosystem. It means the Arduino IDE (or `arduino-cli`) is trying to talk to the board over USB, but the board isn't responding.

### The Fix:
1. **Check your COM Port:** Go to **Tools > Port**. Make sure a port is selected. If no ports appear, you likely have a driver issue (see section 2).
2. **Check the Board Type:** Go to **Tools > Board** and ensure you have the correct board selected. An Arduino Nano requires `Arduino Nano`, and sometimes requires selecting the **Old Bootloader** under **Tools > Processor**.
3. **Unplug Pins 0 and 1:** Pins 0 (RX) and 1 (TX) are used for USB communication. If you have anything wired to these pins (like a Bluetooth module or GPS), the upload will fail. **Unplug them, upload the code, and plug them back in.**
4. **Bad Cable:** Some micro-USB cables are "charge-only" and do not have data lines. Try a different cable.

---

## 2. Missing COM Ports / CH340 Driver Issues

If you plug in your Arduino and absolutely nothing shows up in the Port menu, your computer doesn't recognize the USB-to-Serial chip on the board.

Most cheap, third-party Arduino clones use the **CH340** serial chip instead of the standard FTDI chip. Windows and Mac do not always have this driver pre-installed.

### The Fix:
- **Windows:** Download and install the [CH340 Driver](https://sparks.gogo.co.nz/ch340.html). Restart your IDE.
- **Mac:** Install the CH340 driver for macOS. On modern macOS versions, this is often handled automatically, but you may need to allow the extension in System Settings -> Privacy & Security.

---

## 3. Compilation Error: `No such file or directory`

You will see this error if your code tries to `#include <LibraryName.h>`, but the Arduino IDE cannot find it.

```cpp
fatal error: Adafruit_GFX.h: No such file or directory
```

### The Fix:
1. Open the Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries...** (or press `Ctrl+Shift+I`).
3. Search for the exact name of the missing library (e.g., `Adafruit GFX`).
4. Click **Install**.
5. Compile again.

*Note: If you are using our Devcontainer/Codespaces environment, all libraries are pre-installed automatically!*

---

## 4. I2C Devices Not Responding

You wired up an LCD or an OLED screen, but it's completely blank, or the code hangs at `Wire.begin()`.

### The Fix:
1. **Check the Wiring:** I2C requires SDA and SCL. On an Arduino Uno/Nano, **SDA is A4** and **SCL is A5**. Do not mix them up!
2. **Check the Address:** Not all I2C devices use the same address. Your LCD might be `0x27` or `0x3F`. Your OLED might be `0x3C` or `0x3D`.
3. **Run an I2C Scanner:** If you aren't sure, run an I2C Scanner sketch (like the one we build in Day 24). It will ping every address and print the correct one to the Serial Monitor.

---

## 5. Gibberish in the Serial Monitor

If you open the Serial Monitor and see reverse question marks (``) or random characters, you have a baud rate mismatch.

### The Fix:
Look at your `setup()` function. If you wrote `Serial.begin(9600);`, you must ensure the dropdown menu in the bottom-right corner of the Serial Monitor is also set to **9600 baud**. If it's set to 115200, you will see gibberish.

---

## Keep Going!
Hardware debugging can be frustrating, but every error you solve makes you a better engineer. If you get stuck, take a break, double-check your jumper wires, and read the error message carefully!

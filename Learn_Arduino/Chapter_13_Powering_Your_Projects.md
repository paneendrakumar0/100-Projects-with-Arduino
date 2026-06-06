# Chapter 13: Powering Your Projects

When you are just blinking an LED, the power provided by your computer's USB port is plenty. But what happens when you want to run 4 motors, a bright LCD screen, and a heavy-duty servo? Your Arduino will instantly shut down or reset, or worse, your computer's USB port might shut off to protect itself!

Understanding how to safely power your Arduino and its components is critical.

## 1. How much power can an Arduino provide?

The Arduino is a **brain**, not a **battery**. 

The 5V pin on the Arduino Uno can safely supply roughly **400mA to 500mA** of current when plugged into USB.
- A standard LED uses ~20mA. (Safe!)
- A tiny micro-servo uses ~200mA. (Safe, barely!)
- A DC motor can pull 500mA to 1000mA+ when it starts spinning. (**DANGEROUS!**)

If you pull more current than the Arduino can provide, the voltage will drop, the Arduino will reset, and the 5V voltage regulator on the board might physically overheat and permanently break.

## 2. The 3 Ways to Power an Arduino

### Method A: The USB Cable (5 Volts)
The easiest way. Plug it into your computer or a standard USB phone charger.
**Best for:** Programming, debugging, and simple sensor circuits.

### Method B: The Barrel Jack (7 to 12 Volts)
You can plug a wall adapter or a battery pack (like a 9V battery or a 6xAA battery pack) into the round black port on the Uno.
The Arduino has a built-in "Voltage Regulator" that converts this higher voltage down to the 5V the brain needs.
**Warning:** Do not supply more than 12V. The regulator will get too hot and fry the board!

### Method C: The VIN Pin (7 to 12 Volts)
The VIN (Voltage In) pin does the exact same thing as the Barrel Jack. It connects directly to the voltage regulator. You can connect the positive wire of a 9V battery to VIN, and the negative wire to GND.

> ⚠️ **CRITICAL WARNING:** NEVER connect a 9V or 12V battery directly to the `5V` pin. The `5V` pin bypasses the voltage regulator. If you put 9V into the 5V pin, your Arduino will instantly die in a puff of smoke.

## 3. Powering Heavy Components (Motors, Strips of LEDs)

When you need to run heavy components, you must **separate the power supplies**.

1. Power the Arduino using USB or a 9V battery.
2. Power the motors using a completely separate battery pack (e.g., a 4xAA battery pack providing 6V).
3. **The Most Important Rule in Electronics:** You MUST connect the grounds together! The GND pin of the Arduino must be connected to the negative terminal of the motor's battery pack. If the grounds are not connected, the signals will not make sense and the circuit will not work.

### Example Wiring for a Heavy Load:
- **Arduino GND** connected to **Battery GND**
- **Motor Driver GND** connected to **Battery GND**
- **Motor Driver VCC** connected to **Battery Positive (+)**
- **Arduino 5V** connected to *Nothing* (Let the Arduino run on its own USB power)

By following these rules, your Arduino will stay safe, cool, and happy!

---

[<-- Back to Main Guide](./README.md)

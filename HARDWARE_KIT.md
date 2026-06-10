# Hardware Kit Guide

You do not need every part on day one. Build the kit in layers so you can learn safely, avoid waste, and understand each new module as it appears in the curriculum.

## Tier 1: First Circuits

Use this for the beginner guide and Days 1-10.

- Arduino Uno, Nano, or compatible board
- USB cable that supports data, not charge-only
- Breadboard and jumper wires
- 220 ohm and 10k ohm resistors
- LEDs, RGB LED, pushbuttons, potentiometer
- Active buzzer
- LDR/photoresistor
- Basic sensor kit with DHT11, PIR, flame sensor, water level sensor, and soil moisture sensor

## Tier 2: Displays and Human Interfaces

Use this for Days 11-32.

- SG90 or similar hobby servo
- Joystick module
- 4x4 membrane keypad
- Relay module
- 16x2 I2C LCD
- SSD1306 OLED display
- 7-segment display and 4-digit 7-segment module
- IR receiver and remote
- DS3231 RTC module
- MFRC522 RFID module
- KY-040 rotary encoder

## Tier 3: Motion and Robotics

Use this for motor, robot, and control projects.

- DC motors with wheels
- L298N or TB6612FNG motor driver
- 28BYJ-48 stepper and ULN2003 driver
- NEMA 17 stepper and A4988 driver
- Wheel encoders or motor encoder kit
- HC-SR04 ultrasonic sensor
- IR line sensor array
- Robot chassis, battery holder, switch, and mounting hardware

## Tier 4: Advanced Systems

Use this for communication, logging, timing, power, and autonomy days.

- HC-05 Bluetooth module
- nRF24L01 modules with capacitor or adapter board
- RS485 transceiver
- MCP2515 CAN module
- SD card module
- SPI flash memory module
- MPU6050 IMU
- GPS module such as NEO-6M
- Compass module such as QMC5883L
- INA219 current sensor
- MAX7219 LED matrix

## Power and Safety Essentials

- Use a multimeter early. It will save hours of debugging.
- Keep motor power separate from logic power when motors cause resets.
- Tie grounds together when separate supplies need to share signals.
- Add current-limiting resistors for LEDs.
- Avoid powering motors from the Arduino 5V pin.
- Check module voltage levels before connecting 5V boards to 3.3V devices.
- Disconnect USB power before rewiring a circuit.

## Nice-to-Have Tools

- Wire stripper and flush cutter
- Small screwdriver set
- Dupont crimp kit
- Soldering iron and solder
- Heat shrink tubing
- Logic analyzer for I2C, SPI, UART, and timing issues
- Bench power supply with current limiting

## Buying Strategy

Start with a basic Arduino starter kit, then add parts only when a project needs them. For robotics days, buy a chassis kit with motors, wheels, battery holder, and hardware together. For communication modules, buy two of each radio or bus module so you can test both ends of the link.

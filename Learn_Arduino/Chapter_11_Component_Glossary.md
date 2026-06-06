# Chapter 11: Component Visual Glossary

The 100 Days of Arduino challenge uses dozens of different sensors, modules, and actuators. 
Before you start the challenge, familiarize yourself with these components so that when you read "Day 14: Membrane Keypad", you have an exact picture in your mind of what you are working with!

## Sensors (Inputs)

### 1. PIR Motion Sensor
![PIR Sensor](https://upload.wikimedia.org/wikipedia/commons/d/d5/PIR_Sensor_Visonic_Next%2BMCW.jpg)
Detects motion by measuring changes in the infrared levels emitted by surrounding objects (like humans). Used in burglar alarms and automated lighting.

### 2. IR Receiver (Infrared)
![IR Receiver](https://upload.wikimedia.org/wikipedia/commons/e/e3/IR_receiver_sensor.jpg)
Receives infrared signals from standard TV remotes. Used to decode remote control button presses.

### 3. Rotary Encoder
![Rotary Encoder](https://upload.wikimedia.org/wikipedia/commons/b/b3/Rotary_Encoder.jpg)
A knob that you can spin endlessly in either direction. It outputs pulses that tell the Arduino which way it is turning. Often has a built-in push button.

### 4. Joystick Module
![Joystick Module](https://upload.wikimedia.org/wikipedia/commons/6/6f/Thumbstick_module.jpg)
Similar to a PlayStation or Xbox controller thumbstick. It contains two potentiometers (for X and Y axis) and a push button.

### 5. Ultrasonic Sensor (HC-SR04)
![Ultrasonic Sensor](./images/Ultrasonic.jpg)
Measures distance by sending out a burst of ultrasonic sound and timing how long the echo takes to return. Looks like two little silver "eyes".

### 6. MPU6050 (IMU)
![MPU6050](https://upload.wikimedia.org/wikipedia/commons/4/4b/MPU6050_Module.jpg)
An Inertial Measurement Unit. It contains a 3-axis accelerometer and a 3-axis gyroscope. Used to measure tilt, rotation, and balance for drones and self-balancing robots.

### 7. GPS Module (NEO-6M)
![GPS Module](https://upload.wikimedia.org/wikipedia/commons/3/3d/NEO-6M_GPS_module.jpg)
Receives signals from satellites to determine exact latitude, longitude, and global time.

## Actuators (Outputs)

### 8. Servo Motor
![Servo Motor](https://upload.wikimedia.org/wikipedia/commons/d/d2/Servomotor_01.jpg)
A specialized motor that can accurately rotate to a specific angle (usually between 0 and 180 degrees). Used for robotic arms and steering.

### 9. DC Motor
![DC Motor](https://upload.wikimedia.org/wikipedia/commons/e/ea/DC_motor_with_gearbox.jpg)
A standard motor that spins continuously when power is applied. Reversing the polarity reverses the direction.

### 10. Stepper Motor
![Stepper Motor](https://upload.wikimedia.org/wikipedia/commons/2/22/Stepper_motor_with_driver.jpg)
A motor that moves in discrete, precise "steps" rather than spinning continuously. Excellent for 3D printers and precise machinery.

### 11. Piezo Buzzer
![Buzzer](https://upload.wikimedia.org/wikipedia/commons/e/e4/Cjam-piezo-buzzer.png)
Converts electrical signals into sound. Active buzzers beep when power is applied, while Passive buzzers require the Arduino to send specific frequencies to play melodies.

### 12. Relay Module
![Relay Module](https://upload.wikimedia.org/wikipedia/commons/thumb/3/38/Delta_Electronics_DPS-350FB_A_-_board_1_-_OEG_SDT-SS-112M_-_case_removed-3045.jpg/960px-Delta_Electronics_DPS-350FB_A_-_board_1_-_OEG_SDT-SS-112M_-_case_removed-3045.jpg)
An electrically operated switch. It allows your 5V Arduino to safely turn on and off high-voltage appliances (like a 120V/240V lamp or a heavy pump).

## Displays

### 13. 16x2 LCD Display
![16x2 LCD](https://upload.wikimedia.org/wikipedia/commons/9/90/MELT_16x2_LCD_alphanumeric_display_07%28DXO%29.jpg)
A simple, classic screen that can display 2 rows of 16 text characters.

### 14. OLED Display (SSD1306)
![OLED Display](https://upload.wikimedia.org/wikipedia/commons/thumb/d/df/OLED_Displays.jpg/960px-OLED_Displays.jpg)
A tiny, high-contrast, pixel-perfect screen capable of rendering smooth graphics, animations, and custom fonts.

### 15. Seven-Segment Display
![Seven Segment](https://upload.wikimedia.org/wikipedia/commons/thumb/d/d1/Aurora_electronic_calculator_DT210_10.jpg/960px-Aurora_electronic_calculator_DT210_10.jpg)
Used for digital clocks and counters. It consists of 7 LEDs arranged in a figure-8 pattern to display numbers.

## Modules & Communication

### 16. Membrane Keypad (4x4)
![Membrane Keypad](https://upload.wikimedia.org/wikipedia/commons/f/fe/IBM_Screen_Reader_Pad_membrane_exposed_through_absent_keys.jpg)
A flat 16-button matrix used for password locks and calculator interfaces.

### 17. L298N Motor Driver
![L298N Motor Driver](https://upload.wikimedia.org/wikipedia/commons/d/df/L298N_Motor_Driver.jpg)
An H-Bridge module that sits between the Arduino and DC motors, providing them with the heavy power they need to move.

### 18. Bluetooth Module (HC-05)
![Bluetooth Module](https://upload.wikimedia.org/wikipedia/commons/9/9e/Bluetooth_HC-05.jpg)
Allows your Arduino to communicate wirelessly with a smartphone or laptop.

### 19. RFID Reader (MFRC522)
![RFID Reader](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c3/Rfid-reader%28portal%29.JPG/960px-Rfid-reader%28portal%29.JPG)
Reads wireless ID cards and keyfobs, like the ones used to unlock office doors or hotel rooms.

### 20. MicroSD Card Module
![MicroSD Module](https://upload.wikimedia.org/wikipedia/commons/f/f3/MicroSD_module.jpg)
Allows the Arduino to save vast amounts of data (like temperature logs or GPS paths) to a standard SD card.

---

[<-- Back to Main Guide](./README.md)

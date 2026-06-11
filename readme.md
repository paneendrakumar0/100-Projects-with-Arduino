# 100 Days of Arduino

[![Arduino CI Compilation](https://github.com/paneendrakumar0/100-Projects-with-Arduino/actions/workflows/arduino-build.yml/badge.svg)](https://github.com/paneendrakumar0/100-Projects-with-Arduino/actions/workflows/arduino-build.yml)
[![Next.js Documentation Deploy](https://github.com/paneendrakumar0/100-Projects-with-Arduino/actions/workflows/nextjs.yml/badge.svg)](https://github.com/paneendrakumar0/100-Projects-with-Arduino/actions/workflows/nextjs.yml)

> ⭐ **If you find these projects helpful, please consider leaving a star! It helps the repository grow.**

![Arduino Uno](Learn_Arduino/images/Arduino_Uno.jpg)

An end-to-end Arduino curriculum that grows from first circuits to autonomous robotics, control systems, embedded architecture, and ROS-style integration.

This repository is designed as a practical masterclass: every day has a focused project, working Arduino code, wiring guidance, and a concept trail that explains why the build works. The path starts with safe beginner fundamentals, then moves deliberately into non-blocking programming, sensors, motors, displays, wireless communication, control loops, signal processing, low-power design, and robotics.

## Table of Contents

- [Start Here](#start-here)
- [What Makes This Guide Different](#what-makes-this-guide-different)
- [Repository Structure](#repository-structure)
- [Hardware You Will Use](#hardware-you-will-use)
- [The Journey Log](#the-journey-log)
- [Contributing](#contributing)
- [Safety Note](#safety-note)

## Start Here

- New to Arduino, electronics, or C++? Start with the [Learn Arduino guide](./Learn_Arduino/README.md).
- Want the deeper engineering model behind the projects? Read the [Arduino Core Theory Masterclass](./ARDUINO_CORE_THEORY.md).
- Building a parts kit? Use the [Hardware Kit Guide](./HARDWARE_KIT.md).
- Ready to build? Begin with [Day 1: The Non-Blocking Blink](./Day_01_Millis_Blink).

## What Makes This Guide Different

- **A real learning path:** the projects are ordered to compound skills instead of feeling like disconnected examples.
- **Non-blocking mindset from day one:** the curriculum moves beyond `delay()` early and teaches `millis()`, state machines, timers, and interrupts.
- **Hardware plus software:** each day connects circuit behavior, embedded C/C++, and debugging habits.
- **Beginner to advanced coverage:** the path moves from LEDs and buttons to PID control, sensor fusion, odometry, pure pursuit, and ROS serial bridging.
- **Cloneable structure:** each project is isolated in its own folder with source code and documentation.

## Repository Structure

```text
.
|-- Learn_Arduino/                 # prerequisite zero-to-hero guide
|-- ARDUINO_CORE_THEORY.md          # deeper embedded systems companion
|-- Day_01_Millis_Blink/            # one folder per project
|   |-- Day_01_Millis_Blink.ino
|   `-- README.md
|-- ...
|-- Day_100_ROS_Serial_Bridge/
|-- docs-website/                   # interactive Next.js documentation site
`-- scripts/                        # helper scripts for maintainers
```

## Hardware You Will Use

You can complete the early projects with an Arduino Uno or Nano, breadboard, jumper wires, LEDs, resistors, buttons, a potentiometer, a buzzer, and a few common sensors. Later projects introduce displays, motor drivers, wireless modules, IMUs, storage, encoders, and robotics hardware.

Useful parts across the full curriculum:

- Arduino Uno, Nano, Mega, Leonardo/Micro, or compatible boards
- Breadboard, jumper wires, resistors, LEDs, RGB LED, buttons, buzzer
- Potentiometer, LDR, PIR, flame sensor, soil moisture sensor, water level sensor
- HC-SR04 ultrasonic sensor, DHT11/DHT22, DS18B20, MPU6050, GPS, compass
- 16x2 LCD, OLED SSD1306, 7-segment displays, MAX7219 matrix
- Servo motors, DC motors, steppers, L298N, ULN2003, A4988
- RFID, IR remote, Bluetooth, nRF24L01, RS485, CAN, SD card module

## The Journey Log

### Prerequisite

- [Learn Arduino: Zero to Hero Guide](./Learn_Arduino/README.md)
- [Arduino Core Theory Masterclass](./ARDUINO_CORE_THEORY.md)

### Phase 1: Fundamentals

- [x] [Day 1: The Non-Blocking Blink using `millis()`](./Day_01_Millis_Blink)
- [x] [Day 2: SOS Morse Code Generator](./Day_02_SOS_Morse)
- [x] [Day 3: Pushbutton State Toggle and Debouncing](./Day_03_Pushbutton_Toggle)
- [x] [Day 4: Potentiometer to LED Fade](./Day_04_Potentiometer_Fade)
- [x] [Day 5: RGB LED Color Mixer](./Day_05_RGB_Mixer)
- [x] [Day 6: LDR Automatic Night Light](./Day_06_LDR_Night_Light)
- [x] [Day 7: Ultrasonic Distance Measurer](./Day_07_Ultrasonic_Distance)
- [x] [Day 8: PIR Motion Sensor Alarm](./Day_08_PIR_Alarm)
- [x] [Day 9: DHT11 Temperature and Humidity Logger](./Day_09_DHT11_Logger)
- [x] [Day 10: Active Buzzer Melody Player](./Day_10_Buzzer_Melody)
- [x] [Day 11: Servo Motor Sweep](./Day_11_Servo_Sweep)
- [x] [Day 12: Potentiometer-Controlled Servo](./Day_12_Pot_Servo)
- [x] [Day 13: Joystick Module Value Mapper](./Day_13_Joystick_Mapper)
- [x] [Day 14: 4x4 Keypad Interfacing](./Day_14_Keypad_Interfacing)
- [x] [Day 15: Password Door Lock Logic](./Day_15_Password_Door_Lock)
- [x] [Day 16: Relay Module Control](./Day_16_Relay_Control)
- [x] [Day 17: Sound Sensor Clap Switch](./Day_17_Sound_Clap_Switch)
- [x] [Day 18: Water Level Detection](./Day_18_Water_Level)
- [x] [Day 19: Soil Moisture Monitor](./Day_19_Soil_Moisture)
- [x] [Day 20: Flame Sensor Fire Alarm](./Day_20_Flame_Sensor)

### Phase 2: Displays, Storage, and Interfaces

- [x] [Day 21: 16x2 LCD Hello World](./Day_21_LCD_HelloWorld)
- [x] [Day 22: LCD Stopwatch](./Day_22_LCD_Stopwatch)
- [x] [Day 23: LCD Marquee](./Day_23_LCD_Marquee)
- [x] [Day 24: 7-Segment Counter](./Day_24_7Seg_Counter)
- [x] [Day 25: 4-Digit 7-Segment Clock](./Day_25_4Digit_7Seg_Clock)
- [x] [Day 26: OLED Graphics](./Day_26_OLED_Graphics)
- [x] [Day 27: OLED Animation](./Day_27_OLED_Animation)
- [x] [Day 28: IR Remote Decoder](./Day_28_IR_Decoder)
- [x] [Day 29: DS3231 RTC Clock](./Day_29_RTC_Clock)
- [x] [Day 30: EEPROM Logger](./Day_30_EEPROM_Logger)

### Phase 3: Motion, Communication, and Robotics

- [x] [Day 31: RFID Reader](./Day_31_RFID_Reader)
- [x] [Day 32: Rotary Encoder](./Day_32_Rotary_Encoder)
- [x] [Day 33: DC Motor with L298N](./Day_33_DC_Motor_L298N)
- [x] [Day 34: Stepper Motor with ULN2003](./Day_34_Stepper_ULN2003)
- [x] [Day 35: A4988 Stepper Driver](./Day_35_Stepper_A4988)
- [x] [Day 36: DC Motor Encoder Feedback](./Day_36_Motor_Encoder)
- [x] [Day 37: PID Speed Control](./Day_37_PID_Speed_Control)
- [x] [Day 38: Ultrasonic Radar](./Day_38_Ultrasonic_Radar)
- [x] [Day 39: HC-05 Bluetooth Control](./Day_39_Bluetooth_Control)
- [x] [Day 40: nRF24L01 Wireless](./Day_40_nRF24L01_Wireless)
- [x] [Day 41: MPU6050 IMU](./Day_41_MPU6050_IMU)
- [x] [Day 42: Complementary Filter](./Day_42_Complementary_Filter)
- [x] [Day 43: Obstacle-Avoidance Robot](./Day_43_Obstacle_Avoidance_Robot)
- [x] [Day 44: 2-Sensor Line Follower](./Day_44_Line_Follower_2Sensor)
- [x] [Day 45: Calibrated IR Line Array](./Day_45_Line_Array_Calibrated)
- [x] [Day 46: PD Line Follower](./Day_46_PD_Line_Follower)
- [x] [Day 47: Self-Balancing Robot Controller](./Day_47_Self_Balancing_Robot)
- [x] [Day 48: Maze Solver](./Day_48_Maze_Solver)
- [x] [Day 49: GPS Decoder](./Day_49_GPS_Decoder)
- [x] [Day 50: Compass Heading Lock](./Day_50_Compass_Heading_Lock)

### Phase 4: Advanced Embedded Systems

- [x] [Day 51: Modbus RS485](./Day_51_Modbus_RS485)
- [x] [Day 52: CAN Bus](./Day_52_CAN_Bus)
- [x] [Day 53: SPI Flash Logger](./Day_53_SPI_Flash_Logger)
- [x] [Day 54: WS2812B Bit-Bang Driver](./Day_54_WS2812B_Bitbang)
- [x] [Day 55: Parallel LCD](./Day_55_Parallel_LCD)
- [x] [Day 56: Spatial Positioner](./Day_56_Spatial_Positioner)
- [x] [Day 57: FFT Spectrum Analyzer](./Day_57_FFT_Spectrum)
- [x] [Day 58: Timer1 PWM](./Day_58_Timer1_PWM)
- [x] [Day 59: Voltage Monitor](./Day_59_Voltage_Monitor)
- [x] [Day 60: OLED Oscilloscope](./Day_60_OLED_Oscilloscope)
- [x] [Day 61: Inverse Kinematics](./Day_61_Inverse_Kinematics)
- [x] [Day 62: Task Scheduler](./Day_62_Task_Scheduler)
- [x] [Day 63: DS18B20 1-Wire](./Day_63_DS18B20_1Wire)
- [x] [Day 64: Capacitive Touch](./Day_64_Capacitive_Touch)
- [x] [Day 65: EEPROM Motion Recorder](./Day_65_EEPROM_Motion_Recorder)
- [x] [Day 66: MAX7219 LED Matrix](./Day_66_MAX7219_LED_Matrix)
- [x] [Day 67: Hall RPM Tachometer](./Day_67_Hall_RPM_Tachometer)
- [x] [Day 68: Stepper Microstepping](./Day_68_Stepper_Microstepping)
- [x] [Day 69: DHT22 Custom Protocol](./Day_69_DHT22_Custom_Protocol)
- [x] [Day 70: RFID Logging](./Day_70_RFID_Logging)
- [x] [Day 71: SD Card Writing](./Day_71_SD_Card_Writing)
- [x] [Day 72: CSV Datalogger](./Day_72_CSV_Datalogger)

### Phase 5: Autonomy and Systems Integration

- [x] [Day 73: MPU6050 Raw Registers](./Day_73_MPU6050_Raw)
- [x] [Day 74: MPU6050 Gyro Angles](./Day_74_MPU6050_Gyro)
- [x] [Day 75: MPU6050 Pitch and Roll](./Day_75_MPU6050_Pitch_Roll)
- [x] [Day 76: Fall Detection](./Day_76_Fall_Detection)
- [x] [Day 77: Self-Balancing Logic](./Day_77_Self_Balancing)
- [x] [Day 78: GPS Parser](./Day_78_GPS_Parser)
- [x] [Day 79: GPS Distance](./Day_79_GPS_Distance)
- [x] [Day 80: I2C Network](./Day_80_I2C_Network)
- [x] [Day 81: FreeRTOS Blink](./Day_81_FreeRTOS_Blink)
- [x] [Day 82: FreeRTOS Sensor Tasking](./Day_82_FreeRTOS_Sensor)
- [x] [Day 83: Watchdog Timer](./Day_83_Watchdog_Timer)
- [x] [Day 84: Deep Sleep](./Day_84_Deep_Sleep)
- [x] [Day 85: EEPROM Read/Write](./Day_85_EEPROM_ReadWrite)
- [x] [Day 86: Calibration EEPROM](./Day_86_Calibration_EEPROM)
- [x] [Day 87: Custom Arduino Library](./Day_87_Custom_Library)
- [x] [Day 88: Hardware Interrupts](./Day_88_Hardware_Interrupts)
- [x] [Day 89: Timer Interrupts](./Day_89_Timer_Interrupts)
- [x] [Day 90: HID Leonardo](./Day_90_HID_Leonardo)
- [x] [Day 91: HID Volume Knob](./Day_91_HID_Volume_Knob)
- [x] [Day 92: HID Macro Pad](./Day_92_HID_Macro_Pad)
- [x] [Day 93: Sim Racing Pedals](./Day_93_Sim_Racing_Pedals)
- [x] [Day 94: Serial Parser](./Day_94_Serial_Parser)
- [x] [Day 95: Robotic Arm IK](./Day_95_Robotic_Arm_IK)
- [x] [Day 96: Differential Drive Odometry](./Day_96_Differential_Drive_Odometry)
- [x] [Day 97: Stepper S-Curve](./Day_97_Stepper_S_Curve)
- [x] [Day 98: Kalman Filter](./Day_98_Kalman_Filter)
- [x] [Day 99: Pure Pursuit Path Tracking](./Day_99_Pure_Pursuit_Path_Tracking)
- [x] [Day 100: ROS Serial Bridge](./Day_100_ROS_Serial_Bridge)

## Contributing

Pull requests are welcome for clearer explanations, safer wiring notes, bug fixes, schematics, diagrams, and board-specific improvements. Keep changes focused and include the affected day or guide section in the commit message.

See [CONTRIBUTING.md](./CONTRIBUTING.md) for project structure, code style, documentation style, and verification expectations.

## Safety Note

Double-check wiring before powering a circuit. Use resistors with LEDs, avoid shorting power rails, keep motor power separate from logic power when needed, and never work on high-voltage AC projects without proper training and isolation.

If this guide helps you, share what you build!

---
> ⭐ **Did you enjoy this journey? Please consider leaving a star! It helps others find these projects and grow the community.**

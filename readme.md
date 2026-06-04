# 🚀 100 Days of Arduino: From Basics to Autonomous Systems

Welcome to the **100 Days of Arduino** challenge!

I'm Paneendra, an engineering student and robotics enthusiast with a heavy focus on autonomy, mechatronics, and control systems. I created this repository as a massive, open-source masterclass. Whether you are a beginner looking to understand basic electronics or an advanced builder working towards ROS integration and kinematic models, this repository maps out that journey day by day.

## 🎯 The Goal

To build a highly structured, easily cloneable, and scalable foundation of embedded systems knowledge. We move past basic `delay()` functions very quickly and dive into non-blocking code, state machines, sensor fusion, and hardware interrupts.

## 📁 Repository Structure

Every day has its own dedicated folder containing:

- The `.ino` source code file.
- A `README.md` explaining the core concepts, logic, and exact wiring diagrams.

## 🛠️ Hardware Requirements

To follow along with the entire 100 days, you will need an Arduino Uno/Mega, a standard sensor kit (ultrasonic, IR, PIR, DHT11, MPU6050), various displays (LCD, OLED), motor drivers (L298N, A4988), and communication modules (NRF24L01, Bluetooth).

## 📅 The Journey Log

### Phase 1: The Fundamentals

- [x] [Day 1: The Non-Blocking Blink (Using `millis()`)](./Day_01_Millis_Blink)
- [x] [Day 2: SOS Morse Code Generator](./Day_02_SOS_Morse)
- [x] [Day 3: Pushbutton State Toggle (Debouncing)](./Day_03_Pushbutton_Toggle)
- [x] [Day 4: Potentiometer to LED Fade (Analog In/Out)](./Day_04_Potentiometer_Fade)
- [x] [Day 5: RGB LED Color Mixer](./Day_05_RGB_Mixer)
- [x] [Day 6: Photoresistor (LDR) Automatic Night Light](./Day_06_LDR_Night_Light)
- [x] [Day 7: Ultrasonic Sensor Distance Measurer](./Day_07_Ultrasonic_Distance)
- [x] [Day 8: PIR Motion Sensor Alarm](./Day_08_PIR_Alarm)
- [x] [Day 9: Temperature & Humidity Logger (DHT11)](./Day_09_DHT11_Logger)
- [x] [Day 10: Active Buzzer Melody Player](./Day_10_Buzzer_Melody)
- [x] [Day 11: Servo Motor Sweep](./Day_11_Servo_Sweep)
- [x] [Day 12: Potentiometer-Controlled Servo](./Day_12_Pot_Servo)
- [x] [Day 13: Joystick Module Value Mapper](./Day_13_Joystick_Mapper)
- [x] [Day 14: Membrane Keypad Interfacing (4x4)](./Day_14_Keypad_Interfacing)
- [x] [Day 15: Password Door Lock Logic (Keypad + Servo)](./Day_15_Password_Door_Lock)
- [x] [Day 16: Relay Module Control (High Power switching)](./Day_16_Relay_Control)
- [x] [Day 17: Sound Sensor Clap Switch](./Day_17_Sound_Clap_Switch)
- [x] [Day 18: Water Level Detection](./Day_18_Water_Level)
- [x] [Day 19: Soil Moisture Monitor](./Day_19_Soil_Moisture)
- [x] [Day 20: Flame Sensor Fire Alarm System](./Day_20_Flame_Sensor)
- [x] [Day 21: 16x2 LCD Hello World (I2C)](./Day_21_LCD_HelloWorld)
- [x] [Day 22: LCD Stopwatch](./Day_22_LCD_Stopwatch)
- [x] [Day 23: Scrolling Text Marquee on LCD](./Day_23_LCD_Marquee)
- [x] [Day 24: 7-Segment Display Counter](./Day_24_7Seg_Counter)
- [x] [Day 25: 4-Digit 7-Segment Clock Basic](./Day_25_4Digit_7Seg_Clock)
- [x] [Day 26: OLED Display (SSD1306) Graphics](./Day_26_OLED_Graphics)
- [x] [Day 27: OLED Bouncing Ball Animation](./Day_27_OLED_Animation)
- [x] [Day 28: Infrared (IR) Remote Decoder](./Day_28_IR_Decoder)
- [x] [Day 29: DS3231 Real-Time Clock (RTC)](./Day_29_RTC_Clock)
- [x] [Day 30: EEPROM Data Logger (Wear-Leveling)](./Day_30_EEPROM_Logger)
- [x] [Day 31: MFRC522 RFID Card Reader (SPI)](./Day_31_RFID_Reader)
- [x] [Day 32: Quadrature Rotary Encoder (KY-040)](./Day_32_Rotary_Encoder)
- [x] [Day 33: DC Motor Speed & Direction Control (L298N H-Bridge)](./Day_33_DC_Motor_L298N)
- [x] [Day 34: Stepper Motor Control (ULN2003 / 28BYJ-48)](./Day_34_Stepper_ULN2003)
- [x] [Day 35: Bipolar Stepper Driver (A4988 / NEMA 17)](./Day_35_Stepper_A4988)
- [x] [Day 36: DC Motor Encoder Feedback (RPM Calc)](./Day_36_Motor_Encoder)
- [x] [Day 37: Closed-Loop PID Speed Control](./Day_37_PID_Speed_Control)
- [x] [Day 38: Ultrasonic Radar Sweep (Servo + Range Finder)](./Day_38_Ultrasonic_Radar)
- [x] [Day 39: HC-05 Bluetooth Module (UART Control)](./Day_39_Bluetooth_Control)
- [x] [Day 40: nRF24L01+ 2.4GHz Transceiver (SPI Packet RF)](./Day_40_nRF24L01_Wireless)
- [x] [Day 41: MPU6050 6-Axis IMU (I2C Gyro/Accelerometer)](./Day_41_MPU6050_IMU)
- [x] [Day 42: IMU Sensor Fusion via Complementary Filter](./Day_42_Complementary_Filter)
- [x] [Day 43: Autonomous Obstacle-Avoidance Robot](./Day_43_Obstacle_Avoidance_Robot)
- [x] [Day 44: Basic Line-Following Robot (2-Sensor Differential)](./Day_44_Line_Follower_2Sensor)
- [x] [Day 45: Calibrated 5-Sensor IR Array (Centroid Math)](./Day_45_Line_Array_Calibrated)
- [x] [Day 46: PD-Controlled Line Follower](./Day_46_PD_Line_Follower)
- [x] [Day 47: Self-Balancing Robot Controller (Stabilization Loop)](./Day_47_Self_Balancing_Robot)
- [x] [Day 48: Maze Solving Robot (Left-Hand Rule & Path Optimization)](./Day_48_Maze_Solver)
- [x] [Day 49: GPS Telemetry Decoder (Raw NMEA Parser)](./Day_49_GPS_Decoder)
- [x] [Day 50: Digital Compass Heading Lock (HMC5883L / QMC5883L I2C)](./Day_50_Compass_Heading_Lock)
- [x] [Day 51: Industrial Modbus RTU Slave Node (RS485 Communication)](./Day_51_Modbus_RS485)
- [x] [Day 52: CAN Bus Controller (MCP2515 SPI Node)](./Day_52_CAN_Bus)
- [x] [Day 53: SPI Flash Memory Logger (Winbond W25QXX Direct Registers)](./Day_53_SPI_Flash_Logger)
- [x] [Day 54: WS2812B Addressable RGB LED Strip (Bit-Banged Assembly Timings)](./Day_54_WS2812B_Bitbang)
- [x] [Day 55: Parallel HD44780 LCD (Direct 8-Bit GPIO Interface)](./Day_55_Parallel_LCD)
- [x] [Day 56: Ultrasonic 2D Spatial Positioning Scanner (Trilateration Math)](./Day_56_Spatial_Positioner)
- [x] [Day 57: FFT Audio Spectrum Analyzer (ADC Interrupted DSP)](./Day_57_FFT_Spectrum)
- [x] [Day 58: Hardware Timer1 PWM Generator (Direct AVR Register Config)](./Day_58_Timer1_PWM)
- [x] [Day 59: Precision Voltage & Current Monitor (INA219 + ADC Resistor Divider)](./Day_59_Voltage_Monitor)
- [x] [Day 60: OLED Oscilloscope (Real-Time Waveform on SSD1306 128×64)](./Day_60_OLED_Oscilloscope)
- [x] [Day 61: Robotic Arm Inverse Kinematics (2-DOF Geometric IK Solver)](./Day_61_Inverse_Kinematics)
- [x] [Day 62: Cooperative Task Scheduler (Round-Robin Kernel from Scratch)](./Day_62_Task_Scheduler)
- [x] [Day 63: DS18B20 1-Wire Temperature Sensor (1-Wire Protocol from Scratch)](./Day_63_DS18B20_1Wire)
- [x] [Day 64: Capacitive Touch Sensor (RC Charge-Time Measurement)](./Day_64_Capacitive_Touch)
- [x] [Day 65: EEPROM Servo Motion Recorder & Playback (Non-Volatile Motion Capture)](./Day_65_EEPROM_Motion_Recorder)
- [x] [Day 66: MAX7219 8×8 LED Matrix Driver (SPI Register Control)](./Day_66_MAX7219_LED_Matrix)
- [x] [Day 67: Hall Effect RPM Tachometer (Timer1 Input Capture Unit)](./Day_67_Hall_RPM_Tachometer)
- [x] [Day 68: Stepper Motor Microstepping (A4988 Driver — All Step Resolutions)](./Day_68_Stepper_Microstepping)
- [x] [Day 69: DHT22 Humidity & Temperature Sensor (Custom Single-Wire Bit-Bang Parser)](./Day_69_DHT22_Custom_Protocol)
- [x] [Day 70: RFID Access Control Logging (MFRC522 + EEPROM Wear-Leveling Log + DS3231 RTC)](./Day_70_RFID_Logging)
- [x] [Day 71: SD Card Module File Writing (SPI Datalogger Basics)](./Day_71_SD_Card_Writing)
      _(More days will be linked here as they are completed)_

---

_Feel free to star ⭐ this repository if you find it helpful, fork it to track your own progress, or submit PRs if you find optimizations!_
"# 100-Projects-with-Arduino" 

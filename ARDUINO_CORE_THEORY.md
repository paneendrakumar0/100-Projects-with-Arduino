# Arduino Core Theory Masterclass

Welcome to the ultimate Arduino Computer Science and Electrical Engineering companion!

While the 100 projects in this repository will teach you *how* to wire components and write specific scripts, this guide exists to teach you *why* those scripts work at a silicon level. If you master the concepts in this document, you won't just be an "Arduino Hobbyist" - you will be a capable Embedded Systems Engineer.

---


## Component Visuals

<p align="center">
  <img src="assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="assets/images/components/DC_Motor.jpg" alt="DC Motor" width="200" style="margin:10px;" />
  <img src="assets/images/components/EEPROM.jpg" alt="EEPROM" width="200" style="margin:10px;" />
  <img src="assets/images/components/Kalman_Filter.jpg" alt="Kalman Filter" width="200" style="margin:10px;" />
  <img src="assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
  <img src="assets/images/components/Rotary_Encoder.jpg" alt="Rotary Encoder" width="200" style="margin:10px;" />
</p>

## 1. Microcontroller Architecture (The Silicon Level)

An Arduino is not a computer like your laptop; it is a **Microcontroller Unit (MCU)**. Your laptop has a separate CPU, RAM chip, and Hard Drive. A microcontroller crams the CPU, memory, and storage onto a single, microscopic piece of silicon.

### AVR vs. ARM Architecture
- **AVR (Arduino Uno / Mega / Nano):** These use an 8-bit architecture. This means the CPU processes data in chunks of 8 bits (1 byte) at a time. The maximum number it can natively hold in a register is 255. If you do math with large numbers (like `long`), the compiler secretly breaks it down into multiple 8-bit instructions, which is very slow.
- **ARM (Teensy / ESP32 / Arduino Due):** These use 32-bit architecture. They can process huge numbers in a single clock cycle, making them astronomically faster for robotics math (like Kinematics or Kalman Filters).

### The System Clock
An Arduino Uno has a silver oval crystal oscillator on the board. This crystal vibrates physically exactly **16,000,000 times per second (16 MHz)**. Every time it vibrates, the CPU executes one machine code instruction. While 16 MHz sounds slow compared to a 4 GHz PC, remember there is no operating system overhead! The Arduino executes your code with microsecond deterministic precision.

---

## 2. Memory Management & The Danger of `String`

Memory is the number one cause of crashes in embedded systems. You must protect it with your life.

### The Three Types of Memory
1. **Flash Memory (Storage):** Where your compiled code lives. On a Uno, you have 32 KB. It is non-volatile (survives power loss).
2. **SRAM (Working Memory):** Where your variables live. You only have **2 KB (2,048 bytes)** on a Uno!
3. **EEPROM (Long-term Data):** A tiny 1 KB drive for saving settings (like calibration data).

### The Stack vs. The Heap
When you declare a normal variable (`int x = 5;`), it goes on the **Stack**. The Stack is perfectly organized. When a function finishes, the memory is instantly freed. 

When you use the `String` class (e.g., `String msg = "Hello";`) or `malloc()`, it goes on the **Heap**. 

> [!WARNING]
> **Never use the `String` class on an 8-bit Arduino.**
> 
> Because `String` objects can grow and shrink dynamically, they leave "holes" of empty space in your 2 KB of SRAM over time. This is called **Heap Fragmentation**. Eventually, the holes are too small to fit new data, your RAM hits 100%, and the robot fatally crashes and reboots mid-operation!
> 
> **The Fix:** Always use static C-Style `char` arrays instead:
> `char msg[20] = "Hello";`

---

## 3. Concurrency: Breaking Free from `delay()`

The biggest mistake beginners make is using `delay(1000)`. 

When you call `delay()`, you are telling the 16 MHz CPU to execute an empty `while` loop 16 million times, doing absolutely nothing else. If a robot is `delay()`-ing to blink an LED, it cannot read a crash-sensor, meaning it will drive straight into a wall.

### Cooperative Multitasking with `millis()`
The `millis()` function returns the number of milliseconds since the board powered on. We use it to check the time *without* pausing the CPU.

```cpp
// Non-Blocking Example
unsigned long previousTime = 0;

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - previousTime >= 1000) {
    previousTime = currentTime;
    toggleLED();
  }
  
  readCrashSensors(); // This now runs instantly, thousands of times a second!
}
```

### Finite State Machines (FSMs)
An FSM is a mathematically rigorous way to program logic. The robot can only be in one State at a time (e.g., `IDLE`, `DRIVING`, `AVOIDING_OBSTACLE`). You use `enum` and `switch(state)` to hop between these states safely.

### Real-Time Operating Systems (RTOS)
For advanced boards (ESP32), you use FreeRTOS. Instead of one big `loop()`, you create separate **Tasks**. The RTOS Scheduler acts like a traffic cop, pausing and resuming tasks seamlessly at a microscopic level, giving you true parallel threading.

---

## 4. Hardware Interrupts and Timers

Sometimes, polling `millis()` isn't fast enough. If a rotary encoder spins 1,000 times a second, you will miss ticks if your `loop()` is busy doing math.

### Interrupt Service Routines (ISRs)
An **Interrupt** is an asynchronous hardware signal. When a physical pin changes state, it physically shocks the CPU. 
1. The CPU instantly freezes whatever it was doing in `loop()`.
2. It jumps to a special function called an ISR.
3. It executes the ISR, and then resumes the `loop()` exactly where it left off.

> [!IMPORTANT]
> **The Golden Rules of ISRs:**
> 1. Keep them extremely short (no math, no `Serial.print`). Just update a variable and exit.
> 2. Any variable modified inside an ISR *must* be declared as `volatile` (e.g., `volatile int count = 0;`). This prevents the compiler from accidentally deleting it during optimization.

### Hardware Timers
The ATMega328P has three independent countdown clocks inside the silicon (Timer0, Timer1, Timer2). They run in the background, entirely detached from the CPU core!
- **Timer0 (8-bit):** Used by Arduino to drive `millis()`. Never touch this unless you want time to break.
- **Timer1 (16-bit):** The most powerful timer. You can set it to "CTC Mode" to trigger a perfect interrupt at exactly 10.000 Hz, immune to `loop()` lag.

---

## 5. Communication Protocols (The Big Three)

When microcontrollers talk to sensors, they use one of three main hardware protocols.

### 1. UART / Serial (The Workhorse)
- **Pins:** TX, RX.
- **How it works:** Asynchronous (no clock line). Both sides must agree on the speed (Baud Rate) beforehand.
- **Use case:** PC communication, Bluetooth, GPS.

### 2. I2C / Wire (The Network)
- **Pins:** SDA (Data), SCL (Clock).
- **How it works:** Synchronous. Uses a bus architecture where up to 127 sensors can share the exact same two wires! It uses Open-Drain logic with Pull-Up resistors.
- **Use case:** Accelerometers, OLED screens, short-distance sensors.

### 3. SPI (The Speed Demon)
- **Pins:** MISO, MOSI, SCK, CS.
- **How it works:** Synchronous, full-duplex. Extremely fast (can reach 8 MHz). Instead of an address, it uses a physical Chip Select (CS) wire to wake up the sensor.
- **Use case:** SD Cards, Ethernet, high-resolution screens.

---

## 6. Analog Electronics in a Digital World

A microcontroller only understands binary (`0` or `5V`). How does it deal with analog voltages like `3.3V`?

### The ADC (Analog-to-Digital Converter)
The ADC physically measures the voltage and maps it to a number. On the Uno, it is 10-bit, meaning it has $2^{10} = 1024$ steps. 
`5V / 1024 = 0.0048V`. Therefore, the Arduino can detect changes as tiny as 4.8 millivolts!

### PWM (Pulse Width Modulation)
`analogWrite()` does *not* output an analog voltage! If you write `127` (50%), the pin doesn't output 2.5V. Instead, it rapidly flashes 5V on and off 500 times a second. Because DC motors and LEDs react slowly, they physically average out the flashing into what *appears* to be half power.

### Signal Filtering
Sensors are inherently noisy due to electromagnetic interference.
- **EMA (Exponential Moving Average):** A cheap, 1-line mathematical filter that smooths out spikes by trusting past data over new data.
- **Kalman Filter:** An advanced algorithm that uses matrices to fuse two sensors (like an Accelerometer and a Gyro) to achieve absolute truth.

---

## 7. Power Management & Robustness

Battery-powered systems and industrial robots must survive in harsh environments.

### The Watchdog Timer (WDT)
If radiation or a coding bug causes your robot to freeze in an infinite `while` loop, you lose the robot.
The WDT is an independent hardware timer. You arm it to (for example) 2 seconds. You must "feed the dog" (`wdt_reset()`) in your main loop. If the loop crashes and fails to feed the dog, the timer hits zero, and physically pulls the reset pin, automatically rebooting your crashed robot!

### Deep Sleep
The 16 MHz clock burns ~15mA of current. In Deep Sleep, we physically turn off the clock and internal silicon peripherals (ADC, Brown-out Detectors). The power draw drops to `< 1 uA`, allowing a battery to last for years. The only way to wake up the frozen CPU is via a Hardware Interrupt.

---
*Happy Engineering!*

import os

content_day81 = """
## 🧠 Code Explanation

Let's break down how a Real-Time Operating System manages parallel execution on a single core:

### 1. Task Creation and Prioritization
```cpp
xTaskCreate(TaskBlink1, "Blink1", 100, NULL, 1, NULL);
```
- Standard Arduino programs execute sequentially in `loop()`. If one function takes too long or uses `delay()`, everything else freezes.
- Here, we create discrete Tasks. We assign them a memory stack size (100 words = 200 bytes) and a priority.
- If a low-priority task is running, and a high-priority task needs to run, FreeRTOS instantly pauses the low-priority task mid-execution, runs the critical task, and then resumes the original task perfectly!

### 2. Time Slicing via the Scheduler
```cpp
vTaskDelay(500 / portTICK_PERIOD_MS);
```
- In standard Arduino, `delay(500)` forces the CPU to burn cycles in a busy-wait loop doing absolutely nothing.
- In FreeRTOS, `vTaskDelay()` tells the RTOS Scheduler: "I don't need the CPU for 500ms."
- The Scheduler immediately marks this task as "Blocked" and hands the CPU to the next available task. The CPU is never idle, allowing true cooperative multitasking!
"""
with open("Day_81_FreeRTOS_Blink/README.md", "a", encoding="utf-8") as f:
    f.write(content_day81)

content_day82 = """
## 🧠 Code Explanation

Let's break down how we securely transfer data between parallel threads:

### 1. The Danger of Shared Variables
- If Task A (Producer) and Task B (Consumer) both read/write to a standard global `int` at the exact same microsecond, the data gets corrupted. This is called a "Race Condition."

### 2. Thread-Safe FreeRTOS Queues
```cpp
sensorQueue = xQueueCreate(5, sizeof(int));
```
- A Queue is a protected chunk of memory (a First-In-First-Out buffer). FreeRTOS guarantees that only one Task can push or pull from it at a time.
- If the Producer pushes 5 integers and the Queue fills up, `xQueueSend` fails gracefully instead of overwriting memory.

### 3. Blocking on Data
```cpp
xQueueReceive(sensorQueue, &receivedValue, portMAX_DELAY);
```
- The Consumer task calls `xQueueReceive()`. If there is no data in the Queue, it doesn't spin in a loop checking again and again. 
- By passing `portMAX_DELAY`, the RTOS Scheduler instantly puts the Consumer task to sleep! It won't wake up until the Producer pushes new data, saving massive amounts of CPU processing power.
"""
with open("Day_82_FreeRTOS_Sensor/README.md", "a", encoding="utf-8") as f:
    f.write(content_day82)

content_day83 = """
## 🧠 Code Explanation

Let's break down how we build self-recovering, crash-proof firmware:

### 1. Hardware Watchdog Overview
- Deep inside the silicon of the ATmega328P is a totally independent countdown timer running on its own dedicated 128 kHz oscillator. Even if the main CPU crystal breaks or the software freezes completely, this timer keeps ticking.

### 2. Arming and "Feeding the Dog"
```cpp
wdt_enable(WDTO_2S);
// Inside loop:
wdt_reset();
```
- We arm the Watchdog with a 2-second timeout.
- Inside our `loop()`, we call `wdt_reset()`. This "feeds the dog" by pushing the countdown back to 2 seconds.
- If our code gets stuck in an infinite loop (which we simulate by pressing 'k'), `wdt_reset()` is never called. The countdown hits zero, and the Watchdog physically pulls the CPU reset pin, instantly rebooting the crashed system!

### 3. Forensic Crash Diagnostics
```cpp
byte resetSource = MCUSR;
```
- How do we know *why* the robot rebooted? Did someone unplug it, or did it crash?
- The MCU Status Register (`MCUSR`) sets a specific bit in hardware based on what triggered the reboot. We read this on boot to detect if a Watchdog Timeout (`WDRF`), Power-Outage (`PORF`), or external reset button (`EXTRF`) caused the restart.
"""
with open("Day_83_Watchdog_Timer/README.md", "a", encoding="utf-8") as f:
    f.write(content_day83)

content_day84 = """
## 🧠 Code Explanation

Let's break down how we drastically reduce power consumption for battery operations:

### 1. Putting the CPU to Sleep
```cpp
set_sleep_mode(SLEEP_MODE_PWR_DOWN);
sleep_cpu();
```
- A standard `delay(1000)` doesn't save power; the CPU is still running at 16 MHz executing empty math loops, burning ~15mA.
- `sleep_cpu()` physically halts the 16 MHz crystal oscillator! The CPU completely freezes on that line of code. By using `SLEEP_MODE_PWR_DOWN`, the chip's current draw drops from 15,000 µA down to less than 1 µA!

### 2. Disabling Power-Hungry Peripherals
```cpp
ADCSRA &= ~(1 << ADEN); // Turn off ADC
```
- Even if the CPU clock is halted, the Analog-to-Digital Converter uses active silicon comparators that burn ~250 µA in the background. We manipulate the `ADCSRA` register to physically disconnect power to the ADC before sleeping.

### 3. Waking Up via Hardware Interrupts
```cpp
attachInterrupt(digitalPinToInterrupt(WAKE_BUTTON_PIN), wakeUpISR, LOW);
```
- Because the CPU is frozen, it cannot check `digitalRead()`.
- We map an external button to a hardware interrupt (INT0). When the voltage drops LOW, it triggers an asynchronous logic gate deep in the silicon that instantly restarts the main 16 MHz clock, executes the ISR, and resumes our code right after `sleep_cpu()`!
"""
with open("Day_84_Deep_Sleep/README.md", "a", encoding="utf-8") as f:
    f.write(content_day84)

content_day85 = """
## 🧠 Code Explanation

Let's break down how we store complex configurations directly to non-volatile memory:

### 1. Writing Full Structs to EEPROM
```cpp
EEPROM.put(EEPROM_START_ADDR, activeConfig);
```
- Instead of manually writing variables byte-by-byte (and tracking memory addresses by hand), we pack our variables into a C++ `struct`.
- The `EEPROM.put()` method uses pointers and `sizeof()` to serialize the entire struct into a contiguous block of bytes and writes it to EEPROM. It uses "write gating" internally, meaning it checks the memory first and only writes bytes that have actually changed, saving the silicon from wear-and-tear!

### 2. Data Integrity and Checksums
```cpp
uint16_t calculatedCheck = calculateChecksum(activeConfig);
if (activeConfig.checksum == calculatedCheck) { ... }
```
- EEPROM memory can become corrupted due to power-loss during a write, or you might read from a brand-new chip that holds random garbage data (`0xFF`).
- Before saving, we add up the raw byte values of our struct to create a "checksum". 
- When we load the data on boot, we re-calculate the checksum. If it doesn't match the stored checksum, we know the memory is corrupted, and we automatically wipe it and load safe Factory Defaults!
"""
with open("Day_85_EEPROM_ReadWrite/README.md", "a", encoding="utf-8") as f:
    f.write(content_day85)

content_day86 = """
## 🧠 Code Explanation

Let's break down how an industrial IMU Calibration Manager works:

### 1. The Need for Calibration
- Due to manufacturing tolerances and thermal stress, MEMS gyroscopes and accelerometers have a tiny physical bias. Even sitting perfectly still on a desk, a gyro might read $+0.5^\circ/\text{s}$ rotation. Over time, that error adds up and causes robotic drift.
- Calibration fixes this by measuring the sensor 500 times while perfectly still, averaging the results, and saving that exact "offset" value.

### 2. Persistent Storage with Magic Bytes
```cpp
calData.magicByte = 0xC4;
EEPROM.put(0, calData);
```
- Doing a 2-second calibration on every boot is annoying and assumes the robot isn't moving when you turn it on (impossible for drones in the air).
- We run calibration *once* (triggered by a button press) and save the offsets to the EEPROM.
- To ensure we don't load garbage memory, we write a "Magic Byte" (`0xC4`) to the EEPROM. On boot, if the EEPROM doesn't contain `0xC4`, the system knows the sensor has never been calibrated!

### 3. Applying the Offsets
```cpp
float gxCal = (gx - calData.gyroOffsetX) / GYRO_SCALE_FACTOR;
```
- During real-time operation, we read the raw hardware integer (`gx`), subtract the stored bias offset (`calData.gyroOffsetX`), and *then* divide by the scale factor to get a perfectly true $0.0^\circ/\text{s}$ reading!
"""
with open("Day_86_Calibration_EEPROM/README.md", "a", encoding="utf-8") as f:
    f.write(content_day86)

content_day87 = """
## 🧠 Code Explanation

Let's break down how we implement and package an Exponential Moving Average (EMA) filter:

### 1. EMA Filter Math vs. Simple Averages
- A Simple Moving Average requires a large array (e.g., 50 variables) to store past data, burning through the Arduino's tiny SRAM.
- The **Exponential Moving Average** ($y[n] = \alpha \cdot x[n] + (1 - \alpha) \cdot y[n-1]$) requires only *one* variable of memory: the previous result!

### 2. The Alpha Coefficient (Smoothing vs. Lag)
```cpp
float filteredSignal = myFilter.filter(rawSignal);
```
- The `alpha` value (between 0.0 and 1.0) controls the filter's aggression.
- **Low Alpha (e.g., 0.1):** We only trust 10% of the new raw sensor reading, and rely 90% on our historical smoothed data. This destroys high-frequency noise but creates a delayed response (lag).
- **High Alpha (e.g., 0.8):** We trust 80% of the new reading. This tracks fast movements perfectly but lets a lot of noise pass through.

### 3. Encapsulation into a C++ Class
- We abstracted the math into `EMAFilter.h` and `EMAFilter.cpp`. 
- By using a C++ Class, we encapsulate the memory state (`_lastOutput`). This means we can instantiate ten different `EMAFilter` objects for ten different sensors without their math or memory overlapping!
"""
with open("Day_87_Custom_Library/README.md", "a", encoding="utf-8") as f:
    f.write(content_day87)

content_day88 = """
## 🧠 Code Explanation

Let's break down how Hardware Interrupts respond instantly to physical events:

### 1. Bypassing the Main Loop
```cpp
attachInterrupt(digitalPinToInterrupt(2), buttonISR, FALLING);
```
- If you use `digitalRead()` inside `loop()`, you might miss a button press if the Arduino is busy doing math or executing a `delay()`.
- An Interrupt connects directly to the CPU core. When Pin 2 drops from HIGH to LOW (`FALLING`), the CPU literally freezes whatever it was doing in `loop()`, jumps instantly to the `buttonISR()` function, executes it, and goes right back to where it left off. This is mandatory for Emergency Stop (E-Stop) buttons!

### 2. The Mechanical Bounce Problem
- When two pieces of metal touch inside a physical button, they act like a tiny diving board, bouncing on a microscopic level. The logic pin goes `HIGH-LOW-HIGH-LOW` in a span of 2 milliseconds.
- Because hardware interrupts are so fast, the Arduino will register 1 physical press as 5 to 10 separate triggers!

### 3. Software Debouncing inside the ISR
```cpp
if (currentTime - lastInterruptTime > DEBOUNCE_DELAY_MS) {
    debouncedTriggerCount++;
    lastInterruptTime = currentTime;
}
```
- Inside the ISR, we compare the current timestamp to the last time we processed a trigger. If it's been less than 150ms, we mathematically ignore the trigger because we know it's a physical metal bounce. We only register "true" presses!
"""
with open("Day_88_Hardware_Interrupts/README.md", "a", encoding="utf-8") as f:
    f.write(content_day88)

content_day89 = """
## 🧠 Code Explanation

Let's break down how to guarantee mathematically perfect sampling rates using Timers:

### 1. The Problem with millis()
- If you poll a sensor using `if (millis() - lastTime > 10)`, the actual timing might be 10ms, 11ms, or 15ms depending on what `Serial.print()` or other code is doing. This timing "jitter" ruins advanced math like Fast Fourier Transforms (FFT) or PID Controllers.

### 2. Configuring Timer1 for CTC Mode
```cpp
OCR1A = 19999;
TCCR1B |= (1 << WGM12) | (1 << CS11);
```
- The ATmega328P has a 16-bit hardware counter (Timer1) driven by the 16 MHz crystal.
- We set a Prescaler of 8, slowing the timer to 2 MHz. We then set the Compare Match Register (`OCR1A`) to 19,999.
- In **Clear Timer on Compare Match (CTC)** mode, the timer counts up to 19,999, instantly resets to 0, and triggers an interrupt. 
- $2,000,000 / 20,000 = 100\text{ Hz}$. We have achieved mathematically flawless 10.000 millisecond ticks!

### 3. The Timer ISR
```cpp
ISR(TIMER1_COMPA_vect) {
  sampledValue = analogRead(A0);
  newSampleAvailable = true;
}
```
- Every 10ms, no matter what `loop()` is doing, the CPU halts and executes this ISR. It samples the ADC precisely on time, raises a boolean flag, and returns. `loop()` handles the slow serial printing only when the flag is true.
"""
with open("Day_89_Timer_Interrupts/README.md", "a", encoding="utf-8") as f:
    f.write(content_day89)

content_day90 = """
## 🧠 Code Explanation

Let's break down how we emulate Human Interface Devices (HID):

### 1. Hardware Requirements (ATmega32U4)
```cpp
#if defined(USBCON)
  #include <Keyboard.h>
```
- Standard Arduino Unos (ATmega328P) cannot do this natively because they use a separate USB-to-Serial chip. 
- Boards like the Leonardo, Micro, or Pro Micro use the **ATmega32U4**, which has a direct hardware USB transceiver. When plugged into a PC, it doesn't just show up as a COM port; it can enumerate as a physical USB Keyboard and Mouse, completely bypassing OS security!

### 2. Mouse Jiggling
```cpp
Mouse.move(5, 0, 0); // Move X +5 pixels
```
- Emulating a mouse is as simple as sending relative X and Y coordinate changes. Our script sends a +5 pixel move, waits 100ms, and sends a -5 pixel move back. The PC registers physical mouse activity, preventing sleep modes or screen locks!

### 3. Keyboard Shortcuts and Macros
```cpp
Keyboard.press(KEY_LEFT_ALT);
Keyboard.press(KEY_TAB);
Keyboard.releaseAll();
```
- The `Keyboard` library allows us to spoof key presses. We can type whole strings automatically (`Keyboard.print()`) or send complex modifier shortcuts.
- **Safety Warning:** If you write a loop that hits `ENTER` repeatedly with no delay, you will lock yourself out of your computer and be unable to upload new code! Always use `releaseAll()` and implement physical safety buttons.
"""
with open("Day_90_HID_Leonardo/README.md", "a", encoding="utf-8") as f:
    f.write(content_day90)

import os

content_day61 = """
## 🧠 Code Explanation

Let's break down how we use Inverse Kinematics (IK) to control a robotic arm:

### 1. The Math: Law of Cosines
```cpp
float cos_theta2 = (d_sq - L1 * L1 - L2 * L2) / (2.0f * L1 * L2);
float theta2_rad = acos(cos_theta2);
```
- In Forward Kinematics, we know the joint angles and want to find the hand position. Inverse Kinematics is the opposite: we know the hand position (from the joystick) and must calculate the required joint angles!
- We use the Law of Cosines on the triangle formed by the upper arm (`L1`), the forearm (`L2`), and the straight-line distance (`d`) from the shoulder to the target.
- By isolating `theta2`, we discover exactly how much the elbow needs to bend to place the wrist at the correct distance from the shoulder.

### 2. Workspace Constraints
```cpp
float d_min = fabsf(L1 - L2) + 0.01f;
float d_max = L1 + L2 - 0.01f;
if (d < d_min) d = d_min;
if (d > d_max) d = d_max;
```
- A physical robot arm can't reach everywhere. If you push the joystick too far away, it can't stretch longer than `L1 + L2` (fully extended). If you pull it too close, it can't fold tighter than `L1 - L2`.
- Attempting to calculate angles outside this "donut-shaped" workspace results in imaginary numbers (mathematical singularities), which crash the Arduino.
- We "clamp" the target distance `d` so the arm simply stretches as far as it can and stops smoothly at its physical boundary.
"""
with open("Day_61_Inverse_Kinematics/README.md", "a", encoding="utf-8") as f:
    f.write(content_day61)

content_day62 = """
## 🧠 Code Explanation

Let's break down how we built a Cooperative RTOS Kernel:

### 1. The Task Control Block (TCB)
```cpp
struct Task {
  const char* name;
  TaskFn      taskFn;
  uint16_t    periodMs;
  unsigned long lastRunMs;
  // ...
};
```
- Every operating system tracks programs using a Task Control Block. Our simple array holds all the metadata for our 5 functions.
- Instead of using `delay()` (which paralyzes the entire CPU), we give each task a `periodMs`. The scheduler promises to call the function pointer (`taskFn`) only when enough time has elapsed since `lastRunMs`.

### 2. The Super-Loop Dispatcher
```cpp
for (int i = 0; i < NUM_TASKS; i++) {
  if (now - tasks[i].lastRunMs >= tasks[i].periodMs) {
    tasks[i].taskFn();
    tasks[i].lastRunMs  = now;
  }
}
```
- The main `loop()` does nothing but rapidly scan through the task array. 
- Because this is a **Cooperative** scheduler, the system relies on trust. When `taskFn()` is called, the task *must* do its job quickly and return. If a task accidentally runs a `while(true)` loop, the entire OS freezes because the dispatcher never gets control back!
"""
with open("Day_62_Task_Scheduler/README.md", "a", encoding="utf-8") as f:
    f.write(content_day62)

content_day63 = """
## 🧠 Code Explanation

Let's break down how we bit-banged the Dallas 1-Wire Protocol:

### 1. Microsecond Timing Slots
```cpp
void ow_write_bit(uint8_t bit) {
  ow_low();
  if (bit) {
    delayMicroseconds(6);   // Short pull for '1'
    ow_release();
    delayMicroseconds(64);
  } else {
    delayMicroseconds(70);  // Long pull for '0'
    ow_release();
  }
}
```
- The DS18B20 only has one data wire. To communicate, both devices must agree on strict timing windows (Time Slots).
- A Time Slot is about 70 microseconds long. The Master (Arduino) starts every slot by pulling the line LOW.
- If we want to send a `1`, we quickly release the line after 6µs. The sensor looks at the line 15µs in, sees it is HIGH, and records a `1`.
- If we want to send a `0`, we hold the line LOW for the entire 70µs. The sensor looks at 15µs, sees it is LOW, and records a `0`.

### 2. CRC-8 Integrity Checking
```cpp
uint8_t mix = (crc ^ byte) & 0x01;
crc >>= 1;
if (mix) crc ^= 0x8C; // Reflected polynomial 0x31
```
- Because 1-Wire is susceptible to electrical noise over long cables, the DS18B20 computes a mathematical hash (Cyclic Redundancy Check) of its temperature data and sends it as the 9th byte.
- We run the exact same polynomial division algorithm (`x^8 + x^5 + x^4 + 1`) on the first 8 bytes we receive.
- If our calculated hash matches the 9th byte sent by the sensor, we are 100% mathematically certain that the temperature reading is flawless and uncorrupted!
"""
with open("Day_63_DS18B20_1Wire/README.md", "a", encoding="utf-8") as f:
    f.write(content_day63)

content_day64 = """
## 🧠 Code Explanation

Let's break down how we measured physical capacitance without any special chips:

### 1. The RC Time Constant
```cpp
digitalWrite(SEND_PIN, HIGH);
int count = 0;
while (digitalRead(SENSE_PIN) == LOW && count < MAX_CYCLES) {
  count++;
}
```
- Capacitors take time to fill up with electricity. The time it takes is directly proportional to how big the capacitor is.
- By connecting a massive 1-Megohm resistor between `SEND` and `SENSE`, we slow down the charging process of the physical wire.
- We pull `SEND` HIGH, and then rapidly count in a `while` loop until `SENSE` registers a HIGH voltage (2.5V). 
- If the wire is untouched, it charges very fast (low count). When a human touches the wire, the human body acts as a massive capacitor, soaking up the electricity and slowing down the voltage rise (high count)!

### 2. Dynamic Calibration
```cpp
baseline = calSum / CALIBRATE_READS;
long delta = reading - baseline;
bool isTouched = (delta > TOUCH_THRESHOLD);
```
- Humidity, wire length, and nearby electronics change the baseline capacitance of the room.
- To prevent false triggers, the Arduino takes 50 readings at boot to establish what "normal" looks like.
- In the main loop, we only trigger a touch if the current reading spikes drastically (`delta`) above that dynamic baseline!
"""
with open("Day_64_Capacitive_Touch/README.md", "a", encoding="utf-8") as f:
    f.write(content_day64)

content_day65 = """
## 🧠 Code Explanation

Let's break down how we record physical motion to Non-Volatile Memory:

### 1. Real-Time EEPROM Streaming
```cpp
if (millis() - lastRecordTime >= RECORD_INTERVAL_MS) {
  EEPROM.write(EEPROM_DATA_ADDR + frameCount, angle);
  frameCount++;
}
```
- Standard Arduino variables are erased the moment power is lost. EEPROM is special flash memory that survives power-cycles.
- While the record button is held, we take a snapshot of the servo angle every 50 milliseconds (20 frames per second).
- We write this single byte directly to EEPROM. Since the ATmega328P has 1024 bytes of EEPROM, we can record about 50 seconds of motion before running out of tape!

### 2. The Magic Sentinel Byte
```cpp
const uint8_t EEPROM_MAGIC = 0xA5;
// ...
if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC) {
  // Valid recording found!
}
```
- How does the Arduino know if it has a valid recording saved after you unplug it and plug it back in months later?
- When we stop recording, we write a specific "Magic Number" (`0xA5`) to address 254. 
- On boot, the very first thing `setup()` does is check address 254. If it sees `0xA5`, it knows a human intentionally saved a sequence, and it is safe to play it back!
"""
with open("Day_65_EEPROM_Motion_Recorder/README.md", "a", encoding="utf-8") as f:
    f.write(content_day65)

content_day66 = """
## 🧠 Code Explanation

Let's break down how we manipulate an LED Matrix using direct SPI registers:

### 1. Addressing Hardware Registers over SPI
```cpp
void maxWrite(uint8_t reg, uint8_t data) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(CS_PIN, HIGH);
}
```
- The MAX7219 is an incredibly smart driver chip. Instead of the Arduino constantly flashing LEDs to multiplex them, we just send a 16-bit command to the MAX7219 and it handles the high-speed flashing autonomously!
- We pull Chip Select (CS) LOW, blast 8 bits for the Register Address (e.g., `0x0A` for Intensity), blast 8 bits for the Value (e.g., `0x04` for half brightness), and pull CS HIGH. The hardware instantly latches the new setting.

### 2. The Framebuffer and Bitwise Drawing
```cpp
if (colByte & (1 << row)) {
    pixel |= (0x80 >> col);
}
```
- We create an 8-byte array in Arduino RAM called `framebuffer`. Each byte represents one row of 8 LEDs on the physical matrix.
- To draw graphics (like scrolling text or bouncing balls), we use bitwise math (`&` and `|`) to flip individual bits inside the array `1` or `0`.
- Once our virtual drawing is complete, we run a `flushFramebuffer()` function that blasts the entire 8-byte array over SPI to the MAX7219 chip, updating the physical screen instantly!
"""
with open("Day_66_MAX7219_LED_Matrix/README.md", "a", encoding="utf-8") as f:
    f.write(content_day66)

content_day67 = """
## 🧠 Code Explanation

Let's break down how we measure extreme RPMs using Hardware Timers:

### 1. The Input Capture Unit (ICU)
```cpp
TCCR1B = _BV(ICNC1) | _BV(CS11); 
TIMSK1 = _BV(ICIE1);
```
- Reading a fast motor with `digitalRead()` or even standard `attachInterrupt()` introduces Software Jitter—tiny delays caused by the CPU doing other things. This ruins RPM calculations at high speeds.
- We bypass software entirely and use the ATmega328P's internal hardware: The **Input Capture Unit**.
- When the Hall Sensor detects a magnet, the hardware *instantly* takes a snapshot of the 16-bit `Timer1` clock and freezes it into the `ICR1` register. The CPU can then casually read this perfectly accurate timestamp later!

### 2. Handling Timer Overflows
```cpp
uint32_t periodTicks = ((uint32_t)oflows << 16) + (uint32_t)cap - (uint32_t)last;
```
- Timer1 runs so fast (2 MHz) that it resets to zero every 32 milliseconds.
- If the motor is spinning slowly (e.g., 60 RPM = 1 second per revolution), Timer1 will overflow dozens of times between magnet detections!
- We enable an Overflow Interrupt (`TIMER1_OVF_vect`) to count how many times the clock rolls over. By combining the 16-bit hardware timestamp (`cap`) with our 16-bit software rollover count (`oflows`), we create a massive 32-bit timestamp capable of tracking periods from microseconds all the way up to hours!
"""
with open("Day_67_Hall_RPM_Tachometer/README.md", "a", encoding="utf-8") as f:
    f.write(content_day67)

content_day68 = """
## 🧠 Code Explanation

Let's break down how we achieve silky smooth stepper motion:

### 1. Hardware Microstepping Control
```cpp
digitalWrite(MS1_PIN, HIGH);
digitalWrite(MS2_PIN, HIGH);
digitalWrite(MS3_PIN, HIGH); // 1/16th Step Mode
```
- A standard stepper motor has 200 physical "teeth" per revolution (1.8° per step). If you step them individually, the motor jumps and vibrates violently.
- By toggling the `MS1`, `MS2`, and `MS3` pins on the A4988 driver, we command its internal DACs (Digital-to-Analog Converters) to feed sinusoidal partial-currents to the coils.
- This creates magnetic "sub-steps" between the physical teeth. At 1/16th mode, our 200-step motor transforms into an ultra-smooth 3,200-step motor!

### 2. The Trapezoidal Velocity Profile
```cpp
if (i < accelSteps) {
  stepDelay = map(i, 0, accelSteps, START_DELAY_US, minDelayUs);
} else if (i > steps - accelSteps) {
  stepDelay = map(i, steps - accelSteps, steps, minDelayUs, START_DELAY_US);
}
```
- A stepper motor has zero torque at high speeds. If you instantly command it to spin at 1000 RPM, the magnetic field will spin faster than the physical rotor can accelerate, and the motor will screech and stall ("skipping steps").
- We use a Trapezoidal Profile: We start with a long `stepDelay` (slow speed), and linearly decrease it (accelerate) over the first 100 steps. 
- We cruise at maximum speed in the middle, and linearly increase the delay (decelerate) at the end. This prevents stalling and allows for massive top speeds!
"""
with open("Day_68_Stepper_Microstepping/README.md", "a", encoding="utf-8") as f:
    f.write(content_day68)

content_day69 = """
## 🧠 Code Explanation

Let's break down how we bit-bang the precise DHT22 digital protocol:

### 1. Handshake and Pulse Timing
```cpp
unsigned long pulseWidth = micros() - t;
uint8_t bit = (pulseWidth > BIT_THRESHOLD_US) ? 1 : 0;
```
- The DHT22 sends data by manipulating the length of the HIGH voltage pulses on a single wire.
- A HIGH pulse of 28µs means a logical `0`. A HIGH pulse of 70µs means a logical `1`.
- We use a `while(digitalRead() == HIGH)` loop to time the pulse using `micros()`. If the pulse was longer than our threshold (40µs), we shift a `1` into our byte array. Otherwise, we shift a `0`.

### 2. Bitwise Assembly and Sign Masking
```cpp
uint16_t rawTemp = ((uint16_t)(data[2] & 0x7F) << 8) | data[3];
tempC = (float)rawTemp / 10.0f;
if (data[2] & 0x80) tempC = -tempC;
```
- The sensor gives us Temperature split across two 8-bit bytes (`data[2]` and `data[3]`).
- The protocol dictates that the highest bit of `data[2]` (Bit 15) is the Sign bit. If it's a `1`, the temperature is negative (sub-zero).
- We use `& 0x7F` to strip away the Sign bit so it doesn't mess up our math, combine the remaining 15 bits into an integer, divide by 10 to get the decimal point, and then manually multiply by `-1` if the Sign bit was present!
"""
with open("Day_69_DHT22_Custom_Protocol/README.md", "a", encoding="utf-8") as f:
    f.write(content_day69)

content_day70 = """
## 🧠 Code Explanation

Let's break down how we built a secure EEPROM Access Logger with Wear-Leveling:

### 1. The EEPROM Ring Buffer
```cpp
int targetSlot = (activeSlotIndex + 1) % NUM_LOG_SLOTS;
int targetAddress = EEPROM_LOG_START + (targetSlot * LOG_ENTRY_SIZE);
```
- EEPROM memory degrades after 100,000 writes. If we always logged access events to Address 100, that specific memory cell would die very quickly!
- To prevent this, we divide the EEPROM into 46 "Slots" (20 bytes each). 
- We use the Modulo operator (`%`) to create a "Ring Buffer". Every time someone scans a card, we write to the *next* slot in the circle. When we hit slot 46, we loop back and overwrite slot 0. This spreads the wear evenly across the entire chip!

### 2. State-Machine Driven LED Animations
```cpp
if (indState == IND_ACCESS_DENIED) {
  int phase = elapsed / 200;
  digitalWrite(DENIED_LED, (phase % 2 == 0) ? HIGH : LOW);
}
```
- When access is denied, we want the red LED to flash rapidly 3 times. But if we use `delay(200)`, the CPU is frozen for a whole second, meaning a second person scanning their card would be completely ignored!
- We use a non-blocking State Machine. The main loop checks `millis()` and divides the elapsed time into 200ms "phases". 
- Depending on whether the phase number is even or odd (`% 2`), it turns the LED on or off. The CPU never stops executing the rest of the code!
"""
with open("Day_70_RFID_Logging/README.md", "a", encoding="utf-8") as f:
    f.write(content_day70)

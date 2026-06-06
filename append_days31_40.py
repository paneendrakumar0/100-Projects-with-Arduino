import os

content_day31 = """
## 🧠 Code Explanation

Let's break down how we read an RFID card securely:

### 1. Polling for Cards Non-Blockingly
```cpp
if (!mfrc522.PICC_IsNewCardPresent()) {
    return; // Fast escape if no card is present
}
if (!mfrc522.PICC_ReadCardSerial()) {
    return; // Exit if read failed
}
```
- We don't want the Arduino to freeze and wait forever for a card.
- `PICC_IsNewCardPresent()` quickly checks if a magnetic tag has entered the 13.56 MHz field. If not, the `return` statement instantly kicks us back to the top of `loop()`, allowing other robot tasks to run smoothly.
- If a card *is* there, `PICC_ReadCardSerial()` attempts to download its Unique ID.

### 2. Matching the Master Key
```cpp
if (mfrc522.uid.uidByte[i] != AUTHORIZED_UID[i]) {
    isAuthorized = false;
    break; 
}
```
- We loop through the 4 bytes of the scanned card's UID array.
- We compare each byte to our hardcoded `AUTHORIZED_UID` array.
- The `break` command is an optimization: if even the very first byte doesn't match, we instantly exit the `for` loop and deny access, rather than wasting CPU cycles checking the rest!
"""
with open("Day_31_RFID_Reader/README.md", "a", encoding="utf-8") as f:
    f.write(content_day31)

content_day32 = """
## 🧠 Code Explanation

Let's break down how to capture lightning-fast encoder movements using Hardware Interrupts:

### 1. Hardware Interrupts
```cpp
attachInterrupt(digitalPinToInterrupt(CLK_PIN), handleEncoderISR, FALLING);
```
- If we check the encoder pin inside `loop()`, we might miss a click if the Arduino is busy doing something else (like printing to the screen).
- `attachInterrupt()` wires Pin 2 directly to the core CPU. When the encoder pin transitions from HIGH to LOW (`FALLING`), the Arduino instantly freezes whatever it is doing, runs `handleEncoderISR()`, and then returns to normal. We never miss a click!

### 2. Software Lockout Debouncing
```cpp
if (currentTime - lastISRTime > 2000) {
    if (digitalRead(DT_PIN) == HIGH) { encoderPosition++; }
    else { encoderPosition--; }
    lastISRTime = currentTime;
}
```
- Inside the ISR, we check the current time in microseconds (`micros()`).
- Mechanical switches bounce and spark when turned, causing hundreds of false triggers. Our `> 2000` check creates a 2-millisecond "lockout window". Any bounce spikes that happen immediately after the first click are completely ignored!
- We then check the `DT_PIN` to determine Quadrature Direction (Clockwise vs Counter-Clockwise).
"""
with open("Day_32_Rotary_Encoder/README.md", "a", encoding="utf-8") as f:
    f.write(content_day32)

content_day33 = """
## 🧠 Code Explanation

Let's break down how to control DC motor speed and direction:

### 1. H-Bridge Direction Control
```cpp
void setDirectionForward() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
}
```
- The L298N has 4 internal transistors arranged in an "H" shape.
- Driving `IN1` HIGH and `IN2` LOW turns on the diagonal transistors, pushing current from left to right through the motor, spinning it forward.
- Swapping them (`IN1` LOW, `IN2` HIGH) pushes current from right to left, spinning the motor in reverse!
- Setting *both* LOW turns the motor off. Setting *both* HIGH creates a dead-short across the coils, forcefully applying an Electronic Brake!

### 2. PWM Speed Ramping
```cpp
motorSpeedPWM = (int)(progress * 255.0);
analogWrite(ENA_PIN, motorSpeedPWM);
```
- A DC motor goes full speed if we give it 5V. If we want 50% speed, we can't easily give it 2.5V.
- Instead, we use **Pulse Width Modulation (PWM)** via `analogWrite()`.
- The Arduino turns the 5V power on and off hundreds of times a second. By controlling the "Duty Cycle" (0 to 255), we control the *average* voltage the motor feels, allowing us to ramp the speed up and down smoothly!
"""
with open("Day_33_DC_Motor_L298N/README.md", "a", encoding="utf-8") as f:
    f.write(content_day33)

content_day34 = """
## 🧠 Code Explanation

Let's break down how we built a custom stepper driver from scratch using Bit-Masking:

### 1. The Half-Step Sequence Table
```cpp
const byte halfStepSequence[8] = {
  0b1000, // Coil A
  0b1100, // Coil A + B
  0b0100, // Coil B
  // ...
};
```
- A unipolar stepper motor has 4 electromagnets. To turn the shaft, we have to magnetize them in a very specific order to pull the internal magnets around in a circle.
- We use a "Half-Step" sequence. By alternating between turning 1 coil on and 2 coils on simultaneously, we double the motor's resolution (64 steps per revolution instead of 32) making it incredibly smooth!

### 2. Bit-Mask Execution
```cpp
void writeCoils(byte pinMask) {
  digitalWrite(IN1_PIN, (pinMask & 0b1000) ? HIGH : LOW);
  digitalWrite(IN2_PIN, (pinMask & 0b0100) ? HIGH : LOW);
  // ...
}
```
- Instead of messy `if` statements, we pass our 4-bit binary sequence directly into `writeCoils()`.
- We use the Bitwise AND operator (`&`). `pinMask & 0b1000` checks if the 4th bit (Coil A) is a `1` or a `0`. We use the ternary operator `? HIGH : LOW` to instantly convert that bit into a physical output voltage on the pin!
"""
with open("Day_34_Stepper_ULN2003/README.md", "a", encoding="utf-8") as f:
    f.write(content_day34)

content_day35 = """
## 🧠 Code Explanation

Let's break down how we execute Trapezoidal Velocity Ramping for CNC control:

### 1. The Physics of Ramping
```cpp
case STATE_ACCEL:
    currentSpeedSPS += ACCELERATION_SPS2 * dt;
```
- In robotics, you cannot tell a stepper motor to instantly jump to 1000 RPM. The rotor has physical mass (inertia). If the magnetic field jumps instantly, the motor will screech and stall ("slip poles").
- We must mathematically ramp the speed up linearly. Every 5 milliseconds (`dt`), we add a small chunk of acceleration to our `currentSpeedSPS` (Steps Per Second).

### 2. Microsecond Step Gating
```cpp
if (currentMicros - lastStepMicros >= stepPeriodUs) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(STEP_PIN, LOW);
}
```
- The A4988 driver requires just a 2-microsecond `HIGH` pulse on the STEP pin to turn the motor.
- How do we control the speed? By changing the delay *between* the pulses! (`stepPeriodUs`). 
- As our `currentSpeedSPS` ramps up, we calculate a smaller and smaller `stepPeriodUs`, which fires the pulses faster, smoothly accelerating the heavy motor without skipping!
"""
with open("Day_35_Stepper_A4988/README.md", "a", encoding="utf-8") as f:
    f.write(content_day35)

content_day36 = """
## 🧠 Code Explanation

Let's break down how we calculate Real-Time motor RPM using Odometry:

### 1. Atomic Register Access (Interrupt Safety)
```cpp
noInterrupts();
long pulsesCaptured = encoderPulseCount;
encoderPulseCount = 0; 
interrupts();
```
- `encoderPulseCount` is a 4-byte `long` variable. Because the Arduino is an 8-bit chip, it takes 4 separate clock cycles to read this variable.
- What happens if the hardware interrupt fires *exactly* while we are reading byte 2? The variable changes mid-read, resulting in massive, corrupted RPM spikes!
- **The Fix:** We call `noInterrupts()` to temporarily block the interrupt for exactly 1 microsecond while we safely copy and reset the count, then instantly turn `interrupts()` back on.

### 2. RPM Math
```cpp
calculatedRPM = (float)(pulsesCaptured * 60000.0) / (ENCODER_PPR * timeElapsed);
```
- Our slotted wheel has 20 holes (`ENCODER_PPR`).
- To calculate RPM: We divide the pulses we captured by 20 to get the number of Revolutions.
- We then divide the Revolutions by the Time Elapsed (in milliseconds).
- Finally, we multiply by 60,000 to convert milliseconds into Minutes, giving us Revolutions Per Minute!
"""
with open("Day_36_Motor_Encoder/README.md", "a", encoding="utf-8") as f:
    f.write(content_day36)

content_day37 = """
## 🧠 Code Explanation

Let's break down the math of a Closed-Loop PID Controller:

### 1. Proportional, Integral, and Derivative Math
```cpp
double pTerm = Kp * error;
integralAccumulator += error * dt;
double iTerm = Ki * integralAccumulator;
double dTerm = Kd * ((error - lastError) / dt);
```
- **Proportional (`Kp`):** If our RPM is 100 below target, we push the gas hard. If it's 5 below target, we push lightly. It responds linearly to the *current* error.
- **Integral (`Ki`):** If a heavy robot is stuck on a hill, the Proportional term might not be strong enough to push it. The Integral term *sums up* the error over time. The longer the robot is stuck, the larger the accumulator grows, eventually overpowering the hill!
- **Derivative (`Kd`):** Looks at the *slope* of the error. If the error is dropping very fast (we are approaching the target rapidly), the Derivative term goes negative, pulling the brakes to prevent us from overshooting the target!

### 2. Integral Anti-Windup
```cpp
integralAccumulator = constrain(integralAccumulator, -integralMaxLimit, integralMaxLimit);
```
- If someone physically grabs the motor shaft so it cannot spin, the Integral error will sum up to infinity (Windup).
- When they let go, the massive accumulated error will cause the motor to blast forward wildly out of control.
- `constrain()` clamps the accumulator to a safe maximum limit, ensuring the robot behaves safely when unstuck.
"""
with open("Day_37_PID_Speed_Control/README.md", "a", encoding="utf-8") as f:
    f.write(content_day37)

content_day38 = """
## 🧠 Code Explanation

Let's break down how we use acoustic Time-of-Flight to measure distance:

### 1. The Trigger Pulse
```cpp
digitalWrite(TRIG_PIN, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);
```
- We fire a 10-microsecond 5V pulse into the HC-SR04's Trigger Pin. 
- This wakes up the chip, which then blasts an invisible 8-cycle ultrasonic sound wave (40 kHz) out of the left speaker cone.

### 2. Range-Gated Time of Flight
```cpp
unsigned long pulseDuration = pulseIn(ECHO_PIN, HIGH, 20000);
float distCm = (float)pulseDuration * 0.0343 / 2.0;
```
- `pulseIn()` listens to the Echo pin and times exactly how many microseconds it stays HIGH.
- **The Timeout (20000):** By default, `pulseIn` waits 1000ms (1 full second). If there are no obstacles, our robot will completely freeze for a second! We set a 20ms timeout. In 20ms, sound travels 3.4 meters. If we hear nothing by then, we abort and assume the path is clear.
- **The Math:** We multiply the microseconds by the speed of sound (`0.0343 cm/µs`). We then divide by `2` because the sound had to travel to the wall *and* back!
"""
with open("Day_38_Ultrasonic_Radar/README.md", "a", encoding="utf-8") as f:
    f.write(content_day38)

content_day39 = """
## 🧠 Code Explanation

Let's break down how we parse Bluetooth commands safely using a Buffer:

### 1. Software Serial
```cpp
#include <SoftwareSerial.h>
SoftwareSerial bluetoothSerial(10, 11);
```
- The Arduino Uno only has one hardware Serial port (Pins 0/1). If we wire the Bluetooth module to Pins 0/1, we can't upload code to the Arduino anymore!
- `SoftwareSerial` uses bit-banging magic to create a *virtual* serial port on Pins 10 and 11, allowing us to talk to the HC-05 while keeping the USB port free for PC debugging.

### 2. The Command Parser
```cpp
if (c == ';') {
    packetBuffer.toUpperCase();
    executeRobotCommand(packetBuffer);
    packetBuffer = ""; 
} else {
    packetBuffer += c;
}
```
- Serial data doesn't arrive all at once. The word "FORWARD" arrives one letter at a time, milliseconds apart.
- If we try to process instantly, we only see "F".
- Instead, we add characters to a `packetBuffer` string. We only trigger the motors when we see the `;` terminator character. This ensures our command packet is 100% complete and uncorrupted before the robot acts!
"""
with open("Day_39_Bluetooth_Control/README.md", "a", encoding="utf-8") as f:
    f.write(content_day39)

content_day40 = """
## 🧠 Code Explanation

Let's break down how we send structured binary data over a 2.4GHz Radio link:

### 1. Dual-Role Firmware Setup
```cpp
if (digitalRead(ROLE_PIN) == LOW) {
    isTransmitter = true;
}
```
- Writing separate "Transmitter" and "Receiver" sketches is annoying to maintain.
- We put *both* codes into one file! If we connect Pin 8 to Ground with a jumper wire, the Arduino boots as a remote control. If we leave Pin 8 unplugged, it boots as the receiver. One code rules them all!

### 2. Transmitting Structured Payloads (Structs)
```cpp
struct TelemetryPacket {
  uint32_t packetID;   
  int sensorValue;     
  float uptimeSeconds; 
};

radio.write(&packet, sizeof(TelemetryPacket));
```
- We don't want to send messy, easily corrupted Text strings (like `"ID:1,Val:500"`). It wastes bandwidth and is slow to parse.
- Instead, we pack our variables tightly into a C++ `struct`. This block is exactly 10 bytes long in memory.
- `radio.write` takes the memory address (`&packet`) and just blasts those raw 10 bytes of binary over the airwaves at 1 Megabit per second. The receiver grabs the 10 bytes, drops them directly into its own matching struct, and instantly has access to the floats and ints with zero parsing required!
"""
with open("Day_40_nRF24L01_Wireless/README.md", "a", encoding="utf-8") as f:
    f.write(content_day40)

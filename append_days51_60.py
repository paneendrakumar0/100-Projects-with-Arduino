import os

content_day51 = """
## 🧠 Code Explanation

Let's break down how we built a raw Modbus RTU node:

### 1. The RS485 Half-Duplex Transceiver
```cpp
void transmitFrame(uint8_t* buffer, int length) {
  digitalWrite(RS485_FLOW_PIN, HIGH);
  rs485Serial.write(buffer, length);
  rs485Serial.flush();
  digitalWrite(RS485_FLOW_PIN, LOW);
}
```
- RS485 is a 2-wire differential bus. It is "Half-Duplex", meaning devices cannot talk and listen at the same time.
- The MAX485 chip has a Flow Control pin (`DE/RE`). We must pull it `HIGH` to put the chip into Transmit Mode, blast our byte array out the serial port, call `.flush()` to ensure every single bit has physically left the Arduino, and then instantly pull the pin `LOW` to go back to Listening mode before the master replies!

### 2. Modbus Silent Interval Delimiter
```cpp
if (rxIndex > 0 && (millis() - lastCharTimeMs >= MODBUS_SILENT_INTERVAL_MS)) {
    processModbusFrame();
    rxIndex = 0; 
}
```
- Unlike some protocols that use start/stop characters (like `<` and `>`), Modbus uses **Time** as a delimiter.
- The standard dictates that any silence on the line longer than 3.5 character times (about 4-5ms at 9600 baud) signifies the End of a Frame.
- Every time a byte arrives, we reset `lastCharTimeMs`. Once the line goes quiet for >5ms, our `if` statement triggers, taking all the bytes we collected in the buffer and passing them to the CRC validator and parser!
"""
with open("Day_51_Modbus_RS485/README.md", "a", encoding="utf-8") as f:
    f.write(content_day51)

content_day52 = """
## 🧠 Code Explanation

Let's break down how we talk to the CAN Bus controller using raw SPI registers:

### 1. Bit Timing and Baud Rate Configuration
```cpp
mcpWriteRegister(REG_CNF1, 0x07);
mcpWriteRegister(REG_CNF2, 0x91);
mcpWriteRegister(REG_CNF3, 0x01);
```
- CAN bus requires exact synchronization between nodes. 125 kbps means 1 bit takes exactly 8 microseconds.
- An automotive CAN controller breaks that 8µs bit down into Time Quanta (TQ) to handle propagation delays across long copper wires.
- We configure `CNF1`, `CNF2`, and `CNF3` to set the Prescaler to 8 (yielding 1µs TQ) and divide the bit into a Sync phase, Prop phase, Phase 1, and Phase 2. This perfectly aligns our sampling point to 75% of the bit time!

### 2. Fast SPI Transmission (RTS)
```cpp
digitalWrite(CAN_CS_PIN, LOW);
SPI.transfer(INST_RTS_TXB0);
digitalWrite(CAN_CS_PIN, HIGH);
```
- We write our Standard ID, Data Length (DLC), and 8 bytes of payload into the `TXB0` registers.
- Instead of using a slow software command to tell the chip to transmit, we use a dedicated hardware SPI instruction: Request-To-Send (`INST_RTS_TXB0`, 0x81).
- This blasts the packet out onto the differential CAN lines instantly, autonomously handling bit-stuffing and CRC generation in hardware!
"""
with open("Day_52_CAN_Bus/README.md", "a", encoding="utf-8") as f:
    f.write(content_day52)

content_day53 = """
## 🧠 Code Explanation

Let's break down how we perform raw memory operations on a Winbond SPI Flash chip:

### 1. The Sector Erase Cycle
```cpp
digitalWrite(FLASH_CS_PIN, LOW);
SPI.transfer(CMD_SECTOR_ERASE_4K);
SPI.transfer((address >> 16) & 0xFF);
SPI.transfer((address >> 8) & 0xFF);
SPI.transfer(address & 0xFF);
digitalWrite(FLASH_CS_PIN, HIGH);
```
- Flash memory physically cannot overwrite a `0` bit with a `1` bit. You can only flip `1`s to `0`s.
- Therefore, before writing new data, you MUST erase the memory (which flips all bits in the sector to `1`, or `0xFF`).
- We send the Erase command (`0x20`) followed by a 24-bit address (`A23` down to `A0`), chopped into three 8-bit chunks using bit-shifting.

### 2. Writing C++ Structs directly to Silicon
```cpp
void writeBytes(uint32_t address, uint8_t* buffer, int length)
// ...
TelemetryRecord record1 = { 1000, 24.57, 1, "OK_SYS" };
writeBytes(writeAddr, (uint8_t*)&record1, sizeof(record1));
```
- Instead of converting our variables into messy Comma-Separated Strings, we write the raw binary memory of our C++ `struct` directly to the flash chip!
- By casting the address of our struct to a byte pointer `(uint8_t*)&record1`, our `writeBytes` loop steps through the exact 16 bytes of RAM where the integer, float, and char array live, transferring them 1:1 onto the Flash silicon.
"""
with open("Day_53_SPI_Flash_Logger/README.md", "a", encoding="utf-8") as f:
    f.write(content_day53)

content_day54 = """
## 🧠 Code Explanation

Let's break down the assembly-level magic required to bit-bang WS2812B timings:

### 1. Bypassing the Arduino Abstraction
```cpp
volatile uint8_t *port = &PORTB;
*port = portValHigh; // Sets Pin 8 HIGH instantly
```
- The standard `digitalWrite()` function takes about 4 to 5 microseconds to execute. 
- A WS2812B bit requires a pulse that is only 0.4 microseconds long! `digitalWrite` is 10x too slow.
- We bypass it entirely by using pointers to directly write to the ATmega328P's `PORTB` hardware register, which executes in a single 62.5-nanosecond clock cycle!

### 2. Nanosecond NOP Delays
```cpp
__asm__ __volatile__ (
    "nop\\n\\t" "nop\\n\\t" "nop\\n\\t" "nop\\n\\t"
    "nop\\n\\t" "nop\\n\\t" "nop\\n\\t" "nop\\n\\t"
);
```
- To hold a pin HIGH for exactly 750 nanoseconds, we can't use `delayMicroseconds()`.
- Instead, we inject raw Assembly Language instructions directly into the C++ compiler.
- `nop` stands for "No Operation". It tells the CPU to literally do nothing for exactly 1 clock cycle (62.5ns). By stringing 12 `nop`s together, we create a perfectly deterministic 750ns delay to fulfill the exact waveform requirement of a '1' bit!
"""
with open("Day_54_WS2812B_Bitbang/README.md", "a", encoding="utf-8") as f:
    f.write(content_day54)

content_day55 = """
## 🧠 Code Explanation

Let's break down how we built an 8-bit parallel bus using Port Splitting:

### 1. Port Splitting the Data Bus
```cpp
PORTD = (PORTD & 0x03) | ((value & 0x3F) << 2);
PORTB = (PORTB & 0xFC) | ((value & 0xC0) >> 6);
```
- An 8-bit parallel LCD needs 8 data wires. The ATmega328P's "Port D" has 8 pins (D0-D7). But D0 and D1 are used for the USB Serial connection! If we overwrite them, we lose `Serial.print()`.
- **The Split:** We take our 8-bit character (e.g., `value`).
- We slice off the bottom 6 bits (`& 0x3F`), shift them left by 2 (`<< 2`), and inject them into D2-D7 on `PORTD`. 
- We slice off the top 2 bits (`& 0xC0`), shift them right by 6 (`>> 6`), and inject them into D8-D9 on `PORTB`. 
- Our 8-bit byte is perfectly split across two physical hardware ports simultaneously!

### 2. The Enable Strobe
```cpp
digitalWrite(PIN_E, HIGH);
delayMicroseconds(5); 
digitalWrite(PIN_E, LOW);
```
- Once all 8 data wires are energized with the correct 1s and 0s, the LCD doesn't read them immediately.
- We have to flick the Enable (`E`) pin HIGH, wait a few microseconds, and pull it LOW.
- The LCD's internal controller latches the 8 bits into its memory on the *falling edge* of this strobe pulse!
"""
with open("Day_55_Parallel_LCD/README.md", "a", encoding="utf-8") as f:
    f.write(content_day55)

content_day56 = """
## 🧠 Code Explanation

Let's break down the math behind 2D Ultrasonic Trilateration:

### 1. Sequential Gating to Prevent Cross-Talk
```cpp
float r1 = readDistance(S1_TRIG_PIN, S1_ECHO_PIN);
delay(30); 
float r2 = readDistance(S2_TRIG_PIN, S2_ECHO_PIN);
```
- If we trigger both sonars at the same time, the acoustic pings will collide in the air. Sensor 1 might hear Sensor 2's echo, resulting in completely corrupted coordinate data!
- We enforce a strict 30-millisecond delay between reads. Since sound travels 10 meters in 30ms, this ensures all bouncing echoes from the first ping have faded away before we fire the second.

### 2. Trilateration Geometry
```cpp
float x = (dSq + r1Sq - r2Sq) / (2.0 * BASELINE_D);
float y = sqrt(r1Sq - (x * x));
```
- We treat the two sensors as the centers of two intersecting circles. The radius of Circle 1 is `r1` and the radius of Circle 2 is `r2`.
- By placing Sensor 1 at coordinate `(0,0)` and Sensor 2 at coordinate `(d, 0)`, we can use algebra to find the exact intersection point `(x, y)` of the two circles.
- This gives us the absolute Cartesian coordinates of the target object relative to our sensor baseline!
"""
with open("Day_56_Spatial_Positioner/README.md", "a", encoding="utf-8") as f:
    f.write(content_day56)

content_day57 = """
## 🧠 Code Explanation

Let's break down how we built a Fast Fourier Transform (FFT) engine from scratch:

### 1. Deterministic Sampling at the Nyquist Rate
```cpp
while (micros() - tStart < SAMPLING_PERIOD_US) { }
```
- To analyze frequency, our samples must be perfectly spaced in time. We chose a 4000 Hz sample rate (one sample every 250us).
- `analogRead()` takes ~100us. We then trap the code in an empty `while` loop, monitoring `micros()`, and breaking out the *exact* microsecond the 250us timer hits. This guarantees perfect phase alignment!
- A 4000 Hz sample rate gives us a "Nyquist Frequency" of 2000 Hz. We can accurately detect any audio pitch up to 2000 Hz.

### 2. The Cooley-Tukey Butterfly Operation
```cpp
float tr = real[v] * wr - imag[v] * wi;
float ti = real[v] * wi + imag[v] * wr;
real[v] = real[u] - tr;
// ...
```
- The FFT algorithm breaks a massive 64-point calculation down into tiny pairs of 2, called "Butterflies".
- It multiplies the time-domain audio samples by complex "Twiddle Factors" (Sines and Cosines, represented by `wr` and `wi`).
- This recursively recombines the time-domain points until they mathematically morph into the frequency-domain magnitudes, showing us exactly how much bass, mid, and treble energy is in the audio signal!
"""
with open("Day_57_FFT_Spectrum/README.md", "a", encoding="utf-8") as f:
    f.write(content_day57)

content_day58 = """
## 🧠 Code Explanation

Let's break down how we control Hardware PWM via direct AVR Registers:

### 1. Configuring Fast PWM Mode 14
```cpp
TCCR1A = _BV(COM1A1) | _BV(WGM11);
TCCR1B = _BV(WGM13) | _BV(WGM12) | PRESCALER_BITS;
```
- The Arduino `analogWrite()` command locks you into ~490 Hz or ~980 Hz. To get custom frequencies, we talk directly to the CPU's Timer1 registers (`TCCR1A`, `TCCR1B`).
- We set the Waveform Generation Mode (WGM) bits to `1110` (Mode 14). This mode uses the `ICR1` register to define the "TOP" of the timer (controlling the frequency), and the `OCR1A` register to define when the pin turns off (controlling the duty cycle).

### 2. Calculating the Register Values (TOP and Compare)
```cpp
uint32_t icr = (F_CPU / (PRESCALER_VALUE * freq)) - 1;
ICR1 = icr;
OCR1A = (uint16_t)((dutyCycle / 100.0) * (icr + 1));
```
- **Frequency:** We divide the CPU clock (`16,000,000`) by our prescaler (`8`) and our desired frequency (e.g., `1000 Hz`). This gives us the exact number of clock ticks required for one wave period! We shove this into `ICR1`.
- **Duty Cycle:** If we want a 50% duty cycle, we multiply our total ticks (`icr`) by 0.50, and put that into `OCR1A`. The hardware pin will now stay HIGH for exactly 50% of the clock ticks, and then autonomously switch LOW for the remaining ticks!
"""
with open("Day_58_Timer1_PWM/README.md", "a", encoding="utf-8") as f:
    f.write(content_day58)

content_day59 = """
## 🧠 Code Explanation

Let's break down how we built a precision I2C power monitor:

### 1. Configuring the INA219 ADC Registers
```cpp
uint16_t config = 0b0011111111111111;
writeRegister16(INA219_ADDR, REG_CONFIG, config);
```
- We bypass libraries and write a 16-bit binary string directly to the INA219's `0x00` config register over I2C.
- This binary string (`0x3FFF`) sets the Bus Voltage range to 32V, the Shunt Voltage range to ±320mV, and importantly, configures the internal hardware to take 128 samples and average them together before giving us a reading, completely eliminating electrical noise!

### 2. Calibrating the Current Engine
```cpp
// Cal = trunc(0.04096 / (0.001 * 0.1)) = 4096
writeRegister16(INA219_ADDR, REG_CALIBRATION, 4096);
```
- The INA219 has a built-in math engine. To use it, we have to tell it the physical value of our shunt resistor (`0.1 Ohm`) and the resolution we want (`1 mA per bit`).
- We plug those into the datasheet's calibration formula to get a magic integer (`4096`).
- Once we write `4096` to the calibration register (`0x05`), the INA219 hardware automatically multiplies the raw voltage drop across the shunt resistor by this value. When we read the Current Register (`0x04`), it instantly spits out the exact Amperage, saving our Arduino from doing any floating-point math!
"""
with open("Day_59_Voltage_Monitor/README.md", "a", encoding="utf-8") as f:
    f.write(content_day59)

content_day60 = """
## 🧠 Code Explanation

Let's break down how we built a real-time OLED Oscilloscope:

### 1. Mapping ADC Amplitude to Pixel Rows
```cpp
uint8_t pixelY = WAVE_ROWS - 1 - (uint8_t)((uint32_t)samples[col] * (WAVE_ROWS - 1) / 1023);
setPixel(col, pixelY);
```
- Our ADC reads analog voltage as a number between 0 and 1023.
- Our OLED screen has 64 vertical pixels, but we reserve the bottom 16 for text, giving us 48 pixels (`WAVE_ROWS`) for the waveform.
- We scale the 0-1023 reading down to 0-47. 
- Because pixel `Y=0` is at the *top* of the screen, we subtract our value from `47` to invert it, ensuring that 5V (1023) plots at the top, and 0V (0) plots at the bottom!

### 2. High-Speed I2C Framebuffer Flushing
```cpp
Wire.write(0x40); // Data continuation byte
for (uint8_t col = 0; col < SCREEN_W; col++) {
    Wire.write(framebuffer[col][page]);
}
```
- We don't use slow commands like `display.drawPixel()`. We maintain a 1024-byte `framebuffer` array in the Arduino's RAM.
- We draw our entire waveform into this invisible RAM array first.
- Once complete, we use a rapid I2C loop to dump the entire array sequentially to the OLED's Graphics RAM in one giant blast. This allows us to achieve incredibly high frame rates required for a responsive oscilloscope!
"""
with open("Day_60_OLED_Oscilloscope/README.md", "a", encoding="utf-8") as f:
    f.write(content_day60)

import os

content_day21 = """
## 🧠 Code Explanation

Let's break down how to interface an LCD using only 2 wires (I2C):

### 1. The I2C Backpack
```cpp
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
```
- A standard LCD screen has 16 pins. Wiring all of those up manually takes 6 digital pins from the Arduino and creates a massive nest of wires.
- The I2C Backpack has a microchip (PCF8574) soldered to the back of the LCD. This chip acts as a translator. It takes serial data from the Arduino over just 2 wires (`SDA` and `SCL`) and internally converts it to the 16 parallel pins needed to drive the screen.
- `0x27` is the physical hex address of this specific chip on the I2C bus.

### 2. Formatting the Screen
```cpp
lcd.setCursor(0, 1);
lcd.print("Uptime: ");
lcd.print(uptimeSeconds);
lcd.print("s    ");
```
- The LCD is a grid of 16 columns and 2 rows. `setCursor(Column, Row)` moves the invisible typing cursor. Note that it is 0-indexed! (Row 1 is actually index `0`, Row 2 is index `1`).
- Because the screen doesn't automatically erase old text, printing `10s` over `9s` would look like `10s`. But if the time goes backward (or clears), you get leftover garbage characters. Printing `s    ` (with trailing spaces) perfectly erases any leftover digits from the previous update!
"""
with open("Day_21_LCD_HelloWorld/README.md", "a", encoding="utf-8") as f:
    f.write(content_day21)


content_day22 = """
## 🧠 Code Explanation

Let's break down how we built a responsive Stopwatch UI:

### 1. Display Refresh Rate Limiter
```cpp
const unsigned long DISPLAY_REFRESH_INTERVAL = 50; 
if (currentTime - lastDisplayRefreshTime >= DISPLAY_REFRESH_INTERVAL) {
    updateTimeDisplay(activeTime);
}
```
- Sending text over the I2C bus takes real CPU time (about 1-2 milliseconds per update). If we let the `loop()` update the screen at maximum speed, the I2C bus bottlenecks, the screen flickers violently, and our button presses get ignored.
- We limit the LCD update to exactly 20 Hz (every 50ms). This is fast enough to look perfectly smooth to the human eye, but slow enough to leave 99% of the CPU free to calculate time and read buttons!

### 2. Centisecond Math
```cpp
unsigned long centiseconds = (msTime % 1000) / 10;
```
- We want to display hundredths of a second (like a real sports stopwatch). 
- `msTime % 1000` grabs only the remainder milliseconds (e.g., if total time is `1234ms`, it grabs `234ms`).
- We then divide by `10` to convert milliseconds to centiseconds (`234 / 10 = 23`).
"""
with open("Day_22_LCD_Stopwatch/README.md", "a", encoding="utf-8") as f:
    f.write(content_day22)


content_day23 = """
## 🧠 Code Explanation

Let's break down how to scroll a marquee cleanly:

### 1. The Sliding Window (Substring)
```cpp
String displayWindow = "";
for (int i = 0; i < DISPLAY_WIDTH; i++) {
    int characterIndex = (scrollIndex + i) % messageLength;
    displayWindow += scrollMessage[characterIndex];
}
```
- We want to scroll text on Row 2 while keeping Row 1 completely static. The built-in `lcd.scrollDisplayLeft()` command scrolls the ENTIRE screen, ruining Row 1.
- Instead, we build a 16-character `displayWindow` in software. We grab 16 characters from our long `scrollMessage` starting at `scrollIndex`.
- The magic is the `% messageLength` (modulo operator). If our index exceeds the length of the string, it cleanly wraps back around to `0`. This creates a perfect, infinite circular buffer loop!
"""
with open("Day_23_LCD_Marquee/README.md", "a", encoding="utf-8") as f:
    f.write(content_day23)


content_day24 = """
## 🧠 Code Explanation

Let's break down how we use Binary Bit-Masks to drive the 7-segment display:

### 1. The Binary Bit-Mask
```cpp
const byte numPatterns[] = {
  0b00111111, // 0: A, B, C, D, E, F ON
  0b00000110, // 1: B, C ON
  // ...
};
```
- A 7-segment display has 7 individual LEDs. To display a '1', we need segments B and C on, and the rest off.
- Instead of writing 7 messy `digitalWrite()` lines for every single number, we pack the ON/OFF states into a single 8-bit Byte (`0b0GFEDCBA`). 
- For '1', bits B and C are `1`, everything else is `0`. So the byte is `0b00000110`.

### 2. The Hardware Decoder
```cpp
byte pattern = numPatterns[number];
for (int i = 0; i < 7; i++) {
    int segmentState = bitRead(pattern, i);
    digitalWrite(segmentPins[i], segmentState);
}
```
- We grab the master pattern for the number we want to show.
- A `for` loop cycles through pins 0 through 6. The Arduino `bitRead()` function extracts the specific `1` or `0` for that exact segment from our byte, and we instantly write it to the physical pin!
"""
with open("Day_24_7Seg_Counter/README.md", "a", encoding="utf-8") as f:
    f.write(content_day24)


content_day25 = """
## 🧠 Code Explanation

Let's break down how we drive a 4-digit display using a TM1637 driver chip:

### 1. Hardware Multiplexing
```cpp
#include <TM1637Display.h>
TM1637Display display(CLK_PIN, DIO_PIN);
```
- To display "1234", you can't turn on all the LEDs at once, or you'd get an overlapping mess of numbers. You actually have to flash "1" on the first digit, turn it off, flash "2" on the second digit, turn it off, and repeat hundreds of times a second (Multiplexing).
- Doing this manually on an Arduino completely locks up the CPU.
- The TM1637 chip solves this. You send it the number "1234" over Serial, and the chip's internal processor handles the high-speed multiplexing in the background, freeing your Arduino to do other things!

### 2. Time Accumulation Math
```cpp
int displayVal = (hours * 100) + minutes;
byte colonMask = colonState ? 0b01000000 : 0x00;
display.showNumberDecEx(displayVal, colonMask, true);
```
- The TM1637 wants a single integer to display. So to show `12:34`, we multiply hours by 100 (`1200`), and add minutes (`34`), resulting in the integer `1234`.
- The library has a special function `showNumberDecEx()` that allows us to pass a `colonMask` byte. Sending `0x40` (Bit 6) tells the chip to turn on the two LED dots in the center!
"""
with open("Day_25_4Digit_7Seg_Clock/README.md", "a", encoding="utf-8") as f:
    f.write(content_day25)


content_day26 = """
## 🧠 Code Explanation

Let's break down how to draw vector graphics on an OLED screen:

### 1. The Local Framebuffer
```cpp
display.clearDisplay();
display.drawCircle(32, 32, 20, SSD1306_WHITE);
display.drawLine(0, 0, 128, 64, SSD1306_WHITE);
display.display();
```
- An I2C bus is too slow to send instructions pixel by pixel in real time.
- The Adafruit library creates a "Framebuffer" - a 1024-byte chunk of memory right on the Arduino's RAM representing the 128x64 screen.
- When you call `drawCircle()`, the Arduino mathematically changes the bits inside its own memory instantly.
- When you are completely done drawing your scene, you call `display.display()`. This blasts the entire 1KB memory chunk over the I2C bus in one swift transmission, updating the physical screen flawlessly without tearing!
"""
with open("Day_26_OLED_Graphics/README.md", "a", encoding="utf-8") as f:
    f.write(content_day26)


content_day27 = """
## 🧠 Code Explanation

Let's break down how we programmed a 2D physics engine for the OLED display:

### 1. Vector Kinematics
```cpp
ballX += ballVx;
ballY += ballVy;
```
- Every frame (30 times a second), the ball's position (`ballX`, `ballY`) is updated by adding its Velocity Vectors (`ballVx`, `ballVy`). This makes the ball fly smoothly across the screen diagonally!

### 2. Elastic Collision & Penetration Resolution
```cpp
if (ballX - ballRadius <= PLAY_X_MIN) {
    ballVx = -ballVx;                 // Reverse direction vector
    ballX = PLAY_X_MIN + ballRadius;  // Penetration resolution
}
```
- We check if the edge of the ball (`ballX - ballRadius`) has hit the left wall.
- If it has, we simply multiply the X-velocity by `-1`. If it was moving Left at `2px/frame`, it is now moving Right at `2px/frame`! A perfect bounce.
- **Pro Tip:** We also manually snap `ballX` exactly against the wall. Because our simulation moves in "chunks" of pixels, the ball might have accidentally sunken *into* the wall during the frame. Snapping it back prevents the ball from getting permanently stuck inside the wall boundary!
"""
with open("Day_27_OLED_Animation/README.md", "a", encoding="utf-8") as f:
    f.write(content_day27)


content_day28 = """
## 🧠 Code Explanation

Let's break down how to decode invisible infrared light:

### 1. The NEC Protocol
```cpp
#include <IRremote.hpp>
IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
```
- When you press a button on a remote, the IR LED flashes at exactly 38,000 times a second (38 kHz). This specific frequency cuts through the infrared noise of the sun and house lights.
- The flash pattern spells out a 32-bit binary code (the NEC Protocol).
- The `IrReceiver` object monitors the digital pin in the background using hardware interrupts. It captures the exact microsecond lengths of the flashes and decodes them into a clean Hexadecimal number for us.

### 2. Processing Commands
```cpp
if (IrReceiver.decode()) {
    bool isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT);
    
    if (!isRepeat) {
        processIRCommand(IrReceiver.decodedIRData.command);
    }
    IrReceiver.resume(); 
}
```
- `IrReceiver.decode()` returns `true` if a full, valid IR packet was captured.
- We check for a "Repeat" flag. If you hold a button down, the remote sends a special "Repeat" code. We ignore it so that our LED doesn't rapidly toggle on and off while the button is held!
- Finally, `IrReceiver.resume()` resets the sensor so it can catch the next incoming flash.
"""
with open("Day_28_IR_Decoder/README.md", "a", encoding="utf-8") as f:
    f.write(content_day28)


content_day29 = """
## 🧠 Code Explanation

Let's break down how to talk directly to the DS3231 RTC microchip using raw I2C registers instead of a wrapper library!

### 1. I2C Register Reading
```cpp
Wire.beginTransmission(DS3231_I2C_ADDRESS);
Wire.write(0x00); // Point to seconds register
Wire.endTransmission();

Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
```
- The DS3231 stores the Time and Date inside 7 internal "Registers" (memory slots), starting at memory address `0x00`.
- We use the `Wire` library to ping the chip, telling it "I want to read starting at address `0x00`".
- Then, we `requestFrom()` 7 bytes. The chip streams the seconds, minutes, hours, day, date, month, and year back to us in one quick burst!

### 2. BCD (Binary Coded Decimal) Parsing
```cpp
dt->minute = bcdToDec(Wire.read());
```
- The DS3231 stores numbers in a weird format called **BCD**. Instead of storing the number `45` as a standard binary integer (`0010 1101`), it splits the 4 and the 5 into two 4-bit chunks: `0100` (4) and `0101` (5).
- We use a custom `bcdToDec()` bitwise math function to convert this hardware format back into standard decimal numbers that the Arduino (and humans) can read.
"""
with open("Day_29_RTC_Clock/README.md", "a", encoding="utf-8") as f:
    f.write(content_day29)


content_day30 = """
## 🧠 Code Explanation

Let's break down how we drastically extended the lifespan of our Arduino's memory:

### 1. EEPROM Wear Leveling (Ring Buffer)
```cpp
int targetSlot = (activeSlotIndex + 1) % NUM_SLOTS;
int targetAddress = targetSlot * ENTRY_SIZE;
EEPROM.put(targetAddress, entry);
```
- The Arduino's EEPROM memory physically degrades when you write to it. A single byte can only be erased/written about 100,000 times before it is destroyed forever.
- If we always saved our sensor data to address `0`, the memory would break in a few days.
- **The Fix:** We divide the memory into 128 slots. Every time we save a log, we advance to the next slot (using the `%` modulo operator to loop back to 0 when we reach the end). Because we are spreading the wear out evenly, the memory will last 128x longer (over 12 million writes)!

### 2. Data Integrity Checksums
```cpp
entry.checksum = calculateChecksum(entry.logID, entry.sensorValue);
```
- When EEPROM ages, bits can randomly flip. We calculate a simple mathematical checksum (`logID + sensorValue`) and save it alongside the data.
- When we read the memory later, we do the math again. If our result doesn't match the saved checksum, we instantly know the memory slot has been corrupted!
"""
with open("Day_30_EEPROM_Logger/README.md", "a", encoding="utf-8") as f:
    f.write(content_day30)

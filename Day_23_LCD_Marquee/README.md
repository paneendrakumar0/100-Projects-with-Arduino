# Day 23: Scrolling Text Marquee on LCD

Welcome to Day 23 of the 100-Day Arduino Masterclass! Today, we explore dynamic text layouts. We will learn how to build a scrolling text marquee (ticker tape) on a single row of a 16x2 LCD while keeping the other row stationary.

You will master the internal memory structure of the HD44780 controller, study circular buffer indexing using modular mathematics, and implement string slicing algorithms in C++.

---

## 🎯 Today's Learning Goals
1. Differentiate between hardware-level and software-level display scrolling.
2. Master the memory map of HD44780 DDRAM (Display Data RAM).
3. Use modular arithmetic ($index \pmod{length}$) to build circular wrapping strings.
4. Program a non-blocking substring sliding-window slicer.
5. Calibrate character scrolling speeds for human readability.

---

## 🧠 The "Why" and "What": Marquees in Robotics

### What is a Scrolling Marquee?
A marquee is a sliding text display that scrolls characters horizontally across a screen. Since character LCDs have limited screen size (e.g. 16 columns), a marquee allows you to display messages that are much longer than 16 characters by sliding them across the viewport.

### Why is it Used in Robotics & Mechatronics?
Robots use marquees to show long status messages, logs, or help instructions without crowding the screen:
- **Long Error Logs:** Displaying detailed diagnostic logs (e.g., `ERROR: Photoresistor voltage below threshold on Pin A0 - Check light level!`) on a single row while keeping the header `[STATUS: ERROR]` fixed on Row 1.
- **Instruction Tickers:** Scrolling user guide messages on vending machines or terminals (e.g., `Enter PIN code to unlock safe or press * to clear input...`).
- **Interactive Telemetry:** Scanning through multiple sensor outputs sequentially on a single line.

---

## ⚡ The Physics & Hardware Theory

### 1. HD44780 Display Data RAM (DDRAM) Memory Map
The HD44780 chip on the LCD has a built-in memory bank called **Display Data RAM (DDRAM)**. This memory holds the ASCII characters currently rendered on screen.
* For a 16x2 LCD, the DDRAM has **80 bytes** of memory (40 bytes per row).
* Only the first 16 bytes of each row are physically visible on the glass panel.
* **Row 1 Addresses:** `0x00` to `0x27` (Hex). Visible: `0x00` to `0x0F`.
* **Row 2 Addresses:** `0x40` to `0x67` (Hex). Visible: `0x40` to `0x4F`.

```
                    HD44780 Visible vs Hidden DDRAM Map
                    
          Col:  1   2   3  ...  16   17   18  ...  40
         Row 1: [00][01][02]...[0F]  [10][11]...[27]  <--- Hidden Buffer (scrolls in)
         Row 2: [40][41][42]...[4F]  [50][51]...[67]
                <---- VISIBLE ---->  <--- HIDDEN --->
```

### 2. Hardware Scrolling vs. Software Slicing
* **Hardware Scrolling (`lcd.scrollDisplayLeft()`):** 
  This sends a command to shift the internal address pointer of the DDRAM visible window. 
  - *The Limitation:* It shifts the visible window for **both rows simultaneously**. You cannot have Row 1 remain stationary while Row 2 scrolls.
* **Software Slicing (Day 23 Standard):**
  We store the long message in the Arduino's RAM. On every update, we extract a 16-character slice of the string and write it to a specific row using `lcd.setCursor(0, 1)`. Because we target the row cursor, Row 1 remains completely unaffected.

### 3. Circular-Buffer Modular Arithmetic
To make the text scroll forever without crashing or running out of bounds, we use the modulo operator (`%`). The modulo operator yields the remainder of a division.

Let $S$ be the starting index of the window, $i$ be the column index ($0\text{ to }15$), and $L$ be the message string length. The index of the character to display is calculated as:

$$\text{Character Index} = (S + i) \pmod L$$

```
   Message = "HELLOWORLD " (Length L = 11)
   If scrollIndex S = 8:
   i = 0 ➡️ (8 + 0) % 11 = 8 ➡️ 'R'
   i = 1 ➡️ (8 + 1) % 11 = 9 ➡️ 'L'
   i = 2 ➡️ (8 + 2) % 11 = 10 ➡️ 'D'
   i = 3 ➡️ (8 + 3) % 11 = 0  ➡️ 'H' (wrapped!)
   i = 4 ➡️ (8 + 4) % 11 = 1  ➡️ 'E'
   ...
   Output Slice: "RLDHELLO..."
```

This creates a seamless loop where the beginning of the message chases the tail, preventing string boundary exceptions.

---

## 🔄 Alternatives: Software Marquees vs. Dot-Matrix Panels

| Method | Resolution / Size | Pin Requirement | CPU Overhead | Layout Flexibility | Best Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Software LCD Marquee** | $16 \times 2$ characters. | I2C (2 pins). | Low (slicing runs in microseconds). | High (each row acts independently). | **Chosen** for character LCDs displaying descriptive text logs. |
| **Hardware LCD Marquee** | $16 \times 2$ characters. | I2C (2 pins). | Extremely Low | None (both rows locked in sync). | Full-screen scrolling banners. |
| **MAX7219 Dot-Matrix Panel** | $32 \times 8$ LED grid. | SPI (3 pins). | Moderate | High (custom pixel fonts). | Large moving billboard signs, tickers. |

---

## 🛠️ Components Needed

To build this project, you will need:
1. **Arduino Uno or Mega**.
2. **16x2 LCD with I2C Backpack**.
3. **Breadboard & Jumper Wires**.
4. **USB Cable**.

---

## 🔌 Pin-to-Pin Wiring Instructions

| LCD Backpack Pin | Arduino Pin (Uno/Nano) | Arduino Pin (Mega) | Wire Color | Description |
| :---: | :---: | :---: | :--- | :--- |
| **VCC** | **5V** | **5V** | Red | Power supply (+5V) |
| **GND** | **GND** | **GND** | Black | Ground reference |
| **SDA** | **A4** | **20** | Yellow | Serial Data Line |
| **SCL** | **A5** | **21** | Green | Serial Clock Line |

---

## 🧪 How to Test and Validate

Follow these steps to run the marquee and test its wrap behavior:

### 1. Verification of Layout
- Upload `Day_23_LCD_Marquee.ino`.
- **Verify Row 1 (Static):** Row 1 should display `Day 23: Marquee` static and stable. It must not jitter or move.
- **Verify Row 2 (Scrolling):** Row 2 should show characters sliding from right to left at a readable pace.

### 2. Verify Wrap Behavior
- Watch the transition as the end of the sentence is reached.
- The end of the string (`...step by step.   `) should be followed immediately by the beginning (`Welcome to...`), forming a continuous, unbroken text loop with a clear space gap in between.

### 🔍 Troubleshooting Tips
* **The text is jumping or skipping characters:**
  - Check the `scrollDelay` constant. If set below `150` milliseconds, the character shift happens faster than the LCD's liquid crystals can change state, causing text to look blurred or scrambled. Increase the delay to `350` or `400` ms.
* **The LCD displays garbage blocks or freezes after a few scrolls:**
  - Verify that the I2C wires are pushed fully into the header. Noise on the SDA/SCL lines can corrupt communication packets, hanging the LiquidCrystal_I2C driver.
* **The scroll string length is too short:**
  - The circular wrapping logic expects the message length to be larger than 16 characters. If your message is shorter than 16, pad it with trailing spaces in the code string until it exceeds 16 characters.

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

/*
 * 100 Projects with Arduino - Day 23
 * Project: Independent Row LCD Scrolling Marquee (Sub-string Circular Buffers)
 *
 * DESCRIPTION:
 * This project demonstrates how to scroll a long text message on a single row of a 16x2 I2C LCD
 * while keeping the other row static.
 *
 * To meet professional mechatronics standards, this sketch implements:
 * 1. Software Circular-Buffer Wrapping: Instead of using the hardware LCD scrolling command
 *    (which scrolls both rows together), this code uses modular arithmetic to slice a sliding
 *    16-character window from a long string.
 * 2. Non-blocking timing: The scroll index is advanced every 350ms using millis(), ensuring
 *    the loop continues to execute other tasks at high speed.
 *
 * WIRING:
 * - LCD VCC -> Arduino 5V
 * - LCD GND -> Arduino GND
 * - LCD SDA -> Arduino SDA (A4 on Uno)
 * - LCD SCL -> Arduino SCL (A5 on Uno)
 */

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// --- LCD CONFIGURATION ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- MARQUEE CONSTANTS ---
// The scrolling message. We add spaces at the end so it wraps cleanly without words crashing.
const String scrollMessage =
    "Welcome to Day 23 of the 100-Day Arduino Masterclass! Learn mechatronics step by step.   ";
const int DISPLAY_WIDTH = 16;  // Screen width is 16 characters

// --- SCHEDULER VARIABLES ---
unsigned long lastScrollTime = 0;  // Stores last scroll update timestamp
const unsigned long scrollDelay =
    350;  // Scroll speed in ms per character (350ms is standard readability)

// --- STATE VARIABLES ---
int scrollIndex = 0;  // Current starting character index of the display window

void setup() {
  // Initialize LCD screen
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Write static text on Row 1 (Index 0)
  lcd.setCursor(0, 0);
  lcd.print("Day 23: Marquee");

  Serial.begin(9600);
  Serial.println("==================================================");
  Serial.println("Day 23: Independent Row LCD Scrolling Marquee");
  Serial.println("==================================================");
  Serial.println("System armed. Text scrolling on Row 2.");
}

void loop() {
  unsigned long currentTime = millis();

  // Step 1: Update the scrolling window at scheduled non-blocking interval
  if (currentTime - lastScrollTime >= scrollDelay) {
    lastScrollTime = currentTime;

    // Construct the sliding 16-character substring window
    String displayWindow = "";
    int messageLength = scrollMessage.length();

    for (int i = 0; i < DISPLAY_WIDTH; i++) {
      // Use modular arithmetic (%) to wrap the index back to 0 when we exceed message length.
      // This creates a seamless loop!
      int characterIndex = (scrollIndex + i) % messageLength;
      displayWindow += scrollMessage[characterIndex];
    }

    // Step 2: Write the sliced window to Row 2 (Index 1) of the LCD
    lcd.setCursor(0, 1);
    lcd.print(displayWindow);

    // Step 3: Advance the scroll index
    scrollIndex++;
    if (scrollIndex >= messageLength) {
      scrollIndex = 0;  // Wrap index to prevent overflow
    }
  }

  // Non-blocking architecture - additional routines can run here concurrently
}

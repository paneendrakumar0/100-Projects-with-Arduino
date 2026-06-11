/*
 * 100 Projects with Arduino - Day 21
 * Project: 16x2 Character LCD Hello World (I2C Backpack Interfacing)
 *
 * DESCRIPTION:
 * This project introduces Phase 2 (Displays & Interfaces). It demonstrates how to interface
 * a 16x2 character LCD display using an I2C backpack. Instead of a simple static text display,
 * this code runs a non-blocking loop that updates an uptime counter in seconds on the second row
 * every 1 second using millis(), while keeping the CPU fully available.
 *
 * THEORY OF OPERATION:
 * 1. Character LCD (HD44780): A standard 16x2 display uses the Hitachi HD44780 driver chip.
 *    Parallel communication requires 10 pins (8-bit mode) or 6 pins (4-bit mode). This drains
 *    precious I/O lines.
 * 2. PCF8574 I2C I/O Expander:
 *    The I2C backpack contains a PCF8574 8-bit I/O expander chip. It communicates with the Arduino
 *    via the Inter-Integrated Circuit (I2C) serial bus, which requires only **2 wires (SDA and
 * SCL)**. The chip receives 8-bit digital serial packets and translates them to parallel lines to
 * drive the LCD.
 * 3. I2C Bus Architecture:
 *    - SDA (Serial Data): Line for transmitting data bits.
 *    - SCL (Serial Clock): Line for synchronizing data transfer.
 *    - Device Addressing: Every I2C slave has a unique address. The standard I2C backpack address
 *      is typically 0x27 or 0x3F, depending on the PCF8574 chip manufacturer.
 *
 * WIRING:
 * - LCD VCC -> Arduino 5V
 * - LCD GND -> Arduino GND
 * - LCD SDA -> Arduino SDA (Pin A4 on Uno/Nano, Pin 20 on Mega)
 * - LCD SCL -> Arduino SCL (Pin A5 on Uno/Nano, Pin 21 on Mega)
 *
 * LIBRARY REQUIREMENT:
 * This code requires the "LiquidCrystal I2C" library by Frank de Brabander.
 * Install it via the Arduino IDE Library Manager (Manage Libraries -> search "LiquidCrystal I2C").
 */

#include <LiquidCrystal_I2C.h>  // Include I2C LCD Driver Library
#include <Wire.h>               // Include standard I2C Wire Library

// --- LCD CONFIGURATION ---
// Set the LCD I2C address, column count, and row count.
// - Standard PCF8574T chip: 0x27
// - Standard PCF8574AT chip: 0x3F
const int I2C_ADDR = 0x27;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

// Initialize the LCD object
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

// --- SCHEDULER VARIABLES ---
unsigned long lastUpdateTime = 0;           // Stores last screen update timestamp
const unsigned long updateInterval = 1000;  // Update counter every 1 second (1000ms)

void setup() {
  // Initialize Serial Monitor (optional, for verification)
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 21: 16x2 I2C LCD Hello World");
  Serial.println("==================================================");

  // Initialize the I2C bus and the LCD screen
  lcd.init();

  // Turn ON the LED backlight
  lcd.backlight();

  // Clear any garbage characters on screen
  lcd.clear();

  // Print static greeting on Row 1 (Index 0)
  // setCursor(column, row) - both are 0-indexed!
  lcd.setCursor(0, 0);
  lcd.print("Arduino Master!");

  Serial.println("LCD initialized. Greeting printed on row 1.");
}

void loop() {
  unsigned long currentTime = millis();

  // Step 1: Update the running counter every 1 second non-blockingly
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;

    // Calculate running time in seconds
    unsigned long uptimeSeconds = currentTime / 1000;

    // Set cursor to Row 2 (Index 1), Column 1 (Index 0)
    lcd.setCursor(0, 1);

    // Print the label
    lcd.print("Uptime: ");

    // Print the dynamic value
    lcd.print(uptimeSeconds);

    // Print trailing character 's' and trailing spaces
    // The trailing spaces clear out old digits when the number wraps (e.g. from 10 to 9,
    // though uptime only grows. It is a vital practice for scrolling values).
    lcd.print("s    ");

    // Mirror update to Serial Monitor for validation
    Serial.print("[LCD UPDATE] Running counter: ");
    Serial.print(uptimeSeconds);
    Serial.println(" seconds.");
  }

  // Loop is non-blocking. The CPU is fully available for other mechatronic control loops!
}

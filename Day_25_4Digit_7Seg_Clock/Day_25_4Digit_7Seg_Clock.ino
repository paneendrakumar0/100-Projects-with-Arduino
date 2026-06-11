/*
 * 100 Projects with Arduino - Day 25
 * Project: 4-Digit 7-Segment Clock (TM1637 Serial Driver)
 *
 * DESCRIPTION:
 * This project interfaces a 4-digit 7-segment display module driven by the TM1637 chip
 * to build a basic digital clock. To follow professional coding standards:
 * 1. Time Accumulation: Hours and minutes are updated non-blockingly using a software
 *    timer powered by millis().
 * 2. Flashing Colon (:): The central colon separator flashes ON and OFF once every 500ms
 *    using a non-blocking toggle loop.
 * 3. TM1637 Library Interfacing: Employs standard libraries to handle low-level serial bits,
 *    relieving the Arduino CPU from high-speed multiplexing tasks.
 *
 * THEORY OF OPERATION:
 * 1. Digit Multiplexing Problem: A 4-digit display has 32 separate LED segments. Direct control
 *    requires 12 pins. To show different numbers on each digit, the pins must be cycled
 * (multiplexed) at high speed (e.g. Digit 1 ON for 2ms, Digit 2 ON for 2ms, etc.), which requires
 * continuous CPU cycles.
 * 2. TM1637 Dedicated Driver:
 *    The TM1637 chip handles the multiplexing internally. It has internal RAM to store the digit
 * patterns and continuously scans the segments in the background.
 * 3. 2-Wire Serial Communication:
 *    The Arduino communicates with the TM1637 using a custom synchronous 2-wire serial protocol
 * (not standard I2C):
 *    - CLK (Clock): Synchronizes data transfer.
 *    - DIO (Data Input/Output): Transmits data bits.
 *
 * WIRING:
 * - TM1637 VCC -> Arduino 5V
 * - TM1637 GND -> Arduino GND
 * - TM1637 CLK -> Arduino Pin 2 (Digital output)
 * - TM1637 DIO -> Arduino Pin 3 (Digital I/O)
 *
 * LIBRARY REQUIREMENT:
 * This code requires the "TM1637" library by Avishay Orbach.
 * Install it via the Arduino IDE Library Manager (Manage Libraries -> search "TM1637" by Avishay).
 */

#include <TM1637Display.h>  // Include TM1637 Driver Library

// --- PIN DEFINITIONS ---
const int CLK_PIN = 2;  // Clock line pin
const int DIO_PIN = 3;  // Data line pin

// Initialize the display object
TM1637Display display(CLK_PIN, DIO_PIN);

// --- SOFTWARE CLOCK STATE ---
int hours = 12;   // Start time hours (12-hour format default)
int minutes = 0;  // Start time minutes
int seconds = 0;  // Start time seconds

// --- SCHEDULER VARIABLES ---
unsigned long lastClockTick = 0;            // Stores last 1-second clock tick timestamp
const unsigned long clockTickDelay = 1000;  // Tick every 1 second (1000ms)

unsigned long lastColonToggle = 0;           // Stores last colon toggle timestamp
const unsigned long colonToggleDelay = 500;  // Toggle colon every 500ms (0.5 Hz blink)
bool colonState = false;                     // Current state of the colon (ON or OFF)

void setup() {
  // Initialize Serial Monitor for verification
  Serial.begin(9600);

  // Set display brightness (0 is dimmest, 7 is brightest)
  display.setBrightness(4);

  Serial.println("==================================================");
  Serial.println("Day 25: 4-Digit 7-Segment Clock (TM1637)");
  Serial.println("==================================================");
  Serial.println("System Initialized. Starting time loop...");
}

void loop() {
  unsigned long currentTime = millis();

  // --- PART 1: SOFTWARE CLOCK ACCUMULATION (1 Hz) ---
  if (currentTime - lastClockTick >= clockTickDelay) {
    lastClockTick = currentTime;

    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours > 24) {
          hours = 1;  // Loop back to 1 (or 0 for 24-hour mode)
        }
      }
    }

    // Log clock time to Serial
    Serial.print("[CLOCK] Time: ");
    if (hours < 10) Serial.print('0');
    Serial.print(hours);
    Serial.print(":");
    if (minutes < 10) Serial.print('0');
    Serial.print(minutes);
    Serial.print(":");
    if (seconds < 10) Serial.print('0');
    Serial.println(seconds);
  }

  // --- PART 2: COLON FLASHING & SCREEN UPDATE (2 Hz) ---
  if (currentTime - lastColonToggle >= colonToggleDelay) {
    lastColonToggle = currentTime;

    // Flip the colon state
    colonState = !colonState;

    // Format the number to display: HHMM (e.g. 12:34 is written as 1234)
    int displayVal = (hours * 100) + minutes;

    // Construct the colon bitmask:
    // In TM1637Display library, setting bit 6 (0b01000000 = 0x40) on the second digit
    // turns on the central colon separator dots.
    byte colonMask = colonState ? 0b01000000 : 0x00;

    // Write number to screen
    // showNumberDecEx(value, dotsMask, leadingZerosBool)
    display.showNumberDecEx(displayVal, colonMask, true);
  }

  // Non-blocking architecture - additional routines (sensors, alarms) can run here
}

/*
 * 100 Projects with Arduino - Day 22
 * Project: LCD Stopwatch (FSM User Interface & I2C Display Performance Tuning)
 *
 * DESCRIPTION:
 * This project implements a digital stopwatch using a 16x2 character LCD and two tactile
 * pushbuttons (Start/Stop and Reset).
 * To meet professional mechatronics standards, this sketch implements:
 * 1. Finite State Machine UI: Governs states (RESET, RUNNING, PAUSED) cleanly.
 * 2. Display Refresh Rate Limiter: Writing to an LCD via I2C is relatively slow (takes about
 *    1 to 2 milliseconds per write). The code limits the LCD updates to 20 Hz (every 50ms)
 *    using a non-blocking scheduler. This prevents the I2C bus from bottlenecking and eliminates
 *    display flickering while maintaining smooth centisecond updates.
 * 3. Software Button Debouncing: Filters button presses non-blockingly.
 *
 * WIRING:
 * - Start/Stop Button Pin A -> Arduino Pin 2 (Digital input, INPUT_PULLUP)
 * - Start/Stop Button Pin B -> Arduino GND
 * - Reset Button Pin A      -> Arduino Pin 3 (Digital input, INPUT_PULLUP)
 * - Reset Button Pin B      -> Arduino GND
 * - LCD VCC/GND             -> Arduino 5V / GND
 * - LCD SDA/SCL             -> Arduino SDA (A4) / SCL (A5)
 */

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// --- PIN DEFINITIONS ---
const int START_STOP_PIN = 2;  // Pin connected to the Start/Stop button
const int RESET_PIN = 3;       // Pin connected to the Reset button

// --- LCD CONFIGURATION ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- FINITE STATE MACHINE STATES ---
enum StopwatchState { STATE_RESET, STATE_RUNNING, STATE_PAUSED };

StopwatchState currentUIState = STATE_RESET;

// --- TIME TRACKING VARIABLES ---
unsigned long totalElapsedTime = 0;  // Total accumulated time in milliseconds
unsigned long sessionStartTime = 0;  // Timestamp when the current running session started

// --- TIMING/SCHEDULING CONSTANTS ---
const unsigned long DISPLAY_REFRESH_INTERVAL = 50;  // Refresh display every 50ms (20 Hz)
unsigned long lastDisplayRefreshTime = 0;           // Stores last display refresh timestamp

// --- BUTTON DEBOUNCING VARIABLES ---
const unsigned long DEBOUNCE_DELAY = 50;  // Button debounce threshold (50ms)
unsigned long lastStartDebounceTime = 0;  // Debounce timer for Start/Stop button
unsigned long lastResetDebounceTime = 0;  // Debounce timer for Reset button
int lastStartButtonRawState = HIGH;
int startButtonState = HIGH;
int lastResetButtonRawState = HIGH;
int resetButtonState = HIGH;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Configure button inputs with internal pull-up resistors
  pinMode(START_STOP_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);

  // Initialize LCD screen
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Write static layout labels
  lcd.setCursor(0, 0);
  lcd.print("STATUS: RESET   ");
  lcd.setCursor(0, 1);
  lcd.print("TIME:   00:00.00");

  Serial.println("==================================================");
  Serial.println("Day 22: LCD Stopwatch (FSM UI & Performance Tuned)");
  Serial.println("==================================================");
  Serial.println("System Initialized. Press Start/Stop to run.");
}

void loop() {
  unsigned long currentTime = millis();

  // --- PART 1: NON-BLOCKING DEBOUNCED BUTTON SAMPLING ---
  // Start/Stop Button debounce scan
  int startReading = digitalRead(START_STOP_PIN);
  if (startReading != lastStartButtonRawState) {
    lastStartDebounceTime = currentTime;
  }
  if ((currentTime - lastStartDebounceTime) > DEBOUNCE_DELAY) {
    if (startReading != startButtonState) {
      startButtonState = startReading;
      if (startButtonState == LOW) {
        handleStartStopPress();
      }
    }
  }
  lastStartButtonRawState = startReading;

  // Reset Button debounce scan
  int resetReading = digitalRead(RESET_PIN);
  if (resetReading != lastResetButtonRawState) {
    lastResetDebounceTime = currentTime;
  }
  if ((currentTime - lastResetDebounceTime) > DEBOUNCE_DELAY) {
    if (resetReading != resetButtonState) {
      resetButtonState = resetReading;
      if (resetButtonState == LOW) {
        handleResetPress();
      }
    }
  }
  lastResetButtonRawState = resetReading;

  // --- PART 2: STOPWATCH TIMER CALCULATIONS ---
  unsigned long activeTime = totalElapsedTime;
  if (currentUIState == STATE_RUNNING) {
    // Current active running time = past accumulated time + (now - start timestamp)
    activeTime += (currentTime - sessionStartTime);
  }

  // --- PART 3: SCHEDULER REFRESH LIMITER (20 Hz) ---
  if (currentTime - lastDisplayRefreshTime >= DISPLAY_REFRESH_INTERVAL) {
    lastDisplayRefreshTime = currentTime;

    // Update the second row (time string)
    updateTimeDisplay(activeTime);
  }
}

/**
 * Executes state transition when Start/Stop button is pressed.
 */
void handleStartStopPress() {
  unsigned long pressTime = millis();

  if (currentUIState == STATE_RESET || currentUIState == STATE_PAUSED) {
    // Transition to RUNNING
    currentUIState = STATE_RUNNING;
    sessionStartTime = pressTime;  // Mark start time of this session

    lcd.setCursor(8, 0);
    lcd.print("RUNNING ");
    Serial.println("[STOPWATCH] State: RUNNING");
  } else if (currentUIState == STATE_RUNNING) {
    // Transition to PAUSED
    currentUIState = STATE_PAUSED;
    // Add current session time to cumulative running elapsed time
    totalElapsedTime += (pressTime - sessionStartTime);

    lcd.setCursor(8, 0);
    lcd.print("PAUSED  ");
    Serial.println("[STOPWATCH] State: PAUSED");
  }
}

/**
 * Executes state transition when Reset button is pressed.
 */
void handleResetPress() {
  // Reset only works when the stopwatch is NOT running (i.e. reset or paused)
  if (currentUIState == STATE_PAUSED) {
    currentUIState = STATE_RESET;
    totalElapsedTime = 0;  // Clear accumulated time

    lcd.setCursor(8, 0);
    lcd.print("RESET   ");
    updateTimeDisplay(0);  // Immediately update screen to 00:00.00
    Serial.println("[STOPWATCH] State: RESET (Time cleared)");
  }
}

/**
 * Renders the time in MM:SS.cc format on Row 2 of the LCD.
 * cc = centiseconds (hundredths of a second).
 */
void updateTimeDisplay(unsigned long msTime) {
  // Convert milliseconds into hours, minutes, seconds, and centiseconds
  unsigned long totalSeconds = msTime / 1000;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long centiseconds = (msTime % 1000) / 10;  // 1 centisecond = 10ms

  lcd.setCursor(8, 1);

  // Format Minutes (pad single-digit with '0')
  if (minutes < 10) lcd.print('0');
  lcd.print(minutes);
  lcd.print(':');

  // Format Seconds (pad single-digit with '0')
  if (seconds < 10) lcd.print('0');
  lcd.print(seconds);
  lcd.print('.');

  // Format Centiseconds (pad single-digit with '0')
  if (centiseconds < 10) lcd.print('0');
  lcd.print(centiseconds);
}

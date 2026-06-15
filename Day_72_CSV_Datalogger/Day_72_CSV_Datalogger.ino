/*
 * 100 Projects with Arduino - Day 72
 * Project: CSV Datalogger (Sensors to SD Card with Safe Toggle/Eject)
 *
 * DESCRIPTION:
 * This project implements a robust, periodic sensor datalogger that records sensor telemetry
 * in Comma-Separated Values (CSV) format onto an SD Card. To meet industrial design standards:
 * 1. Safe Eject/Toggle Button: Integrates a debounced push button on Pin 2 that allows the user
 *    to gracefully start and stop the datalogger. Stopping the logger writes a finalization footer
 *    to the CSV file and closes the file system handles, enabling safe card ejection.
 * 2. Comma-Separated Values (CSV): Writes a header row on boot if the file is new. Fields logged:
 *    Timestamp (ms), LDR Raw, Potentiometer Raw, Computed Voltage (V).
 * 3. Non-blocking Execution: Sampling and logging are managed at a precise 1 Hz (1000ms) interval
 *    using millis() timers, allowing concurrent button polling and LED status flashing.
 *
 * WIRING:
 * - SD Card Module Pin -> Arduino Uno Pin
 *   - GND             -> GND
 *   - VCC             -> 5V
 *   - MISO            -> Pin 12
 *   - MOSI            -> Pin 11
 *   - SCK             -> Pin 13
 *   - CS              -> Pin 4
 * - Sensors & Indicators:
 *   - LDR (Photoresistor)   -> Pin A0 (with 10kΩ pull-down resistor forming a voltage divider)
 *   - Potentiometer (10kΩ)  -> Pin A1 (outer pins to 5V and GND, center wiper to A1)
 *   - Toggle/Eject Button   -> Pin 2 (connect between D2 and GND, using internal INPUT_PULLUP)
 *   - Status LED (Logging)  -> Pin 5 (via 220Ω resistor to GND)
 */

#include <SD.h>
#include <SPI.h>

// --- PIN DEFINITIONS ---
const int CS_PIN = 4;      // SD CS pin
const int BUTTON_PIN = 2;  // Start/Stop logging toggle button
const int STATUS_LED = 5;  // Logging status LED indicator
const int LDR_PIN = A0;    // LDR analog input
const int POT_PIN = A1;    // Potentiometer analog input

// --- DATALOG CONFIGURATION ---
const char *CSV_FILENAME = "datalog.csv";
const unsigned long SAMPLE_INTERVAL_MS = 1000;  // Log data every 1 second

// --- STATE VARIABLES ---
bool isLogging = false;            // Current logging state
unsigned long lastSampleTime = 0;  // Timer for sampling interval
unsigned long lastBlinkTime = 0;   // Timer for status LED blink
bool statusLedState = false;       // LED state tracking

// Button debounce state variables
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 50;

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 72: CSV Sensor Datalogger with Safe Eject"));
  Serial.println(F("=================================================="));

  // Initialize the SD card
  if (!SD.begin(CS_PIN)) {
    Serial.println(F("[ERROR] SD Card initialization failed! Logging disabled."));
    while (1) {
      // Fast error flash of status LED
      digitalWrite(STATUS_LED, HIGH);
      delay(100);
      digitalWrite(STATUS_LED, LOW);
      delay(100);
    }
  }

  Serial.println(F("[SYSTEM] SD Card initialized successfully."));

  // Write the CSV header row if the file is new
  checkAndWriteHeader();

  printShellMenu();
  Serial.println(F("[STATUS] Press Button D2 or type 't' to START logging."));
}

void loop() {
  // 1. Debounce and read button input to toggle logging state
  pollToggleButton();

  // 2. Perform periodic sampling if logging is active
  if (isLogging) {
    unsigned long currentMillis = millis();

    // Log data every 1000ms (non-blocking)
    if (currentMillis - lastSampleTime >= SAMPLE_INTERVAL_MS) {
      lastSampleTime = currentMillis;
      logSensorData();
    }

    // Blink status LED to show active logging
    if (currentMillis - lastBlinkTime >= 500) {
      lastBlinkTime = currentMillis;
      statusLedState = !statusLedState;
      digitalWrite(STATUS_LED, statusLedState ? HIGH : LOW);
    }
  } else {
    // Turn off status LED when idle
    digitalWrite(STATUS_LED, LOW);
  }

  // 3. Process Serial Shell commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 't':
      case 'T':
        toggleLogging();
        break;
      case 'r':
      case 'R':
        readCSVContents();
        break;
      case 'd':
      case 'D':
        deleteCSVFile();
        break;
      case 'h':
      case 'H':
        printShellMenu();
        break;
      default:
        Serial.print(F("[SHELL] Unknown command: '"));
        Serial.print(cmd);
        Serial.println(F("' - Press 'h' for help."));
        break;
    }
  }
}

// =============================================================
//  CORE DATALOGGER LOGIC
// =============================================================

/**
 * Checks if the CSV file exists. If it does not, creates it and
 * writes a column header row.
 */
void checkAndWriteHeader() {
  if (!SD.exists(CSV_FILENAME)) {
    Serial.print(F("[SYSTEM] Creating new CSV file '"));
    Serial.print(CSV_FILENAME);
    Serial.println(F("'..."));

    File myFile = SD.open(CSV_FILENAME, FILE_WRITE);
    if (myFile) {
      // Write CSV headers
      myFile.println(F("Timestamp_ms,LDR_Raw,Pot_Raw,Voltage_V"));
      myFile.close();
      Serial.println(F("[SYSTEM] CSV Header row written successfully."));
    } else {
      Serial.println(F("[ERROR] Failed to write CSV header. Check card write switch."));
    }
  } else {
    Serial.print(F("[SYSTEM] Existing '"));
    Serial.print(CSV_FILENAME);
    Serial.println(F("' file found. Appending logs."));
  }
}

/**
 * Toggles the current state of the datalogger.
 * Performs graceful setup/shutdown logs.
 */
void toggleLogging() {
  isLogging = !isLogging;

  File myFile = SD.open(CSV_FILENAME, FILE_WRITE);

  if (isLogging) {
    Serial.println(F("[LOGGER] >>> LOGGING STARTED <<<"));
    if (myFile) {
      myFile.println(F("# LOGGER: Logging session started."));
      myFile.close();
    }
  } else {
    Serial.println(F("[LOGGER] >>> LOGGING STOPPED (Safe to eject SD Card) <<<"));
    if (myFile) {
      myFile.println(F("# LOGGER: Logging session terminated gracefully."));
      myFile.close();
    }
  }
}

/**
 * Reads sensor inputs, calculates voltage, and appends a row to the CSV.
 */
void logSensorData() {
  int ldrVal = analogRead(LDR_PIN);
  int potVal = analogRead(POT_PIN);

  // Calculate voltage from potentiometer (0 - 1023 -> 0.0 - 5.0 V)
  float voltage = (potVal * 5.0f) / 1023.0f;

  unsigned long timeStamp = millis();

  // Open the file in write mode
  File myFile = SD.open(CSV_FILENAME, FILE_WRITE);

  if (myFile) {
    // Format: Timestamp, LDR, Pot, Voltage
    myFile.print(timeStamp);
    myFile.print(F(","));
    myFile.print(ldrVal);
    myFile.print(F(","));
    myFile.print(potVal);
    myFile.print(F(","));
    myFile.println(voltage, 2);  // 2 decimal places

    // Commit changes to disk
    myFile.close();

    // Console output for feedback
    Serial.print(F("[LOG] "));
    Serial.print(timeStamp);
    Serial.print(F(" ms | LDR: "));
    Serial.print(ldrVal);
    Serial.print(F(" | Pot: "));
    Serial.print(potVal);
    Serial.print(F(" | Volt: "));
    Serial.print(voltage, 2);
    Serial.println(F(" V"));
  } else {
    Serial.println(F("[ERROR] Failed to open CSV file for logging!"));
  }
}

/**
 * Reads the CSV file contents and outputs them to the Serial Monitor.
 */
void readCSVContents() {
  if (isLogging) {
    Serial.println(F("[ERROR] Cannot read file while logging is active. Stop logging first."));
    return;
  }

  if (!SD.exists(CSV_FILENAME)) {
    Serial.print(F("[SYSTEM] File '"));
    Serial.print(CSV_FILENAME);
    Serial.println(F("' does not exist. Start logging to create it."));
    return;
  }

  Serial.print(F("[SD] Reading '"));
  Serial.print(CSV_FILENAME);
  Serial.println(F("'..."));

  File myFile = SD.open(CSV_FILENAME, FILE_READ);
  if (myFile) {
    Serial.println(F("----------------- CSV DATA LOG -----------------"));
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
    Serial.println(F("------------------------------------------------"));
  } else {
    Serial.println(F("[ERROR] Failed to open CSV file for reading!"));
  }
}

/**
 * Deletes the CSV log file.
 */
void deleteCSVFile() {
  if (isLogging) {
    Serial.println(F("[ERROR] Cannot delete file while logging is active. Stop logging first."));
    return;
  }

  if (!SD.exists(CSV_FILENAME)) {
    Serial.print(F("[SYSTEM] File '"));
    Serial.print(CSV_FILENAME);
    Serial.println(F("' does not exist. Nothing to delete."));
    return;
  }

  Serial.print(F("[SD] Deleting '"));
  Serial.print(CSV_FILENAME);
  Serial.println(F("'..."));

  if (SD.remove(CSV_FILENAME)) {
    Serial.println(F("[SD] CSV file deleted successfully."));
  } else {
    Serial.println(F("[ERROR] Failed to delete CSV file."));
  }
}

// =============================================================
//  HARDWARE POLLING & INTERACTIVE COMMANDS
// =============================================================

/**
 * Polls the toggle button pin and implements debounce logic.
 * Toggles logging state upon a valid press detection.
 */
void pollToggleButton() {
  int reading = digitalRead(BUTTON_PIN);

  // If the button state changed (due to noise or pressing)
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    // If the button state has changed stably
    if (reading != buttonState) {
      buttonState = reading;

      // Button is active-low (pressed down)
      if (buttonState == LOW) {
        toggleLogging();
      }
    }
  }

  lastButtonState = reading;
}

/**
 * Prints the interactive serial shell menu.
 */
void printShellMenu() {
  Serial.println(F("\n--- CSV DATALOGGER INTERACTIVE MENU ---"));
  Serial.println(F(" 't' : Toggle logging state (Start/Stop)"));
  Serial.println(F(" 'r' : Read and print CSV file contents"));
  Serial.println(F(" 'd' : Delete the CSV file"));
  Serial.println(F(" 'h' : Display this help menu"));
  Serial.println(F("---------------------------------------"));
}

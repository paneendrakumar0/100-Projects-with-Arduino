/*
 * 100 Projects with Arduino - Day 71
 * Project: SD Card Module File Writing (SPI Datalogger Basics)
 *
 * DESCRIPTION:
 * This project demonstrates how to interface a standard SPI SD Card Module with an Arduino
 * using the standard SD and SPI libraries. To align with industrial firmware design paradigms:
 * 1. Safe File Operations: Employs correct sequences of opening, writing, flushing, and closing
 *    files to prevent data loss or FAT filesystem corruption during sudden power losses.
 * 2. Diagnostics: Checks for card presence and verifies the SPI link before initiating file access.
 * 3. Interactive Serial Shell: Provides a console interface to write data, read log history,
 *    and delete files, facilitating in-field testing.
 *
 * SD CARD MODULE & SPI THEORY:
 * - FAT File System: The SD library supports FAT16 and FAT32 filesystems on standard SD and SDHC
 * cards. Note: Cards must be formatted as FAT16 or FAT32 (NOT exFAT or NTFS).
 * - SPI (Serial Peripheral Interface): The SD Card module communicates via SPI.
 *   - CS (Chip Select) is typically connected to Pin 4 or Pin 10.
 *   - MOSI (Pin 11), MISO (Pin 12), SCK (Pin 13) are standard hardware SPI pins on Arduino Uno.
 * - Logic Level Shifting: SD cards operate at 3.3V logic. Connecting 5V signals directly to the
 *   SD card pins will damage it. Most breakout boards include an onboard 3.3V regulator and a
 *   level shifter IC (like the 74LVC125A) to translate the 5V signals from the Arduino safely.
 *
 * WIRING:
 * - SD Card Module Pin -> Arduino Uno Pin
 *   - GND             -> GND
 *   - VCC             -> 5V (if module has regulator) or 3.3V
 *   - MISO            -> Pin 12
 *   - MOSI            -> Pin 11
 *   - SCK             -> Pin 13
 *   - CS              -> Pin 4 (configurable)
 */

#include <SD.h>
#include <SPI.h>

// --- PIN DEFINITIONS ---
const int CS_PIN = 4;  // Chip Select pin for SD card module

// --- FILE PATH CONFIGURATION ---
const char *LOG_FILENAME = "datalog.txt";

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // Wait for serial port to connect (needed for native USB boards like Leonardo/Micro)
  }

  Serial.println(F("=================================================="));
  Serial.println(F("Day 71: SD Card Module File Writing & Diagnostics"));
  Serial.println(F("=================================================="));

  // Initialize the SD card
  initializeSDCard();
  printShellMenu();
}

void loop() {
  // Check for serial commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;  // Ignore formatting

    switch (cmd) {
      case 'w':
      case 'W':
        appendLogEntry();
        break;
      case 'r':
      case 'R':
        readFileContents();
        break;
      case 'd':
      case 'D':
        deleteLogFile();
        break;
      case 'i':
      case 'I':
        initializeSDCard();
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
//  SD CARD UTILITY FUNCTIONS
// =============================================================

/**
 * Initializes the SD card and outputs module status.
 */
void initializeSDCard() {
  Serial.print(F("[SYSTEM] Initializing SD Card on CS Pin "));
  Serial.print(CS_PIN);
  Serial.println(F("..."));

  // Check if card is present and can be initialized
  if (!SD.begin(CS_PIN)) {
    Serial.println(F("[ERROR] SD Card initialization failed!"));
    Serial.println(F("[ERROR] Checks:"));
    Serial.println(F("  1. Is the card inserted correctly?"));
    Serial.println(F("  2. Is your wiring correct (CS, MOSI, MISO, SCK)?"));
    Serial.println(F("  3. Is the card formatted as FAT16 or FAT32?"));
    return;
  }

  Serial.println(F("[SYSTEM] SD Card initialized successfully."));

  // Check if our log file already exists
  if (SD.exists(LOG_FILENAME)) {
    Serial.print(F("[SYSTEM] Active log file '"));
    Serial.print(LOG_FILENAME);
    Serial.println(F("' detected."));
  } else {
    Serial.print(F("[SYSTEM] Log file '"));
    Serial.print(LOG_FILENAME);
    Serial.println(F("' not found. It will be created on next write."));
  }
}

/**
 * Appends a diagnostic message to the log file.
 * Formats: Timestamp (millis) and a simple text message.
 */
void appendLogEntry() {
  Serial.print(F("[SD] Opening '"));
  Serial.print(LOG_FILENAME);
  Serial.println(F("' in WRITE mode..."));

  // Open the file in write mode (creates file if it doesn't exist, appends to it)
  File myFile = SD.open(LOG_FILENAME, FILE_WRITE);

  if (myFile) {
    unsigned long timestamp = millis();
    Serial.print(F("[SD] Writing data at t="));
    Serial.print(timestamp);
    Serial.println(F(" ms..."));

    // Write formatted log data
    myFile.print(F("Timestamp: "));
    myFile.print(timestamp);
    myFile.println(F(" ms | Log Event: System running normally."));

    // Force write data to physical media (forces flush of internal SRAM cache to SD card)
    myFile.flush();

    // Close the file (also performs flush and releases file system handle)
    myFile.close();
    Serial.println(F("[SD] File closed successfully. Data is safe."));
  } else {
    Serial.print(F("[ERROR] Failed to open '"));
    Serial.print(LOG_FILENAME);
    Serial.println(F("' for writing!"));
  }
}

/**
 * Reads the entire contents of the log file and dumps it to the Serial Monitor.
 */
void readFileContents() {
  if (!SD.exists(LOG_FILENAME)) {
    Serial.print(F("[SYSTEM] File '"));
    Serial.print(LOG_FILENAME);
    Serial.println(F("' does not exist. Press 'w' to write first."));
    return;
  }

  Serial.print(F("[SD] Opening '"));
  Serial.print(LOG_FILENAME);
  Serial.println(F("' in READ mode..."));

  // Open the file for reading
  File myFile = SD.open(LOG_FILENAME, FILE_READ);

  if (myFile) {
    Serial.println(F("----------------- FILE CONTENTS -----------------"));

    // Read from the file until there's nothing else in it
    while (myFile.available()) {
      Serial.write(myFile.read());
    }

    myFile.close();
    Serial.println(F("-------------------------------------------------"));
  } else {
    Serial.print(F("[ERROR] Failed to open '"));
    Serial.print(LOG_FILENAME);
    Serial.println(F("' for reading!"));
  }
}

/**
 * Deletes the log file from the SD card.
 */
void deleteLogFile() {
  if (!SD.exists(LOG_FILENAME)) {
    Serial.print(F("[SYSTEM] File '"));
    Serial.print(LOG_FILENAME);
    Serial.println(F("' does not exist. Nothing to delete."));
    return;
  }

  Serial.print(F("[SD] Deleting file '"));
  Serial.print(LOG_FILENAME);
  Serial.println(F("'..."));

  if (SD.remove(LOG_FILENAME)) {
    Serial.println(F("[SD] File deleted successfully."));
  } else {
    Serial.println(F("[ERROR] Failed to delete file. Check write protection."));
  }
}

/**
 * Prints the help menu.
 */
void printShellMenu() {
  Serial.println(F("\n--- SD CARD FILE WRITER SHELL ---"));
  Serial.println(F(" 'w' : Append a simulated log entry to file"));
  Serial.println(F(" 'r' : Read and print entire file contents"));
  Serial.println(F(" 'd' : Delete the log file"));
  Serial.println(F(" 'i' : Re-initialize / status check"));
  Serial.println(F(" 'h' : Display this help menu"));
  Serial.println(F("---------------------------------"));
}

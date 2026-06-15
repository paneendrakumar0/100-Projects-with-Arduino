/*
 * 100 Projects with Arduino - Day 85
 * Project: Persistent System Configuration Manager (EEPROM Structs & CRC Diagnostics)
 *
 * DESCRIPTION:
 * This project demonstrates how to save, load, and manage structured non-volatile configuration
 * parameters (such as sensor offsets, calibration constants, and boot logs) using the Arduino's
 * built-in EEPROM. To align with industrial firmware design standards:
 * 1. Struct Serialization: Packs multiple heterogeneous variables (integer, float, char array)
 *    into a single data structure (`SystemConfig`) and saves/loads it using `EEPROM.put()` and
 * `EEPROM.get()`.
 * 2. Data Integrity (Checksum Validation): Calculates a checksum for the config block to verify
 *    that memory has not degraded or been corrupted.
 * 3. Boot Tracking: Automatically loads, increments, and rewrites a persistent boot counter on
 * startup.
 * 4. Configuration CLI: Provides a Serial Command Line Interface to dynamically edit multiplier,
 *    offset, and system name parameters, and save them permanently.
 *
 * EEPROM CONFIGURATION THEORY:
 * - Direct Struct Writing: Standard EEPROM libraries write bytes. `EEPROM.put()` simplifies writing
 *   complex structs by calculating the byte size internally and calling `EEPROM.update()` to write
 *   each byte sequentially.
 * - Write Gating: `EEPROM.put()` only writes a byte if the new value differs from the existing
 * value stored in that cell. This prevents unnecessary wear on the silicon.
 * - Struct Padding: Compilers sometimes insert empty padding bytes to align variables to 2-byte or
 *   4-byte boundaries. On the 8-bit AVR architecture, variables are aligned to single bytes,
 * meaning no padding is added, making the struct size compact and predictable.
 *
 * WIRING:
 * No external components are required. Runs on internal EEPROM.
 */

#include <EEPROM.h>

// --- STRUCT DEFINITIONS ---
struct SystemConfig {
  uint32_t bootCount;      // 4 bytes: Persistent reboot log counter
  float sensorMultiplier;  // 4 bytes: Simulated sensor scaling factor
  float sensorOffset;      // 4 bytes: Simulated sensor zero-point offset
  char systemName[16];     // 16 bytes: Device identification tag
  uint16_t checksum;       // 2 bytes: Validation checksum
};  // Total size = 30 bytes

// --- GLOBAL VARIABLES ---
const int EEPROM_START_ADDR = 0;
SystemConfig activeConfig;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // Wait for USB connection
  }

  Serial.println(F("=================================================="));
  Serial.println(F("Day 85: Persistent EEPROM Configuration Manager"));
  Serial.println(F("=================================================="));

  // 1. Load configuration from EEPROM
  loadConfiguration();

  // 2. Increment boot counter and save back
  activeConfig.bootCount++;
  saveConfiguration();

  Serial.print(F("[BOOT] Boot cycle #"));
  Serial.print(activeConfig.bootCount);
  Serial.println(F(" logged successfully."));

  printConfig(activeConfig);
  printMenu();
}

void loop() {
  // Check for CLI input commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'r':
      case 'R':
        Serial.println(F("[SYSTEM] Reading active configuration:"));
        printConfig(activeConfig);
        break;

      case 's':
      case 'S':
        parseAndSaveConfig();
        break;

      case 'd':
      case 'D':
        resetToFactoryDefaults();
        break;

      case 'h':
      case 'H':
        printMenu();
        break;

      default:
        Serial.print(F("[SHELL] Unknown command: '"));
        Serial.print(cmd);
        Serial.println(F("' - Send 'h' for help."));
        break;
    }
  }
}

// =============================================================
//  EEPROM READ/WRITE & CHECKSUM UTILITIES
// =============================================================

/**
 * Calculates a basic checksum of all bytes in the struct (excluding the checksum field itself).
 */
uint16_t calculateChecksum(const SystemConfig &cfg) {
  uint16_t sum = 0;
  const byte *bytePtr = (const byte *)&cfg;
  int size = sizeof(SystemConfig) - sizeof(cfg.checksum);

  for (int i = 0; i < size; i++) {
    sum += bytePtr[i];
  }
  return sum;
}

/**
 * Loads the system configuration from EEPROM. Falls back to defaults if corrupted.
 */
void loadConfiguration() {
  Serial.println(F("[SYSTEM] Loading configuration from EEPROM..."));

  // Read struct from EEPROM address
  EEPROM.get(EEPROM_START_ADDR, activeConfig);

  // Validate loaded data using the checksum
  uint16_t calculatedCheck = calculateChecksum(activeConfig);

  if (activeConfig.checksum == calculatedCheck && activeConfig.bootCount > 0 &&
      activeConfig.bootCount != 0xFFFFFFFF) {
    Serial.println(F("[SYSTEM] Configuration loaded successfully (CRC match)."));
  } else {
    Serial.println(F("[WARNING] Checksum mismatch or uninitialized memory!"));
    Serial.println(F("[SYSTEM] Loading factory defaults..."));
    loadFactoryDefaults();
  }
}

/**
 * Commits the active configuration struct to the EEPROM with a fresh checksum.
 */
void saveConfiguration() {
  // Calculate checksum before saving
  activeConfig.checksum = calculateChecksum(activeConfig);

  // Save the struct to EEPROM (uses write gating internally via EEPROM.update)
  EEPROM.put(EEPROM_START_ADDR, activeConfig);
}

/**
 * populates the struct with factory default variables.
 */
void loadFactoryDefaults() {
  activeConfig.bootCount = 0;
  activeConfig.sensorMultiplier = 1.0f;
  activeConfig.sensorOffset = 0.0f;
  strcpy(activeConfig.systemName, "Rover-Default");
}

/**
 * Overwrites config back to defaults and saves.
 */
void resetToFactoryDefaults() {
  Serial.println(F("[SYSTEM] Resetting EEPROM to factory defaults..."));
  loadFactoryDefaults();
  saveConfiguration();
  printConfig(activeConfig);
}

// =============================================================
//  CLI INPUT PARSING
// =============================================================

/**
 * Parses user input from console, format: s [multiplier] [offset] [name]
 */
void parseAndSaveConfig() {
  // Read parameters
  float newMultiplier = Serial.parseFloat();
  float newOffset = Serial.parseFloat();
  String newName = Serial.readStringUntil('\n');
  newName.trim();

  // Validate parameters
  if (newMultiplier == 0.0f && newOffset == 0.0f && newName.length() == 0) {
    Serial.println(F("[SHELL] Usage: s [multiplier] [offset] [name]"));
    Serial.println(F("  Example: s 1.25 -0.05 Rover-Beta"));
    return;
  }

  // Update active config
  activeConfig.sensorMultiplier = newMultiplier;
  activeConfig.sensorOffset = newOffset;

  if (newName.length() > 0) {
    // Ensure name fits within char array (15 chars + null terminator)
    newName.substring(0, 15).toCharArray(activeConfig.systemName, 16);
  }

  // Commit changes to EEPROM
  saveConfiguration();
  Serial.println(F("[SYSTEM] Configuration saved to EEPROM."));
  printConfig(activeConfig);
}

// =============================================================
//  FORMATTING PRINT UTILITIES
// =============================================================
void printConfig(const SystemConfig &cfg) {
  Serial.println(F("--------------------------------------"));
  Serial.print(F(" System Name : "));
  Serial.println(cfg.systemName);
  Serial.print(F(" Boot Count  : "));
  Serial.println(cfg.bootCount);
  Serial.print(F(" Multiplier  : "));
  Serial.println(cfg.sensorMultiplier, 4);
  Serial.print(F(" Zero Offset : "));
  Serial.println(cfg.sensorOffset, 4);
  Serial.print(F(" Checksum    : 0x"));
  Serial.println(cfg.checksum, HEX);
  Serial.println(F("--------------------------------------"));
}

void printMenu() {
  Serial.println(F("\n--- EEPROM CONFIG MANAGER COMMAND MENU ---"));
  Serial.println(F(" 'r'             : Read and print current configuration"));
  Serial.println(F(" 's [mult] [off] [name]' : Save new calibration values"));
  Serial.println(F("                           * Example: s 2.54 -0.12 Gripper-1A"));
  Serial.println(F(" 'd'             : Reset configuration to factory defaults"));
  Serial.println(F(" 'h'             : Display this command menu"));
  Serial.println(F("------------------------------------------"));
}

/*
 * 100 Projects with Arduino - Day 30
 * Project: EEPROM Data Logger (Wear-Leveling Ring Buffer)
 *
 * DESCRIPTION:
 * This project implements a robust non-volatile data logger using the Arduino's built-in EEPROM.
 * To meet professional mechatronics and safety standards:
 * 1. Wear-Leveling Ring Buffer: Instead of writing repeatedly to the same memory addresses
 *    (which destroys the silicon), this sketch distributes writes across all 1024 bytes of the
 * EEPROM, increasing the maximum lifespan of the memory by a factor of 128 (over 12 million
 * writes).
 * 2. Data Integrity Checksums: Calculates a checksum for every struct entry to verify that data
 *    has not been corrupted by memory fade.
 * 3. Write-Through Mitigation: Uses `EEPROM.put()` which executes write gating (only burning cells
 *    if the values have changed).
 * 4. Interactive Serial Shell: Provides commands to write simulated logs, read the log buffer
 *    chronologically, and wipe/format the EEPROM.
 *
 * EEPROM HARDWARE THEORY:
 * - Floating Gate Transistors: EEPROM (Electrically Erasable Programmable Read-Only Memory) uses
 * floating-gate transistors to trap electrons to store bits. Program/erase cycles require high
 * voltages generated internally, which slowly degrades the oxide insulation layer. The microchip's
 * internal EEPROM is rated for approximately 100,000 write/erase cycles per byte.
 * - Why Wear Leveling?
 *   If a robot logs data to address 0 every 10 seconds, that byte will wear out and fail in
 * just 11.5 days! By spreading data updates across the entire memory space sequentially, we ensure
 * the entire chip wears out together, drastically prolonging active service life.
 *
 * WIRING:
 * No external components are strictly required as this runs on the Arduino's internal EEPROM.
 * To simulate real-world logging, you can attach an analog sensor (like a potentiometer or LDR) to
 * Pin A0.
 */

#include <EEPROM.h>

// --- STRUCT DEFINITIONS ---
struct LogEntry {
  uint32_t
      logID;  // 4 bytes: Monotonically increasing identifier (used to find chronological order)
  uint16_t sensorValue;  // 2 bytes: Raw ADC data (e.g. from A0)
  uint16_t checksum;     // 2 bytes: Checks data integrity (logID + sensorValue)
};                       // Total size = 8 bytes per entry

// --- EEPROM SIZE & SLOT CONFIGURATION ---
const int EEPROM_SIZE = 1024;                    // 1024 bytes available on ATmega328P
const int ENTRY_SIZE = sizeof(LogEntry);         // 8 bytes
const int NUM_SLOTS = EEPROM_SIZE / ENTRY_SIZE;  // 1024 / 8 = 128 log slots

// --- STATE VARIABLES ---
int activeSlotIndex = -1;  // Index of the slot containing the newest log (-1 if empty/unformatted)
uint32_t nextLogID = 1;    // The next ID to write

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 30: EEPROM Data Logger with Wear-Leveling");
  Serial.println("==================================================");
  Serial.print("[SYSTEM] EEPROM Size: ");
  Serial.print(EEPROM_SIZE);
  Serial.print(" bytes | Struct size: ");
  Serial.print(ENTRY_SIZE);
  Serial.print(" bytes | Total slots: ");
  Serial.println(NUM_SLOTS);

  // Scan the EEPROM on boot to find the write head and chronological sequence
  scanEEPROM();

  printShellMenu();
}

void loop() {
  // Check for incoming serial commands to simulate data logger interface
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    // Ignore newline/carriage return characters
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'w':
      case 'W':
        logSensorValue();
        break;

      case 'r':
      case 'R':
        readAllLogs();
        break;

      case 'c':
      case 'C':
        formatEEPROM();
        break;

      case 'h':
      case 'H':
        printShellMenu();
        break;

      default:
        Serial.print("[SHELL] Unknown command: '");
        Serial.print(cmd);
        Serial.println("' - Press 'h' for help.");
        break;
    }
  }
}

/**
 * Scans all slots in the EEPROM.
 * Validates checksums and finds the slot containing the maximum Log ID to locate the write head.
 */
void scanEEPROM() {
  Serial.println("[SYSTEM] Scanning EEPROM memory map...");
  uint32_t maxLogID = 0;
  int newestSlot = -1;
  int validCount = 0;

  for (int i = 0; i < NUM_SLOTS; i++) {
    LogEntry temp;
    int address = i * ENTRY_SIZE;
    EEPROM.get(address, temp);

    // Validate entry integrity using the checksum
    uint16_t calculatedCheck = calculateChecksum(temp.logID, temp.sensorValue);

    if (temp.checksum == calculatedCheck && temp.logID > 0 && temp.logID != 0xFFFFFFFF) {
      validCount++;
      if (temp.logID > maxLogID) {
        maxLogID = temp.logID;
        newestSlot = i;
      }
    }
  }

  if (newestSlot != -1) {
    activeSlotIndex = newestSlot;
    nextLogID = maxLogID + 1;
    Serial.print("[SYSTEM] Boot scan finished. Found ");
    Serial.print(validCount);
    Serial.print(" valid records. Newest log in Slot: ");
    Serial.print(activeSlotIndex);
    Serial.print(" (LogID: ");
    Serial.print(maxLogID);
    Serial.println(")");
  } else {
    activeSlotIndex = -1;
    nextLogID = 1;
    Serial.println(
        "[SYSTEM] Boot scan finished. No valid records found (EEPROM empty or cleared).");
  }
}

/**
 * Formulates a simple checksum for data validation.
 */
uint16_t calculateChecksum(uint32_t id, uint16_t value) {
  return (uint16_t)((id & 0xFFFF) + ((id >> 16) & 0xFFFF) + value);
}

/**
 * Captures an analog reading and logs it to the next ring buffer slot.
 */
void logSensorValue() {
  // Read A0 (simulated log input). If pin is floating, will yield noise, which is fine!
  uint16_t sensorVal = analogRead(A0);

  // Determine target slot (increment and wrap around)
  int targetSlot = (activeSlotIndex + 1) % NUM_SLOTS;
  int targetAddress = targetSlot * ENTRY_SIZE;

  // Populate log struct
  LogEntry entry;
  entry.logID = nextLogID;
  entry.sensorValue = sensorVal;
  entry.checksum = calculateChecksum(entry.logID, entry.sensorValue);

  // Write to EEPROM using put() which optimizes write counts (uses EEPROM.update)
  EEPROM.put(targetAddress, entry);

  // Update system tracking pointers
  activeSlotIndex = targetSlot;
  nextLogID++;

  Serial.print("[LOG] Saved to Slot ");
  Serial.print(targetSlot);
  Serial.print(" (Address: ");
  Serial.print(targetAddress);
  Serial.print(") | ID: ");
  Serial.print(entry.logID);
  Serial.print(" | Value: ");
  Serial.println(entry.sensorValue);
}

/**
 * Reads and displays all valid logs in chronological order.
 */
void readAllLogs() {
  Serial.println("\n----------------- CHRONOLOGICAL LOGS -----------------");
  int printCount = 0;

  // To print chronologically in a wrapped ring buffer, we need to sort or scan.
  // The simplest, most educational sorting logic for Arduino is to sweep from
  // the oldest possible log ID up to the active slot log ID.
  // We can sweep the array sequentially but print them sorted by LogID.
  // A clean way is to allocate a temporary index map or simply run a loop:

  for (uint32_t targetID = (nextLogID > NUM_SLOTS ? nextLogID - NUM_SLOTS : 1);
       targetID < nextLogID; targetID++) {
    // Find the slot with this ID
    for (int i = 0; i < NUM_SLOTS; i++) {
      LogEntry temp;
      EEPROM.get(i * ENTRY_SIZE, temp);

      if (temp.logID == targetID) {
        uint16_t calcCheck = calculateChecksum(temp.logID, temp.sensorValue);
        if (temp.checksum == calcCheck) {
          Serial.print("Slot ");
          if (i < 10) Serial.print("0");
          Serial.print(i);
          Serial.print(" | ID: ");
          Serial.print(temp.logID);
          Serial.print(" | Sensor Reading (A0): ");
          Serial.println(temp.sensorValue);
          printCount++;
        }
      }
    }
  }

  if (printCount == 0) {
    Serial.println("No logs to display. Press 'w' to write some data first.");
  }
  Serial.println("------------------------------------------------------");
}

/**
 * Clears/Resets the EEPROM by writing 0xFF (erased state) to all bytes.
 */
void formatEEPROM() {
  Serial.println("[SYSTEM] Wiping/formatting EEPROM. Please wait...");

  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.update(i, 0xFF);  // Sets each byte to standard factory-erased state
  }

  activeSlotIndex = -1;
  nextLogID = 1;
  Serial.println("[SYSTEM] EEPROM cleared. All slots formatted to blank.");
}

/**
 * Helper to display shell controls.
 */
void printShellMenu() {
  Serial.println("\n--- INTERACTIVE COMMANDS ---");
  Serial.println(" 'w' : Read sensor and Write log entry (Ring buffer)");
  Serial.println(" 'r' : Read all logged entries in chronological order");
  Serial.println(" 'c' : Clear / Format entire EEPROM");
  Serial.println(" 'h' : Display command help menu");
  Serial.println("----------------------------");
}

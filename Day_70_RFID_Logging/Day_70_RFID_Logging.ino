/*
 * 100 Projects with Arduino - Day 70
 * Project: RFID Access Control Logging System
 *
 * DESCRIPTION:
 * This project implements a secure, standalone Access Control System using the MFRC522
 * RFID reader (SPI) and a DS3231 Real-Time Clock (I2C), backed by the Arduino's built-in
 * EEPROM. It is designed to emulate industrial security systems with:
 * 1. An EEPROM User Database: Stores a Master Card and up to 10 user cards.
 * 2. An EEPROM Access Logger: Implements a wear-leveling ring buffer (similar to Day 30)
 *    to log all scans (Authorized/Denied/Registered/Deregistered) with exact timestamps.
 * 3. A Program Mode: Toggled by scanning the Master Card. While in this mode, scanning any
 *    user card adds it to the authorized database if not present, or removes it if present.
 * 4. A Non-Blocking Indicator System: Uses millis() and a state machine to drive Green and
 *    Red status LEDs (blinks in program mode, shows access granted/denied animations).
 * 5. Interactive Serial Shell & Simulator: Supports commands to dump logs, list cards, clear
 *    memory, and simulate card scans via hex entry (perfect for testing without hardware).
 *
 * EEPROM MEMORY MAP (1024 bytes available on ATmega328P):
 *   - [Address 0]       : System Config Magic Byte (0x7A = initialized, other = format on boot)
 *   - [Address 1]       : Master Card UID Length (1 byte)
 *   - [Address 2-9]     : Master Card UID bytes (8 bytes reserved)
 *   - [Address 10]      : Count of registered user cards (1 byte, max 10)
 *   - [Address 11-90]   : Registered User Cards Database (10 slots * 8 bytes each = 80 bytes)
 *                         (Each slot: 1 byte length + 7 bytes UID)
 *   - [Address 100-1019]: Access Log Ring Buffer (46 slots * 20 bytes each = 920 bytes)
 *
 * WIRING:
 *   - MFRC522 Pin  -> Arduino Uno Pin
 *     - 3.3V       -> 3.3V (WARNING: 5V will destroy the MFRC522 chip!)
 *     - GND        -> GND
 *     - RST (Reset)-> Pin 9
 *     - MISO       -> Pin 12
 *     - MOSI       -> Pin 11
 *     - SCK        -> Pin 13
 *     - SDA (SS)   -> Pin 10
 *   - DS3231 RTC Pin -> Arduino Uno Pin
 *     - VCC        -> 5V
 *     - GND        -> GND
 *     - SDA        -> Pin A4 (SDA)
 *     - SCL        -> Pin A5 (SCL)
 *   - Green LED (Access) -> Pin 5 (via 220Ω resistor to GND)
 *   - Red LED (Denied)   -> Pin 6 (via 220Ω resistor to GND)
 */

#include <EEPROM.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

// --- PIN DEFINITIONS ---
const int RST_PIN = 9;     // MFRC522 Reset
const int SS_PIN = 10;     // MFRC522 Slave Select (SDA)
const int ACCESS_LED = 5;  // Green LED (Access Granted)
const int DENIED_LED = 6;  // Red LED (Access Denied / Programming Mode)

// --- EEPROM DATABASE ADDRESSES ---
const int EEPROM_MAGIC_ADDR = 0;
const int EEPROM_MASTER_LEN_ADDR = 1;
const int EEPROM_MASTER_UID_ADDR = 2;
const int EEPROM_USER_COUNT_ADDR = 10;
const int EEPROM_USER_DB_START = 11;
const int MAX_USER_CARDS = 10;
const int USER_SLOT_SIZE = 8;  // 1 byte length + 7 bytes UID

// --- EEPROM LOG BUFFER CONFIGURATION ---
const int EEPROM_LOG_START = 100;
const int EEPROM_LOG_END = 1020;
const int MAX_UID_LEN = 7;

// --- I2C RTC ADDRESS ---
#define DS3231_I2C_ADDRESS 0x68

// --- DATA STRUCTURES ---
struct RTCDateTime {
  byte second;
  byte minute;
  byte hour;
  byte dayOfWeek;   // 1 = Sunday, 2 = Monday, etc.
  byte dayOfMonth;  // 1 - 31
  byte month;       // 1 - 12
  byte year;        // 0 - 99 (represents 2000 - 2099)
};

struct LogEntry {
  uint32_t logID;     // 4 bytes: Monotonically increasing record ID
  byte uid[7];        // 7 bytes: Card UID (standard MIFARE tags use 4 or 7 bytes)
  byte uidLength;     // 1 byte: Scanned card UID length
  byte hour;          // 1 byte: Timestamp Hour
  byte minute;        // 1 byte: Timestamp Minute
  byte second;        // 1 byte: Timestamp Second
  byte day;           // 1 byte: Timestamp Day
  byte month;         // 1 byte: Timestamp Month
  byte year;          // 1 byte: Timestamp Year (offset from 2000)
  byte status;        // 1 byte: 0 = Denied, 1 = Granted, 2 = Registered, 3 = Deregistered
  uint16_t checksum;  // 2 bytes: Integrity validation checksum
};  // Struct Size = 20 bytes

const int LOG_ENTRY_SIZE = sizeof(LogEntry);  // 20 bytes
const int NUM_LOG_SLOTS =
    (EEPROM_LOG_END - EEPROM_LOG_START) / LOG_ENTRY_SIZE;  // (1020 - 100)/20 = 46 slots

// --- STATE VARIABLES ---
MFRC522 mfrc522(SS_PIN, RST_PIN);
int activeSlotIndex = -1;  // Pointer to the newest log slot in EEPROM
uint32_t nextLogID = 1;    // Next log ID to assign

// Software RTC Fallback state
unsigned long softwareTimeOffset = 0;                // Accumulated software time in seconds
RTCDateTime bootDateTime = {0, 0, 12, 5, 4, 6, 26};  // Default: Thursday, June 4, 2026 12:00:00

// Non-blocking indicator animation state machine
enum IndicatorState {
  IND_IDLE,
  IND_ACCESS_GRANTED,
  IND_ACCESS_DENIED,
  IND_PROGRAM_MODE,
  IND_CARD_ADDED,
  IND_CARD_REMOVED
};
IndicatorState indState = IND_IDLE;
unsigned long indStart = 0;   // Animation timer start
unsigned long lastBlink = 0;  // Blinking toggle timer
bool ledState = false;        // Toggle helper for blinks

void setup() {
  Serial.begin(9600);
  Wire.begin();
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(ACCESS_LED, OUTPUT);
  pinMode(DENIED_LED, OUTPUT);
  digitalWrite(ACCESS_LED, LOW);
  digitalWrite(DENIED_LED, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 70: RFID Access Control & Wear-Leveling Logger"));
  Serial.println(F("=================================================="));

  // Scan EEPROM log buffer to locate the write head
  scanLogBuffer();

  // Print system initialization feedback
  checkMasterStatus();
  printShellMenu();
}

void loop() {
  // 1. Non-blocking update of indicator LEDs
  updateIndicators();

  // 2. Poll physical MFRC522 RFID reader
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    processCardScan(mfrc522.uid.uidByte, mfrc522.uid.size);
    mfrc522.PICC_HaltA();  // Halt PICC to prevent duplicate scans
  }

  // 3. Process Serial Shell commands & Simulation scans
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;  // Ignore formatting characters

    switch (cmd) {
      case 'r':
      case 'R':
        readAllLogs();
        break;
      case 'a':
      case 'A':
        printAuthorizedCards();
        break;
      case 'c':
      case 'C':
        formatEEPROM();
        break;
      case 's':
      case 'S':
        parseSimulatedCard();
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
//  CORE CARD SCAN PROCESSING
// =============================================================
void processCardScan(byte *uid, byte len) {
  byte magic = EEPROM.read(EEPROM_MAGIC_ADDR);

  // --- REGISTRATION PHASE (NO MASTER CARD SET) ---
  if (magic != 0x7A) {
    EEPROM.write(EEPROM_MAGIC_ADDR, 0x7A);
    EEPROM.write(EEPROM_MASTER_LEN_ADDR, len);
    for (int i = 0; i < len && i < 8; i++) {
      EEPROM.write(EEPROM_MASTER_UID_ADDR + i, uid[i]);
    }
    EEPROM.write(EEPROM_USER_COUNT_ADDR, 0);  // User count = 0

    Serial.println(F("[ACCESS CONTROL] Master Card successfully registered!"));
    Serial.print(F("[ACCESS CONTROL] Master Card UID:"));
    for (int i = 0; i < len; i++) {
      Serial.print(uid[i] < 0x10 ? " 0" : " ");
      Serial.print(uid[i], HEX);
    }
    Serial.println();

    // Trigger visual validation feedback
    indState = IND_CARD_ADDED;
    indStart = millis();
    return;
  }

  // --- TOGGLE PROGRAM MODE (MASTER CARD SCANNED) ---
  if (isMasterCard(uid, len)) {
    if (indState == IND_PROGRAM_MODE || indState == IND_CARD_ADDED ||
        indState == IND_CARD_REMOVED) {
      indState = IND_IDLE;
      Serial.println(F("[ACCESS CONTROL] Exited Program Mode. Returned to Access Control Mode."));
    } else {
      indState = IND_PROGRAM_MODE;
      lastBlink = millis();
      ledState = false;
      Serial.println(F("[ACCESS CONTROL] Entered Program Mode. Scan user cards to add/remove."));
    }
    return;
  }

  // --- PROGRAM MODE ACTIVE ---
  if (indState == IND_PROGRAM_MODE) {
    if (checkDatabase(uid, len)) {
      // Card exists: remove it (Deregistration)
      if (removeCardFromDatabase(uid, len)) {
        logAccess(uid, len, 3);  // Status 3 = Deregistered
        Serial.println(F("[ACCESS CONTROL] Card removed from database."));
        indState = IND_CARD_REMOVED;
        indStart = millis();
      } else {
        Serial.println(F("[ERROR] Failed to remove card from EEPROM."));
      }
    } else {
      // Card is new: add it (Registration)
      if (addCardToDatabase(uid, len)) {
        logAccess(uid, len, 2);  // Status 2 = Registered
        Serial.println(F("[ACCESS CONTROL] Card added to database."));
        indState = IND_CARD_ADDED;
        indStart = millis();
      } else {
        Serial.println(F("[ACCESS CONTROL] ERROR: Database full (Max 10 cards)!"));
        indState = IND_ACCESS_DENIED;
        indStart = millis();
      }
    }
    return;
  }

  // --- ACCESS CONTROL MODE (NORMAL OPERATION) ---
  if (checkDatabase(uid, len)) {
    Serial.println(F("[ACCESS CONTROL] -> AUTHORIZED KEY SCANNED. Access Granted!"));
    logAccess(uid, len, 1);  // Status 1 = Granted
    indState = IND_ACCESS_GRANTED;
    indStart = millis();
  } else {
    Serial.println(F("[ACCESS CONTROL] -> WARNING: UNAUTHORIZED KEY SCANNED. Access Denied!"));
    logAccess(uid, len, 0);  // Status 0 = Denied
    indState = IND_ACCESS_DENIED;
    indStart = millis();
  }
}

// =============================================================
//  EEPROM USER DATABASE OPERATIONS
// =============================================================
bool checkDatabase(const byte *uid, byte len) {
  byte count = EEPROM.read(EEPROM_USER_COUNT_ADDR);
  for (int i = 0; i < count; i++) {
    int addr = EEPROM_USER_DB_START + (i * USER_SLOT_SIZE);
    byte slotLen = EEPROM.read(addr);
    if (slotLen == len) {
      bool match = true;
      for (int j = 0; j < len; j++) {
        if (EEPROM.read(addr + 1 + j) != uid[j]) {
          match = false;
          break;
        }
      }
      if (match) return true;
    }
  }
  return false;
}

bool addCardToDatabase(const byte *uid, byte len) {
  byte count = EEPROM.read(EEPROM_USER_COUNT_ADDR);
  if (count >= MAX_USER_CARDS) return false;
  if (checkDatabase(uid, len)) return true;  // Already exists

  int addr = EEPROM_USER_DB_START + (count * USER_SLOT_SIZE);
  EEPROM.write(addr, len);
  for (int i = 0; i < len && i < 7; i++) {
    EEPROM.write(addr + 1 + i, uid[i]);
  }
  EEPROM.write(EEPROM_USER_COUNT_ADDR, count + 1);
  return true;
}

bool removeCardFromDatabase(const byte *uid, byte len) {
  byte count = EEPROM.read(EEPROM_USER_COUNT_ADDR);
  for (int i = 0; i < count; i++) {
    int addr = EEPROM_USER_DB_START + (i * USER_SLOT_SIZE);
    byte slotLen = EEPROM.read(addr);
    if (slotLen == len) {
      bool match = true;
      for (int j = 0; j < len; j++) {
        if (EEPROM.read(addr + 1 + j) != uid[j]) {
          match = false;
          break;
        }
      }
      if (match) {
        // Shift remaining cards down
        for (int k = i; k < count - 1; k++) {
          int src = EEPROM_USER_DB_START + ((k + 1) * USER_SLOT_SIZE);
          int dst = EEPROM_USER_DB_START + (k * USER_SLOT_SIZE);
          byte tempLen = EEPROM.read(src);
          EEPROM.write(dst, tempLen);
          for (int j = 0; j < 7; j++) {
            EEPROM.write(dst + 1 + j, EEPROM.read(src + 1 + j));
          }
        }
        EEPROM.write(EEPROM_USER_COUNT_ADDR, count - 1);
        return true;
      }
    }
  }
  return false;
}

bool isMasterCard(const byte *uid, byte len) {
  byte mLen = EEPROM.read(EEPROM_MASTER_LEN_ADDR);
  if (mLen != len) return false;
  for (int i = 0; i < len; i++) {
    if (EEPROM.read(EEPROM_MASTER_UID_ADDR + i) != uid[i]) return false;
  }
  return true;
}

// =============================================================
//  EEPROM WEAR-LEVELING ACCESS LOGGER
// =============================================================
uint16_t calculateLogChecksum(const LogEntry &entry) {
  uint16_t sum = 0;
  sum += (entry.logID & 0xFF) + ((entry.logID >> 8) & 0xFF) + ((entry.logID >> 16) & 0xFF) +
         ((entry.logID >> 24) & 0xFF);
  for (int i = 0; i < 7; i++) sum += entry.uid[i];
  sum += entry.uidLength;
  sum += entry.hour + entry.minute + entry.second + entry.day + entry.month + entry.year;
  sum += entry.status;
  return sum;
}

void scanLogBuffer() {
  uint32_t maxLogID = 0;
  int newestSlot = -1;
  int validCount = 0;

  for (int i = 0; i < NUM_LOG_SLOTS; i++) {
    LogEntry temp;
    int address = EEPROM_LOG_START + (i * LOG_ENTRY_SIZE);
    EEPROM.get(address, temp);

    uint16_t calculatedCheck = calculateLogChecksum(temp);

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
    Serial.print(F("[SYSTEM] Logs scanned. Found "));
    Serial.print(validCount);
    Serial.print(F(" valid records. Newest in Slot: "));
    Serial.print(activeSlotIndex);
    Serial.print(F(" (LogID: "));
    Serial.print(maxLogID);
    Serial.println(F(")"));
  } else {
    activeSlotIndex = -1;
    nextLogID = 1;
    Serial.println(F("[SYSTEM] Logs scanned. No valid records found (EEPROM empty)."));
  }
}

void logAccess(const byte *uid, byte len, byte status) {
  int targetSlot = (activeSlotIndex + 1) % NUM_LOG_SLOTS;
  int targetAddress = EEPROM_LOG_START + (targetSlot * LOG_ENTRY_SIZE);

  LogEntry entry;
  entry.logID = nextLogID;
  entry.uidLength = len;
  memset(entry.uid, 0, 7);
  for (int i = 0; i < len && i < 7; i++) {
    entry.uid[i] = uid[i];
  }

  RTCDateTime now;
  getSystemTime(now);
  entry.hour = now.hour;
  entry.minute = now.minute;
  entry.second = now.second;
  entry.day = now.dayOfMonth;
  entry.month = now.month;
  entry.year = now.year;
  entry.status = status;
  entry.checksum = calculateLogChecksum(entry);

  // EEPROM.put only writes if values differ to conserve write endurance
  EEPROM.put(targetAddress, entry);

  activeSlotIndex = targetSlot;
  nextLogID++;

  Serial.print(F("[LOG] Slot "));
  Serial.print(targetSlot);
  Serial.print(F(" | ID: "));
  Serial.print(entry.logID);
  Serial.print(F(" | Status: "));
  switch (status) {
    case 0:
      Serial.print(F("DENIED"));
      break;
    case 1:
      Serial.print(F("GRANTED"));
      break;
    case 2:
      Serial.print(F("REGISTERED"));
      break;
    case 3:
      Serial.print(F("DEREGISTERED"));
      break;
  }
  Serial.print(F(" | Timestamp: "));
  printDateTime(now);
  Serial.println();
}

void readAllLogs() {
  Serial.println(F("\n----------------- ACCESS LOG CHRONOLOGICAL DUMP -----------------"));
  int printCount = 0;
  uint32_t startID = (nextLogID > NUM_LOG_SLOTS) ? (nextLogID - NUM_LOG_SLOTS) : 1;

  for (uint32_t targetID = startID; targetID < nextLogID; targetID++) {
    for (int i = 0; i < NUM_LOG_SLOTS; i++) {
      LogEntry temp;
      int address = EEPROM_LOG_START + (i * LOG_ENTRY_SIZE);
      EEPROM.get(address, temp);

      if (temp.logID == targetID) {
        uint16_t calc = calculateLogChecksum(temp);
        if (temp.checksum == calc) {
          Serial.print(F("Slot "));
          if (i < 10) Serial.print(F("0"));
          Serial.print(i);
          Serial.print(F(" | ID: "));
          Serial.print(temp.logID);
          Serial.print(F(" | UID:"));
          for (int j = 0; j < temp.uidLength; j++) {
            Serial.print(temp.uid[j] < 0x10 ? " 0" : " ");
            Serial.print(temp.uid[j], HEX);
          }
          for (int p = temp.uidLength; p < 7; p++) Serial.print(F("   "));  // Padding

          Serial.print(F(" | Status: "));
          switch (temp.status) {
            case 0:
              Serial.print(F("DENIED      "));
              break;
            case 1:
              Serial.print(F("GRANTED     "));
              break;
            case 2:
              Serial.print(F("REGISTERED  "));
              break;
            case 3:
              Serial.print(F("DEREGISTERED"));
              break;
          }
          Serial.print(F(" | Time: 20"));
          if (temp.year < 10) Serial.print(F("0"));
          Serial.print(temp.year);
          Serial.print(F("-"));
          if (temp.month < 10) Serial.print(F("0"));
          Serial.print(temp.month);
          Serial.print(F("-"));
          if (temp.day < 10) Serial.print(F("0"));
          Serial.print(temp.day);
          Serial.print(F(" "));
          if (temp.hour < 10) Serial.print(F("0"));
          Serial.print(temp.hour);
          Serial.print(F(":"));
          if (temp.minute < 10) Serial.print(F("0"));
          Serial.print(temp.minute);
          Serial.print(F(":"));
          if (temp.second < 10) Serial.print(F("0"));
          Serial.print(temp.second);
          Serial.println();
          printCount++;
        }
      }
    }
  }
  if (printCount == 0) {
    Serial.println(F("No access logs found in memory."));
  }
  Serial.println(F("-----------------------------------------------------------------"));
}

// =============================================================
//  LOW-LEVEL I2C RTC READ & UTILITIES (DS3231)
// =============================================================
byte decToBcd(byte val) {
  return ((val / 10) << 4) | (val % 10);
}

byte bcdToDec(byte val) {
  return ((val >> 4) * 10) + (val & 0x0F);
}

bool readRTCDateTime(RTCDateTime *dt) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);  // Point to register 0 (seconds)
  if (Wire.endTransmission() != 0) {
    return false;  // RTC device not responding
  }

  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  if (Wire.available() >= 7) {
    dt->second = bcdToDec(Wire.read() & 0x7F);
    dt->minute = bcdToDec(Wire.read());
    dt->hour = bcdToDec(Wire.read() & 0x3F);  // 24-hr format
    dt->dayOfWeek = bcdToDec(Wire.read());
    dt->dayOfMonth = bcdToDec(Wire.read());
    dt->month = bcdToDec(Wire.read() & 0x1F);
    dt->year = bcdToDec(Wire.read());
    return true;
  }
  return false;
}

void getSystemTime(RTCDateTime &dt) {
  // Try reading physical RTC first
  if (readRTCDateTime(&dt)) {
    return;
  }

  // Fallback to Software RTC calculation using millis()
  unsigned long currentTotalSeconds = (millis() / 1000) + softwareTimeOffset;

  // Advance seconds, minutes, hours, days
  unsigned long seconds = (bootDateTime.second + currentTotalSeconds) % 60;
  unsigned long totalMinutes =
      bootDateTime.minute + (bootDateTime.second + currentTotalSeconds) / 60;
  unsigned long minutes = totalMinutes % 60;
  unsigned long totalHours = bootDateTime.hour + totalMinutes / 60;
  unsigned long hours = totalHours % 24;
  unsigned long daysIncrement = totalHours / 24;

  dt.second = seconds;
  dt.minute = minutes;
  dt.hour = hours;
  dt.dayOfMonth = bootDateTime.dayOfMonth + daysIncrement;  // Simple linear approximation
  dt.month = bootDateTime.month;
  dt.year = bootDateTime.year;
  dt.dayOfWeek = ((bootDateTime.dayOfWeek - 1 + daysIncrement) % 7) + 1;
}

void printDateTime(RTCDateTime dt) {
  Serial.print(F("20"));
  if (dt.year < 10) Serial.print(F("0"));
  Serial.print(dt.year);
  Serial.print(F("-"));
  if (dt.month < 10) Serial.print(F("0"));
  Serial.print(dt.month);
  Serial.print(F("-"));
  if (dt.dayOfMonth < 10) Serial.print(F("0"));
  Serial.print(dt.dayOfMonth);
  Serial.print(F(" "));
  if (dt.hour < 10) Serial.print(F("0"));
  Serial.print(dt.hour);
  Serial.print(F(":"));
  if (dt.minute < 10) Serial.print(F("0"));
  Serial.print(dt.minute);
  Serial.print(F(":"));
  if (dt.second < 10) Serial.print(F("0"));
  Serial.print(dt.second);
}

// =============================================================
//  NON-BLOCKING INDICATOR STATE MACHINE
// =============================================================
void updateIndicators() {
  unsigned long now = millis();

  // ACCESS GRANTED: Solid Green LED for 2000 ms
  if (indState == IND_ACCESS_GRANTED) {
    if (now - indStart < 2000) {
      digitalWrite(ACCESS_LED, HIGH);
      digitalWrite(DENIED_LED, LOW);
    } else {
      digitalWrite(ACCESS_LED, LOW);
      indState = IND_IDLE;
    }
    return;
  }

  // ACCESS DENIED: Red LED flashes 3 times rapidly (200ms per phase)
  if (indState == IND_ACCESS_DENIED) {
    unsigned long elapsed = now - indStart;
    int phase = elapsed / 200;
    if (phase < 6) {
      digitalWrite(ACCESS_LED, LOW);
      digitalWrite(DENIED_LED, (phase % 2 == 0) ? HIGH : LOW);
    } else {
      digitalWrite(DENIED_LED, LOW);
      indState = IND_IDLE;
    }
    return;
  }

  // CARD REGISTERED: Green LED flashes 3 times rapidly (150ms per phase)
  if (indState == IND_CARD_ADDED) {
    unsigned long elapsed = now - indStart;
    int phase = elapsed / 150;
    if (phase < 6) {
      digitalWrite(ACCESS_LED, (phase % 2 == 0) ? HIGH : LOW);
      digitalWrite(DENIED_LED, LOW);
    } else {
      digitalWrite(ACCESS_LED, LOW);
      // Return to Program Mode indicator if we were program-toggling
      byte magic = EEPROM.read(EEPROM_MAGIC_ADDR);
      if (magic == 0x7A && !isMasterCard(mfrc522.uid.uidByte, mfrc522.uid.size)) {
        indState = IND_PROGRAM_MODE;
        lastBlink = now;
        ledState = false;
      } else {
        indState = IND_IDLE;
      }
    }
    return;
  }

  // CARD DEREGISTERED: Green and Red LEDs flash together 3 times rapidly
  if (indState == IND_CARD_REMOVED) {
    unsigned long elapsed = now - indStart;
    int phase = elapsed / 150;
    if (phase < 6) {
      digitalWrite(ACCESS_LED, (phase % 2 == 0) ? HIGH : LOW);
      digitalWrite(DENIED_LED, (phase % 2 == 0) ? HIGH : LOW);
    } else {
      digitalWrite(ACCESS_LED, LOW);
      digitalWrite(DENIED_LED, LOW);
      indState = IND_PROGRAM_MODE;
      lastBlink = now;
      ledState = false;
    }
    return;
  }

  // PROGRAMMING MODE: Red LED blinks continuously (250ms interval)
  if (indState == IND_PROGRAM_MODE) {
    if (now - lastBlink >= 250) {
      lastBlink = now;
      ledState = !ledState;
      digitalWrite(DENIED_LED, ledState ? HIGH : LOW);
      digitalWrite(ACCESS_LED, LOW);
    }
    return;
  }

  // IDLE STATE: Both LEDs off
  if (indState == IND_IDLE) {
    digitalWrite(ACCESS_LED, LOW);
    digitalWrite(DENIED_LED, LOW);
  }
}

// =============================================================
//  SERIAL USER INTERFACE & SIMULATION UTILITIES
// =============================================================
void checkMasterStatus() {
  byte magic = EEPROM.read(EEPROM_MAGIC_ADDR);
  if (magic == 0x7A) {
    byte len = EEPROM.read(EEPROM_MASTER_LEN_ADDR);
    Serial.print(F("[SYSTEM] Master Card: SET | UID:"));
    for (int i = 0; i < len; i++) {
      byte b = EEPROM.read(EEPROM_MASTER_UID_ADDR + i);
      Serial.print(b < 0x10 ? " 0" : " ");
      Serial.print(b, HEX);
    }
    Serial.println();
    byte userCount = EEPROM.read(EEPROM_USER_COUNT_ADDR);
    Serial.print(F("[SYSTEM] Authorized User DB: "));
    Serial.print(userCount);
    Serial.println(F(" registered cards."));
  } else {
    Serial.println(F("[SYSTEM] Master Card: UNSET"));
    Serial.println(F("[SYSTEM] WARNING: Please scan a card to register it as the MASTER CARD."));
  }
}

void printAuthorizedCards() {
  byte magic = EEPROM.read(EEPROM_MAGIC_ADDR);
  byte count = EEPROM.read(EEPROM_USER_COUNT_ADDR);

  Serial.println(F("\n----------------- AUTHORIZED DATABASE -----------------"));
  if (magic != 0x7A) {
    Serial.println(F("System uninitialized. Master Card is not set."));
    Serial.println(F("-------------------------------------------------------"));
    return;
  }

  // Master Card
  byte masterLen = EEPROM.read(EEPROM_MASTER_LEN_ADDR);
  Serial.print(F("Master Card UID:"));
  for (int i = 0; i < masterLen; i++) {
    Serial.print(F(" "));
    byte b = EEPROM.read(EEPROM_MASTER_UID_ADDR + i);
    if (b < 0x10) Serial.print(F("0"));
    Serial.print(b, HEX);
  }
  Serial.println();

  // User Cards
  Serial.print(F("Registered User Cards: "));
  Serial.print(count);
  Serial.print(F(" / "));
  Serial.println(MAX_USER_CARDS);

  for (int i = 0; i < count; i++) {
    int addr = EEPROM_USER_DB_START + (i * USER_SLOT_SIZE);
    byte len = EEPROM.read(addr);
    Serial.print(F(" Slot "));
    Serial.print(i);
    Serial.print(F(": UID:"));
    for (int j = 0; j < len; j++) {
      Serial.print(F(" "));
      byte b = EEPROM.read(addr + 1 + j);
      if (b < 0x10) Serial.print(F("0"));
      Serial.print(b, HEX);
    }
    Serial.println();
  }
  Serial.println(F("-------------------------------------------------------"));
}

void formatEEPROM() {
  Serial.println(F("[SYSTEM] Wiping/formatting EEPROM database and logs. Please wait..."));
  for (int i = 0; i < 1024; i++) {
    EEPROM.update(i, 0xFF);
  }
  activeSlotIndex = -1;
  nextLogID = 1;
  indState = IND_IDLE;
  Serial.println(F("[SYSTEM] EEPROM cleared. System is uninitialized (ready for Master Card)."));
}

void parseSimulatedCard() {
  // Read hex sequence from serial buffer (e.g. "s DE AD BE EF")
  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) {
    Serial.println(F("[SHELL] Usage: s XX XX XX XX (up to 7 hex bytes)"));
    return;
  }

  byte simUid[7];
  byte simLen = 0;
  int index = 0;

  while (index < line.length() && simLen < 7) {
    // Skip whitespace
    while (index < line.length() && isSpace(line[index])) {
      index++;
    }
    if (index >= line.length()) break;

    // Extract next byte token
    String hexWord = "";
    while (index < line.length() && !isSpace(line[index])) {
      hexWord += line[index];
      index++;
    }

    if (hexWord.length() > 0) {
      simUid[simLen] = (byte)strtol(hexWord.c_str(), NULL, 16);
      simLen++;
    }
  }

  if (simLen > 0) {
    Serial.print(F("[SIMULATION] Scanned UID:"));
    for (int i = 0; i < simLen; i++) {
      Serial.print(F(" "));
      if (simUid[i] < 0x10) Serial.print(F("0"));
      Serial.print(simUid[i], HEX);
    }
    Serial.println();

    processCardScan(simUid, simLen);
  } else {
    Serial.println(F("[SHELL] Failed to parse hex values. Example: s DE AD BE EF"));
  }
}

void printShellMenu() {
  Serial.println(F("\n--- ACCESS CONTROL INTERACTIVE SHELL ---"));
  Serial.println(F(" 'r' : Read/dump all access logs in chronological order"));
  Serial.println(F(" 'a' : List Master Card and all authorized user UIDs"));
  Serial.println(F(" 'c' : Clear/Format entire EEPROM (wipe database and logs)"));
  Serial.println(F(" 's' : Simulate a card scan (Usage: s XX XX XX XX)"));
  Serial.println(F(" 'h' : Display this help menu"));
  Serial.println(F("-----------------------------------------"));
}

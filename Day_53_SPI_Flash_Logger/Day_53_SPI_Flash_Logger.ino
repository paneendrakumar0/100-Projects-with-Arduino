/*
 * 100 Projects with Arduino - Day 53
 * Project: SPI Flash Memory Logger (Winbond W25QXX Direct Registers)
 *
 * DESCRIPTION:
 * This project interfaces a Winbond W25Q64 (64 Megabits / 8 Megabytes) SPI Flash Memory chip
 * directly at the register level using hardware SPI. It implements raw commands for
 * page-programming, sector-erasing, status register polling, and data retrieval to create a custom
 * "flight-recorder" data logging stack from scratch, without external memory libraries.
 *
 * FLASH CHIP MEMORY MAP (W25Q64):
 * - Total Capacity: 8,388,608 Bytes (8 MB)
 * - 128 Blocks (64 KB each)
 * - 2048 Sectors (4 KB each - minimum erase unit)
 * - 32,768 Pages (256 Bytes each - maximum write unit)
 *
 * W25Q64 REGISTERS & INSTRUCTION CODES:
 * - CMD_WREN            (0x06): Write Enable (Must be called before every Erase/Program operation)
 * - CMD_WRDI            (0x04): Write Disable
 * - CMD_RDSR1           (0x05): Read Status Register 1 (Bit 0 BUSY indicates internal operations)
 * - CMD_READ_DATA       (0x03): Read Data Bytes starting at 24-bit address
 * - CMD_PAGE_PROGRAM    (0x02): Writes up to 256 bytes to a page (24-bit address)
 * - CMD_SECTOR_ERASE_4K (0x20): Erases a 4 KB sector (Fills all bits with 1s / 0xFF)
 * - CMD_JEDEC_ID        (0x9F): Reads manufacturer ID (0xEF), memory type (0x40), and capacity
 * (0x17)
 *
 * WIRING:
 * - W25Q64 Flash Module -> Arduino Uno
 *   - VCC -> 3.3V (W25QXX is a 3.3V chip! Do NOT connect to 5V power)
 *   - GND -> GND
 *   - CS  -> Pin 10 (SPI SS)
 *   - DO  -> Pin 12 (SPI MISO)
 *   - DI  -> Pin 11 (SPI MOSI via level shifter or 1k/2k resistor divider if powered at 5V logic)
 *   - CLK -> Pin 13 (SPI SCK via level shifter or 1k/2k resistor divider)
 */

#include <SPI.h>

// --- PIN DEFINITIONS ---
const int FLASH_CS_PIN = 10;
const int LED_INDICATOR_PIN = 9;

// --- W25Q64 INSTRUCTION SET ---
const uint8_t CMD_WREN = 0x06;
const uint8_t CMD_WRDI = 0x04;
const uint8_t CMD_RDSR1 = 0x05;
const uint8_t CMD_READ_DATA = 0x03;
const uint8_t CMD_PAGE_PROGRAM = 0x02;
const uint8_t CMD_SECTOR_ERASE_4K = 0x20;
const uint8_t CMD_JEDEC_ID = 0x9F;

// --- STRUCTURE FOR LOG DATA ---
// Aligning structure to 16 bytes for neat memory segmentation
struct TelemetryRecord {
  uint32_t timestamp;   // 4 bytes
  float temperature;    // 4 bytes
  uint16_t eventCount;  // 2 bytes
  char statusFlag[6];   // 6 bytes (e.g. "ACTIVE")
};

void setup() {
  Serial.begin(9600);

  pinMode(FLASH_CS_PIN, OUTPUT);
  digitalWrite(FLASH_CS_PIN, HIGH);  // SPI CS active low, set HIGH default

  pinMode(LED_INDICATOR_PIN, OUTPUT);
  digitalWrite(LED_INDICATOR_PIN, LOW);

  // Initialize Hardware SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  Serial.println(F("\n[FLASH] Initializing Winbond W25Q64 Chip..."));
  delay(100);

  // Step 1: Read JEDEC Identification numbers to verify hardware communication
  uint8_t mfgID, memType, capacity;
  readJEDECID(&mfgID, &memType, &capacity);

  Serial.println(F("[FLASH] JEDEC Identification Telemetry:"));
  Serial.print(F("  Manufacturer ID: 0x"));
  Serial.println(mfgID, HEX);
  Serial.print(F("  Memory Type:     0x"));
  Serial.println(memType, HEX);
  Serial.print(F("  Capacity ID:     0x"));
  Serial.println(capacity, HEX);

  // Verification check: Winbond Mfg ID = 0xEF, Capacity ID for W25Q64 = 0x17
  if (mfgID != 0xEF || capacity != 0x17) {
    Serial.println(F("[ERROR] JEDEC ID mismatch! Check SPI wiring/levels. Halt."));
    digitalWrite(LED_INDICATOR_PIN, HIGH);
    for (;;)
      ;
  }
  Serial.println(F("[FLASH] W25Q64 confirmed active. Proceeding with logging demo..."));

  // Step 2: Perform Sector Erase (Address 0x000000)
  // Note: Flash bytes must be erased (set to 0xFF) before programming new values.
  uint32_t testSectorAddress = 0x000000;
  Serial.println(F("[FLASH] Erasing 4 KB Sector at address 0x000000..."));
  eraseSector(testSectorAddress);
  Serial.println(F("[FLASH] Sector Erase Complete."));

  // Step 3: Write Telemetry Records to the Flash chip
  // We will log two records sequentially onto Page 0 (Address 0x000000)
  TelemetryRecord record1 = {1000, 24.57, 1, "OK_SYS"};
  TelemetryRecord record2 = {2000, 25.82, 2, "ALERT"};

  uint32_t writeAddr = 0x000000;
  Serial.println(F("[FLASH] Logging Record 1..."));
  writeBytes(writeAddr, (uint8_t*)&record1, sizeof(record1));

  writeAddr += sizeof(record1);  // Shift address to write next block
  Serial.println(F("[FLASH] Logging Record 2..."));
  writeBytes(writeAddr, (uint8_t*)&record2, sizeof(record2));

  // Step 4: Read logged records back from SPI memory
  Serial.println(F("\n[FLASH] Reading raw logged telemetry logs..."));

  TelemetryRecord readRecord1;
  TelemetryRecord readRecord2;

  uint32_t readAddr = 0x000000;
  readBytes(readAddr, (uint8_t*)&readRecord1, sizeof(readRecord1));

  readAddr += sizeof(readRecord1);
  readBytes(readAddr, (uint8_t*)&readRecord2, sizeof(readRecord2));

  // Step 5: Dump results to serial
  printRecord(1, readRecord1);
  printRecord(2, readRecord2);
}

void loop() {
  // Flash status indicator showing idle/ready state
  digitalWrite(LED_INDICATOR_PIN, HIGH);
  delay(100);
  digitalWrite(LED_INDICATOR_PIN, LOW);
  delay(900);
}

// --- LOW-LEVEL SPI W25QXX DRIVER COMMANDS ---

void readJEDECID(uint8_t* manufacturer, uint8_t* memType, uint8_t* capacity) {
  digitalWrite(FLASH_CS_PIN, LOW);
  SPI.transfer(CMD_JEDEC_ID);
  *manufacturer = SPI.transfer(0x00);
  *memType = SPI.transfer(0x00);
  *capacity = SPI.transfer(0x00);
  digitalWrite(FLASH_CS_PIN, HIGH);
}

uint8_t readStatusRegister() {
  digitalWrite(FLASH_CS_PIN, LOW);
  SPI.transfer(CMD_RDSR1);
  uint8_t status = SPI.transfer(0x00);
  digitalWrite(FLASH_CS_PIN, HIGH);
  return status;
}

void waitForReady() {
  // Bit 0 of Status Register 1 is BUSY (1 = busy, 0 = ready)
  while (readStatusRegister() & 0x01) {
    delayMicroseconds(100);  // Polling delay
  }
}

void writeEnable() {
  digitalWrite(FLASH_CS_PIN, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FLASH_CS_PIN, HIGH);
}

void eraseSector(uint32_t address) {
  waitForReady();
  writeEnable();

  digitalWrite(FLASH_CS_PIN, LOW);
  SPI.transfer(CMD_SECTOR_ERASE_4K);
  // Send 24-bit Address: [A23-A16, A15-A8, A7-A0]
  SPI.transfer((address >> 16) & 0xFF);
  SPI.transfer((address >> 8) & 0xFF);
  SPI.transfer(address & 0xFF);
  digitalWrite(FLASH_CS_PIN, HIGH);

  waitForReady();  // Wait for sector erase operation to finish (takes 40-100ms)
}

void writeBytes(uint32_t address, uint8_t* buffer, int length) {
  waitForReady();
  writeEnable();

  digitalWrite(FLASH_CS_PIN, LOW);
  SPI.transfer(CMD_PAGE_PROGRAM);
  // Send 24-bit Start Address
  SPI.transfer((address >> 16) & 0xFF);
  SPI.transfer((address >> 8) & 0xFF);
  SPI.transfer(address & 0xFF);

  // Stream data buffer
  for (int i = 0; i < length; i++) {
    SPI.transfer(buffer[i]);
  }
  digitalWrite(FLASH_CS_PIN, HIGH);

  waitForReady();  // Wait for page program to finish (takes ~0.5ms - 3ms)
}

void readBytes(uint32_t address, uint8_t* buffer, int length) {
  waitForReady();

  digitalWrite(FLASH_CS_PIN, LOW);
  SPI.transfer(CMD_READ_DATA);
  // Send 24-bit Start Address
  SPI.transfer((address >> 16) & 0xFF);
  SPI.transfer((address >> 8) & 0xFF);
  SPI.transfer(address & 0xFF);

  // Stream data back
  for (int i = 0; i < length; i++) {
    buffer[i] = SPI.transfer(0x00);
  }
  digitalWrite(FLASH_CS_PIN, HIGH);
}

// --- HELPER FORMATTED PRINTERS ---

void printRecord(int index, const TelemetryRecord& rec) {
  Serial.println(F("---------------------------------------------"));
  Serial.print(F("  Record #"));
  Serial.println(index);
  Serial.print(F("  Timestamp:   "));
  Serial.print(rec.timestamp);
  Serial.println(F(" ms"));
  Serial.print(F("  Temperature: "));
  Serial.print(rec.temperature, 2);
  Serial.println(F(" °C"));
  Serial.print(F("  Event Count: "));
  Serial.println(rec.eventCount);
  Serial.print(F("  Status:      "));
  Serial.println(rec.statusFlag);
  Serial.println(F("---------------------------------------------"));
}

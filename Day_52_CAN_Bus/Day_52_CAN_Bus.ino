/*
 * 100 Projects with Arduino - Day 52
 * Project: CAN Bus Controller (MCP2515 SPI Node Driver from scratch)
 *
 * DESCRIPTION:
 * This project implements a raw register-level SPI driver for the MCP2515 CAN Bus Controller.
 * It configures standard CAN 2.0B frame transmission and reception at a bus speed of 125 kbps
 * (standard for automotive/industrial networks) without any high-level CAN libraries.
 *
 * LOW-LEVEL SPI INSTRUCTION STACK:
 * 1. SPI Interface Setup: Drives Chip Select (CS) Pin 10, communicating using mode 0,0
 *    with a SPI clock frequency of 4 MHz (SPI_CLOCK_DIV4).
 * 2. MCP2515 SPI Command Set:
 *    - RESET (0xC0): Soft-resets the internal registers.
 *    - READ (0x03): Reads data from selected register addresses.
 *    - WRITE (0x02): Writes data to target register addresses.
 *    - BIT MODIFY (0x05): Changes individual bits of a register without altering others.
 *    - LOAD TX BUFFER (0x40): Rapidly writes payload bytes to TX buffer.
 *    - RTS (0x81): Request-to-Send instruction to trigger message transmission.
 * 3. Baud Rate Bit Timing (CNF Registers): Computes the time quanta (TQ) phases (Propagation,
 *    Phase1, Phase2 Segments) for a 16 MHz oscillator to achieve 125 kbps.
 * 4. Message Filtering: Configures RXB0 to accept all standard message IDs by disabling mask gates,
 *    storing incoming packets, and parsing fields (Standard Identifier, DLC, Data Bytes).
 *
 * CAN MESSAGE FRAME (Standard Frame 2.0B):
 * - Standard ID (11 bits): Identifier determining packet priority.
 * - DLC (Data Length Code): Integer (0-8) specifying byte count.
 * - Data Bytes: 0 to 8 bytes payload.
 *
 * WIRING:
 * - MCP2515 CAN Controller -> Arduino Uno
 *   - VCC -> 5V
 *   - GND -> GND
 *   - CS  -> Pin 10 (SPI SS)
 *   - SO  -> Pin 12 (SPI MISO)
 *   - SI  -> Pin 11 (SPI MOSI)
 *   - SCK -> Pin 13 (SPI SCK)
 *   - INT -> Pin 2  (Optional: Interrupt trigger pin, polled in our non-blocking code)
 * - CAN Transceiver (TJA1050/MCP2551)
 *   - CAN_H / CAN_L -> Differential CAN twisted-pair network bus
 */

#include <SPI.h>

// --- PIN DEFINITIONS ---
const int CAN_CS_PIN = 10;
const int LED_INDICATOR_PIN = 9;  // Status LED indicating packet activity
const int TX_TRIGGER_PIN = 7;     // Pull low (button press) to transmit a test frame

// --- MCP2515 INSTRUCTION CODES ---
const uint8_t INST_RESET = 0xC0;
const uint8_t INST_READ = 0x03;
const uint8_t INST_WRITE = 0x02;
const uint8_t INST_BIT_MODIFY = 0x05;
const uint8_t INST_RTS_TXB0 = 0x81;  // RTS for TX buffer 0

// --- MCP2515 REGISTER MAP ---
const uint8_t REG_CANSTAT = 0x0E;
const uint8_t REG_CANCTRL = 0x0F;
const uint8_t REG_CNF1 = 0x2A;
const uint8_t REG_CNF2 = 0x29;
const uint8_t REG_CNF3 = 0x28;
const uint8_t REG_CANINTF = 0x2C;   // Interrupt Flag Register
const uint8_t REG_RXB0CTRL = 0x60;  // RX Buffer 0 Control
const uint8_t REG_RXB0SIDH = 0x61;  // RX Buffer 0 Std Identifier High
const uint8_t REG_RXB0SIDL = 0x62;  // RX Buffer 0 Std Identifier Low
const uint8_t REG_RXB0DLC = 0x65;   // RX Buffer 0 Data Length Code
const uint8_t REG_RXB0D0 = 0x66;    // RX Buffer 0 Data Byte 0

const uint8_t REG_TXB0CTRL = 0x30;  // TX Buffer 0 Control
const uint8_t REG_TXB0SIDH = 0x31;  // TX Buffer 0 Std Identifier High
const uint8_t REG_TXB0SIDL = 0x32;  // TX Buffer 0 Std Identifier Low
const uint8_t REG_TXB0DLC = 0x35;   // TX Buffer 0 Data Length Code
const uint8_t REG_TXB0D0 = 0x36;    // TX Buffer 0 Data Byte 0

// Mode bits
const uint8_t MODE_CONFIG = 0x80;
const uint8_t MODE_NORMAL = 0x00;

// Update timing control
unsigned long lastTxTime = 0;
const unsigned long txIntervalMs = 2000;  // Auto-transmit a telemetry node frame every 2s

void setup() {
  Serial.begin(9600);

  pinMode(CAN_CS_PIN, OUTPUT);
  digitalWrite(CAN_CS_PIN, HIGH);  // CS active low, start HIGH

  pinMode(LED_INDICATOR_PIN, OUTPUT);
  pinMode(TX_TRIGGER_PIN, INPUT_PULLUP);

  digitalWrite(LED_INDICATOR_PIN, LOW);

  // Initialize Hardware SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  Serial.println(F("[CAN] Resetting MCP2515 controller..."));
  mcpReset();
  delay(50);

  // Read status to ensure chip woke up in CONFIG mode (0x80)
  uint8_t status = mcpReadRegister(REG_CANSTAT);
  Serial.print(F("[CAN] Post-Reset Status: 0x"));
  Serial.println(status, HEX);

  if ((status & 0xE0) != MODE_CONFIG) {
    Serial.println(F("[ERROR] MCP2515 failed to enter Config mode! Check wiring."));
    digitalWrite(LED_INDICATOR_PIN, HIGH);  // Fail light
    for (;;);
  }

  // --- CONFIGURE CAN BAUD RATE (125 kbps with 16 MHz Oscillator) ---
  // Bit time = 8 TQ (Time Quanta). BRP (Baud Rate Prescaler) = 7.
  // Formula: TQ = 2 * (BRP + 1) / Fosc = 2 * 8 / 16,000,000 = 1 microsecond.
  // Bit rate = 1 / (8 * TQ) = 1 / 8us = 125,000 bps = 125 kbps.
  // CNF1: SJW=1 TQ (00), BRP=7 (000111) -> 0x07
  // CNF2: BTLMODE=1 (1), SAM=0 (0), PHSEG1=3 TQ (010), PRSEG=2 TQ (001) -> 10010001 = 0x91
  // CNF3: SOF=0 (0), WAKFIL=0 (0), PHSEG2=2 TQ (001) -> 00000001 = 0x01
  mcpWriteRegister(REG_CNF1, 0x07);
  mcpWriteRegister(REG_CNF2, 0x91);
  mcpWriteRegister(REG_CNF3, 0x01);

  // Configure RXB0: Accept all standard frames, turn off filter masks
  // RXM1:0 = 11 (Turn filters/masks off, receive all valid messages)
  mcpWriteRegister(REG_RXB0CTRL, 0x60);

  // Switch MCP2515 to NORMAL mode to join the active CAN network
  mcpBitModify(REG_CANCTRL, 0xE0, MODE_NORMAL);
  delay(10);

  status = mcpReadRegister(REG_CANSTAT);
  if ((status & 0xE0) != MODE_NORMAL) {
    Serial.println(F("[ERROR] MCP2515 failed to enter Normal operating mode."));
    for (;;);
  }

  Serial.println(F("[CAN] Node initialized at 125 kbps. Operational."));
}

void loop() {
  // Check if a packet has been received in RX Buffer 0
  uint8_t intFlags = mcpReadRegister(REG_CANINTF);

  if (intFlags & 0x01) {  // RXB0IF is high: packet received!
    receiveCANFrame();
  }

  // Handle periodic telemetry transmit
  if (millis() - lastTxTime >= txIntervalMs) {
    lastTxTime = millis();

    // Read local sensor data
    int rawSensorVal = analogRead(A0);
    uint8_t payload[4];
    payload[0] = 0x55;                        // Telemetry header code
    payload[1] = (rawSensorVal >> 8) & 0xFF;  // Data High
    payload[2] = rawSensorVal & 0xFF;         // Data Low
    payload[3] = digitalRead(LED_INDICATOR_PIN);

    // Transmit message with standard priority ID 0x244
    transmitCANFrame(0x244, 4, payload);
  }

  // Handle manual trigger transmit
  if (digitalRead(TX_TRIGGER_PIN) == LOW) {
    uint8_t alertPayload[2] = {0xAA, 0x99};
    transmitCANFrame(0x100, 2, alertPayload);  // ID 0x100 (high priority alert ID)
    delay(250);                                // Debounce button trigger
  }
}

// --- SPI CONCURRENT COMMAND HANDLERS ---

void mcpReset() {
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(INST_RESET);
  digitalWrite(CAN_CS_PIN, HIGH);
}

uint8_t mcpReadRegister(uint8_t reg) {
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(INST_READ);
  SPI.transfer(reg);
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(CAN_CS_PIN, HIGH);
  return value;
}

void mcpWriteRegister(uint8_t reg, uint8_t val) {
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(INST_WRITE);
  SPI.transfer(reg);
  SPI.transfer(val);
  digitalWrite(CAN_CS_PIN, HIGH);
}

void mcpBitModify(uint8_t reg, uint8_t mask, uint8_t val) {
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(INST_BIT_MODIFY);
  SPI.transfer(reg);
  SPI.transfer(mask);
  SPI.transfer(val);
  digitalWrite(CAN_CS_PIN, HIGH);
}

// --- CAN PACKET TRANSMISSION PIPELINE ---

void transmitCANFrame(uint16_t id, uint8_t length, uint8_t *data) {
  // Wait until TXB0 becomes free (TXREQ bit in TXB0CTRL is low)
  while (mcpReadRegister(REG_TXB0CTRL) & 0x08) {
    // Spin lock wait
  }

  // Load CAN Standard Identifier into registers
  // 11-bit standard ID: ID10...ID3 -> TXB0SIDH; ID2...ID0 -> TXB0SIDL bits 7...5
  mcpWriteRegister(REG_TXB0SIDH, (uint8_t)(id >> 3));
  mcpWriteRegister(REG_TXB0SIDL, (uint8_t)((id & 0x07) << 5));

  // Load Data Length Code (DLC)
  mcpWriteRegister(REG_TXB0DLC, length & 0x0F);

  // Load packet data payload bytes
  for (int i = 0; i < length; i++) {
    mcpWriteRegister(REG_TXB0D0 + i, data[i]);
  }

  // Flash Tx light
  digitalWrite(LED_INDICATOR_PIN, HIGH);

  // Trigger RTS command for TX Buffer 0 to transmit on bus
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(INST_RTS_TXB0);
  digitalWrite(CAN_CS_PIN, HIGH);

  delayMicroseconds(50);  // Small settle
  digitalWrite(LED_INDICATOR_PIN, LOW);

  // Log event
  Serial.print(F("[CAN TX] Frame ID: 0x"));
  Serial.print(id, HEX);
  Serial.print(F(" | DLC: "));
  Serial.print(length);
  Serial.print(F(" | Data: "));
  for (int i = 0; i < length; i++) {
    Serial.print(F("0x"));
    Serial.print(data[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
}

// --- CAN PACKET RECEPTION PIPELINE ---

void receiveCANFrame() {
  uint16_t id = 0;
  uint8_t length = 0;
  uint8_t data[8];

  // Read Standard Identifier Registers
  uint8_t sidh = mcpReadRegister(REG_RXB0SIDH);
  uint8_t sidl = mcpReadRegister(REG_RXB0SIDL);
  id = (sidh << 3) | (sidl >> 5);

  // Read DLC
  length = mcpReadRegister(REG_RXB0DLC) & 0x0F;

  // Read payload bytes
  for (int i = 0; i < length; i++) {
    data[i] = mcpReadRegister(REG_RXB0D0 + i);
  }

  // Clear the RXB0IF flag so MCP2515 can accept the next packet
  mcpBitModify(REG_CANINTF, 0x01, 0x00);

  // Flash RX indicator double pulse
  digitalWrite(LED_INDICATOR_PIN, HIGH);
  delay(10);
  digitalWrite(LED_INDICATOR_PIN, LOW);

  // Log parsed data to PC
  Serial.print(F("[CAN RX] Frame ID: 0x"));
  Serial.print(id, HEX);
  Serial.print(F(" | DLC: "));
  Serial.print(length);
  Serial.print(F(" | Payload: "));
  for (int i = 0; i < length; i++) {
    Serial.print(F("0x"));
    Serial.print(data[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
}

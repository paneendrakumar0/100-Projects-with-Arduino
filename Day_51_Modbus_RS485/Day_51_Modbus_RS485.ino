/*
 * 100 Projects with Arduino - Day 51
 * Project: Industrial Modbus RTU Slave Node (RS485 Half-Duplex Parser from scratch)
 *
 * DESCRIPTION:
 * This project implements a fully compliant Modbus RTU slave node on an Arduino Uno using
 * a MAX485 transceiver. The entire communication stack, including raw byte framing,
 * silent interval detection (3.5-character times), and CRC-16 validation, is written
 * from scratch without external libraries.
 *
 * EMBEDDED PROTOCOL STACK:
 * 1. Half-Duplex Transceiver Driver: Directs a MAX485 transceiver using a DE/RE flow pin,
 *    switching the lines to high-impedance listening (RX) or active drive (TX) modes.
 * 2. Modbus RTU Frame Delimiter: Implements a non-blocking timeout parser. Since Modbus RTU
 *    demarcates packets using a silent interval of 3.5 character times (approx. 4ms at 9600 baud),
 *    the receiver triggers packet processing when the serial line is idle for >= 5ms.
 * 3. CRC-16 verification: Computes standard cyclic redundancy check coefficients using the
 *    generator polynomial 0xA001.
 * 4. Modbus Functions Supported:
 *    - Function Code 0x03 (Read Holding Registers): Reports virtual registers mapping local
 *      sensor states (e.g. Register 0x0000 = Analog A0 LDR value, Register 0x0001 = D13 LED
 * Status).
 *    - Function Code 0x06 (Write Single Register): Accepts control commands to actuate relays
 *      or toggles (e.g. Writing 1 or 0 to Register 0x0001 triggers LED control pin).
 *
 * WIRING:
 * - MAX485 Module -> Arduino Uno
 *   - VCC -> 5V
 *   - GND -> GND
 *   - RO (Receiver Output) -> Pin 2 (SoftwareSerial RX)
 *   - DI (Driver Input)    -> Pin 3 (SoftwareSerial TX)
 *   - DE/RE (Tied together)-> Pin 4 (RS485 Flow Control Pin - High=TX, Low=RX)
 *   - A / B                -> Industrial RS485 twisted-pair bus lines
 */

#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const int RS485_RX_PIN = 2;
const int RS485_TX_PIN = 3;
const int RS485_FLOW_PIN = 4;
const int SENSOR_INPUT_PIN = A0;
const int LED_INDICATOR_PIN = 13;

// --- MODBUS NODE CONFIG ---
const uint8_t SLAVE_ADDRESS = 1;  // Unique Modbus Slave ID

// --- VIRTUAL HOLDING REGISTERS MAP ---
// Register 0x0000: Raw sensor reading (ReadOnly)
// Register 0x0001: LED control pin state (Read/Write)
uint16_t holdingRegisters[2] = {0, 0};

SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN);

// --- PROTOCOL BUFFER VARIABLES ---
const int MAX_FRAME_LEN = 64;
uint8_t rxBuffer[MAX_FRAME_LEN];
int rxIndex = 0;

unsigned long lastCharTimeMs = 0;
const unsigned long MODBUS_SILENT_INTERVAL_MS = 5;  // Idle time indicating end of frame

void setup() {
  Serial.begin(9600);       // USB Serial for diagnostics logging
  rs485Serial.begin(9600);  // RS485 serial communication (9600 Baud, 8N1)

  pinMode(RS485_FLOW_PIN, OUTPUT);
  pinMode(LED_INDICATOR_PIN, OUTPUT);

  // Set MAX485 to default Listening (Receive) Mode
  digitalWrite(RS485_FLOW_PIN, LOW);
  digitalWrite(LED_INDICATOR_PIN, LOW);

  Serial.print(F("[MODBUS] Slave ID: "));
  Serial.print(SLAVE_ADDRESS);
  Serial.println(F(" active. Listening on RS485..."));
}

void loop() {
  // Update ReadOnly Sensor register with current analog input (A0)
  holdingRegisters[0] = analogRead(SENSOR_INPUT_PIN);

  // Read incoming characters from MAX485 transceiver
  while (rs485Serial.available() > 0) {
    uint8_t b = rs485Serial.read();

    // Check if this is the start of a new packet after a long silent gap
    if (millis() - lastCharTimeMs > MODBUS_SILENT_INTERVAL_MS) {
      rxIndex = 0;
    }

    lastCharTimeMs = millis();

    if (rxIndex < MAX_FRAME_LEN) {
      rxBuffer[rxIndex++] = b;
    }
  }

  // Check if a complete packet has arrived by looking for the silent idle gap
  if (rxIndex > 0 && (millis() - lastCharTimeMs >= MODBUS_SILENT_INTERVAL_MS)) {
    // Process the frame
    processModbusFrame();
    rxIndex = 0;  // Reset buffer pointer
  }
}

// --- MODBUS RTU FRAME PARSER ---

void processModbusFrame() {
  // Minimum valid Modbus RTU packet length is 8 bytes:
  // [Slave ID, Func Code, Addr High, Addr Low, Data High, Data Low, CRC Low, CRC High]
  if (rxIndex < 8) return;

  // Validate Slave Address
  if (rxBuffer[0] != SLAVE_ADDRESS) return;

  // Verify Frame Checksum (CRC-16)
  uint16_t calculatedCRC = calculateCRC16(rxBuffer, rxIndex - 2);
  uint16_t receivedCRC = (rxBuffer[rxIndex - 1] << 8) | rxBuffer[rxIndex - 2];

  if (calculatedCRC != receivedCRC) {
    Serial.println(F("[MODBUS] Frame dropped: Checksum error (CRC-16 invalid)."));
    return;
  }

  // Process function code
  uint8_t functionCode = rxBuffer[1];
  switch (functionCode) {
    case 0x03:  // Read Holding Registers
      handleReadHoldingRegisters();
      break;

    case 0x06:  // Write Single Register
      handleWriteSingleRegister();
      break;

    default:
      // Function not supported - send exception response
      sendExceptionResponse(functionCode, 0x01);  // 0x01 = Illegal Function Exception
      break;
  }
}

// --- FUNCTION CODE 0x03 (READ HOLDING REGISTERS) ---

void handleReadHoldingRegisters() {
  // Request structure: [SlaveID, 0x03, StartAddrH, StartAddrL, RegQtyH, RegQtyL, CRCL, CRCH]
  uint16_t startAddress = (rxBuffer[2] << 8) | rxBuffer[3];
  uint16_t regQuantity = (rxBuffer[4] << 8) | rxBuffer[5];

  // Validate requested address range
  if (startAddress + regQuantity > 2) {
    sendExceptionResponse(0x03, 0x02);  // 0x02 = Illegal Data Address Exception
    return;
  }

  // Build Response frame
  // Frame: [SlaveID, FuncCode, ByteCount, Reg1H, Reg1L, ..., CRCL, CRCH]
  uint8_t txBuffer[20];
  txBuffer[0] = SLAVE_ADDRESS;
  txBuffer[1] = 0x03;
  txBuffer[2] = regQuantity * 2;  // Each register is 2 bytes

  int txIdx = 3;
  for (int i = 0; i < regQuantity; i++) {
    uint16_t regValue = holdingRegisters[startAddress + i];
    txBuffer[txIdx++] = (regValue >> 8) & 0xFF;
    txBuffer[txIdx++] = regValue & 0xFF;
  }

  // Compute and append CRC
  uint16_t crc = calculateCRC16(txBuffer, txIdx);
  txBuffer[txIdx++] = crc & 0xFF;
  txBuffer[txIdx++] = (crc >> 8) & 0xFF;

  // Send packet over RS485 half-duplex line
  transmitFrame(txBuffer, txIdx);

  // Log event
  Serial.print(F("[MODBUS] Read Request. Sent "));
  Serial.print(regQuantity);
  Serial.println(F(" register(s)."));
}

// --- FUNCTION CODE 0x06 (WRITE SINGLE REGISTER) ---

void handleWriteSingleRegister() {
  // Request structure: [SlaveID, 0x06, RegAddrH, RegAddrL, ValueH, ValueL, CRCL, CRCH]
  uint16_t regAddress = (rxBuffer[2] << 8) | rxBuffer[3];
  uint16_t regValue = (rxBuffer[4] << 8) | rxBuffer[5];

  // Validate address
  if (regAddress >= 2) {
    sendExceptionResponse(0x06, 0x02);  // Illegal Data Address Exception
    return;
  }

  // Validate LED status register write values (Register 1 only accepts 0 or 1)
  if (regAddress == 1 && regValue > 1) {
    sendExceptionResponse(0x06, 0x03);  // 0x03 = Illegal Data Value Exception
    return;
  }

  // Update register values and apply actuation
  holdingRegisters[regAddress] = regValue;
  if (regAddress == 1) {
    digitalWrite(LED_INDICATOR_PIN, regValue == 1 ? HIGH : LOW);
    Serial.print(F("[MODBUS] Write Request: LED status -> "));
    Serial.println(regValue == 1 ? F("HIGH") : F("LOW"));
  }

  // Modbus FC06 echoes the request packet directly back as the confirmation response
  transmitFrame(rxBuffer, rxIndex);
}

// --- MODBUS EXCEPTION HANDLING ---

void sendExceptionResponse(uint8_t functionCode, uint8_t exceptionCode) {
  // Frame: [SlaveID, FuncCode | 0x80, ExceptionCode, CRCL, CRCH]
  uint8_t txBuffer[5];
  txBuffer[0] = SLAVE_ADDRESS;
  txBuffer[1] = functionCode | 0x80;  // Set MSB bit high to signal error
  txBuffer[2] = exceptionCode;

  uint16_t crc = calculateCRC16(txBuffer, 3);
  txBuffer[3] = crc & 0xFF;
  txBuffer[4] = (crc >> 8) & 0xFF;

  transmitFrame(txBuffer, 5);
  Serial.print(F("[MODBUS] Exception thrown. Code: 0x"));
  Serial.println(exceptionCode, HEX);
}

// --- PHYSICAL TRANSCEIVER CONTROL ---

void transmitFrame(uint8_t *buffer, int length) {
  // Step 1: Set MAX485 pin to HIGH (Transmit Mode)
  digitalWrite(RS485_FLOW_PIN, HIGH);
  delayMicroseconds(50);  // Settle electrical state

  // Step 2: Write buffer to RS485 Serial port
  rs485Serial.write(buffer, length);

  // Step 3: Wait for serial output buffer to clear completely
  rs485Serial.flush();
  delayMicroseconds(100);  // Extra safety guard for last bit transit

  // Step 4: Set MAX485 pin back to LOW (Receive Mode)
  digitalWrite(RS485_FLOW_PIN, LOW);
}

// --- CRC-16 GENERATOR ALGORITHM ---

uint16_t calculateCRC16(uint8_t *buffer, int length) {
  uint16_t crc = 0xFFFF;  // Initialize Modbus CRC register
  for (int i = 0; i < length; i++) {
    crc ^= buffer[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;  // Polynomial XOR (Reflected 0x8005)
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

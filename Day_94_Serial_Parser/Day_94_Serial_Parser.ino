/*
 * 100 Projects with Arduino - Day 94
 * Project: Robust Serial Telemetry Parser (FSM Packet Framing & Checksum Validation)
 *
 * DESCRIPTION:
 * This project implements a production-grade Serial Packet Parser using a Finite State Machine
 * (FSM). When communicating between a PC (running ROS, Python, or custom GUI) and a
 * microcontroller, raw string-based communication (like Serial.parseInt()) is slow,
 * non-deterministic, and prone to corruption.
 *
 * To ensure data integrity, we frame binary data packets using a standard protocol:
 *   [SOF (0x02)] [Length] [Command ID] [Payload Bytes...] [Checksum] [EOF (0x03)]
 *
 * Packet Breakdown:
 * - Start of Frame (SOF): 0x02 (ASCII STX)
 * - Length: 1 byte (specifies number of bytes from Command ID to last Payload byte)
 * - Command ID: 1 byte (tells the Arduino what to do)
 * - Payload: 0 to 250 bytes of raw data
 * - Checksum: 1 byte (XOR sum of Command ID and all Payload bytes)
 * - End of Frame (EOF): 0x03 (ASCII ETX)
 *
 * FINITE STATE MACHINE STATES:
 * - STATE_WAIT_SOF : Wait for 0x02 start byte.
 * - STATE_READ_LEN : Read the length of the packet payload.
 * - STATE_READ_DATA: Read the data bytes (Command + Payload).
 * - STATE_READ_CS  : Read the checksum byte and validate.
 * - STATE_READ_EOF : Verify the final byte is 0x03.
 *
 * CLI INTERACTIVE TESTER:
 * To make this project testable without a Python script, the Serial Monitor CLI can generate
 * pre-framed test packets (e.g. to control an LED or set simulated motor speeds) and feed
 * them byte-by-byte into the parser, printing the state transitions live.
 *
 * WIRING:
 * - No external components required. Runs over standard Serial interface.
 */

// --- FSM STATE ENUMERATION ---
enum ParserState { STATE_WAIT_SOF, STATE_READ_LEN, STATE_READ_DATA, STATE_READ_CS, STATE_READ_EOF };

// --- PACKET CONFIGURATION ---
const uint8_t SOF_BYTE = 0x02;  // Start of Frame
const uint8_t EOF_BYTE = 0x03;  // End of Frame
const int MAX_PAYLOAD_SIZE = 64;

// --- STATE VARIABLES ---
ParserState currentState = STATE_WAIT_SOF;
uint8_t rxBuffer[MAX_PAYLOAD_SIZE];
uint8_t rxIndex = 0;
uint8_t packetLength = 0;
uint8_t receivedChecksum = 0;
uint8_t calculatedChecksum = 0;

// --- PIN DEFINITIONS ---
const int LED_PIN = 13;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 94: Robust Serial Telemetry Parser (FSM)"));
  Serial.println(F("=================================================="));

  printMenu();
}

void loop() {
  // 1. Process incoming bytes from the hardware Serial buffer
  while (Serial.available() > 0) {
    uint8_t incomingByte = Serial.read();

    // In human-interactive CLI mode, we intercept letters to trigger test generators.
    // If it's a binary command, we parse it.
    if (currentState == STATE_WAIT_SOF && isAlphaCommand(incomingByte)) {
      handleCLICommand((char)incomingByte);
    } else {
      processIncomingByte(incomingByte);
    }
  }
}

/**
 * Checks if the byte is a human menu command.
 */
bool isAlphaCommand(uint8_t b) {
  return (b == 'l' || b == 'L' || b == 'm' || b == 'M' || b == 'e' || b == 'E' || b == 'h' ||
          b == 'H' || b == 'r' || b == 'R');
}

// =============================================================
//  FINITE STATE MACHINE PARSER
// =============================================================

/**
 * Non-blocking FSM Parser. Processes one byte at a time.
 */
void processIncomingByte(uint8_t val) {
  switch (currentState) {
    case STATE_WAIT_SOF:
      if (val == SOF_BYTE) {
        currentState = STATE_READ_LEN;
        rxIndex = 0;
        calculatedChecksum = 0;
        Serial.println(F("[FSM] SOF Detected (0x02) -> Entering STATE_READ_LEN"));
      }
      break;

    case STATE_READ_LEN:
      packetLength = val;
      if (packetLength == 0 || packetLength > MAX_PAYLOAD_SIZE) {
        // Guard against buffer overflow or invalid packets
        Serial.println(F("[FSM] Error: Invalid packet length! Resetting..."));
        currentState = STATE_WAIT_SOF;
      } else {
        currentState = STATE_READ_DATA;
        Serial.print(F("[FSM] Packet Length: "));
        Serial.print(packetLength);
        Serial.println(F(" bytes -> Entering STATE_READ_DATA"));
      }
      break;

    case STATE_READ_DATA:
      rxBuffer[rxIndex++] = val;
      calculatedChecksum ^= val;  // Cumulative XOR checksum

      if (rxIndex >= packetLength) {
        currentState = STATE_READ_CS;
        Serial.println(F("[FSM] All data bytes received -> Entering STATE_READ_CS"));
      }
      break;

    case STATE_READ_CS:
      receivedChecksum = val;
      currentState = STATE_READ_EOF;
      Serial.print(F("[FSM] Checksum Received: 0x"));
      Serial.print(receivedChecksum, HEX);
      Serial.print(F(", Calculated: 0x"));
      Serial.print(calculatedChecksum, HEX);
      Serial.println(F(" -> Entering STATE_READ_EOF"));
      break;

    case STATE_READ_EOF:
      if (val == EOF_BYTE) {
        // Validate Checksum
        if (receivedChecksum == calculatedChecksum) {
          Serial.println(F("[FSM] SUCCESS: Valid packet checksum & EOF verified!"));
          executePacketCommand();
        } else {
          Serial.println(F("[FSM] CHECKSUM ERROR: Discarding packet."));
        }
      } else {
        Serial.print(F("[FSM] EOF ERROR: Expected 0x03, received 0x"));
        Serial.println(val, HEX);
      }
      // Return to search for next packet
      currentState = STATE_WAIT_SOF;
      break;
  }
}

// =============================================================
//  PACKET COMMAND EXECUTION
// =============================================================
void executePacketCommand() {
  uint8_t cmdID = rxBuffer[0];
  Serial.print(F("[COMMAND] Executing CMD ID: 0x"));
  Serial.println(cmdID, HEX);

  switch (cmdID) {
    case 0x10:  // Command: Toggle LED
      if (packetLength >= 2) {
        uint8_t state = rxBuffer[1];
        digitalWrite(LED_PIN, state ? HIGH : LOW);
        Serial.print(F("  -> Set LED State to: "));
        Serial.println(state ? F("ON") : F("OFF"));
      }
      break;

    case 0x20:  // Command: Set Motor Speeds
      if (packetLength >= 3) {
        // Payload: Left motor speed (signed 8-bit), Right motor speed (signed 8-bit)
        int8_t leftSpeed = (int8_t)rxBuffer[1];
        int8_t rightSpeed = (int8_t)rxBuffer[2];
        Serial.println(F("  -> Actuator Update:"));
        Serial.print(F("     Left Motor  Speed: "));
        Serial.print(leftSpeed);
        Serial.println(F(" RPM"));
        Serial.print(F("     Right Motor Speed: "));
        Serial.print(rightSpeed);
        Serial.println(F(" RPM"));
      }
      break;

    default:
      Serial.println(F("  -> Error: Unknown Command ID."));
      break;
  }
  Serial.println(F("--------------------------------------------------"));
}

// =============================================================
//  TEST PACKET GENERATORS (CLI MODE)
// =============================================================

/**
 * Handles menu keyboard inputs and feeds generated packets into our own FSM.
 */
void handleCLICommand(char cmd) {
  switch (cmd) {
    case 'l':
    case 'L':
      // Generate packet: Toggle LED ON
      // Format: SOF(0x02) Len(2) Cmd(0x10) State(1) CS(0x11) EOF(0x03)
      Serial.println(F("\n[TESTER] Generating Packet: Turn LED ON"));
      feedSimulatedPacket(0x10, 1);
      break;

    case 'm':
    case 'M':
      // Generate packet: Toggle LED OFF
      // Format: SOF(0x02) Len(2) Cmd(0x10) State(0) CS(0x10) EOF(0x03)
      Serial.println(F("\n[TESTER] Generating Packet: Turn LED OFF"));
      feedSimulatedPacket(0x10, 0);
      break;

    case 'e':
    case 'E':
      // Generate packet: Set Motor Speeds (Left = +80 RPM, Right = -50 RPM)
      // Format: SOF(0x02) Len(3) Cmd(0x20) L_Speed(80) R_Speed(-50/206) CS(XOR) EOF(0x03)
      Serial.println(F("\n[TESTER] Generating Packet: Set Motor Speeds (+80, -50)"));
      feedSimulatedMotorPacket(80, -50);
      break;

    case 'r':
    case 'R':
      // Generate CORRUPTED packet to test validation
      Serial.println(F("\n[TESTER] Generating Corrupted Checksum Packet"));
      feedCorruptedPacket();
      break;

    case 'h':
    case 'H':
      printMenu();
      break;
  }
}

/**
 * Simulates sending bytes over the physical wire, passing them to the FSM.
 */
void feedSimulatedPacket(uint8_t cmd, uint8_t payloadByte) {
  uint8_t len = 2;  // Cmd ID + 1 byte payload
  uint8_t cs = cmd ^ payloadByte;

  uint8_t packet[6] = {SOF_BYTE, len, cmd, payloadByte, cs, EOF_BYTE};

  // Print hex representation of package
  Serial.print(F(" Raw Bytes Sent: "));
  for (int i = 0; i < 6; i++) {
    if (packet[i] < 16) Serial.print('0');
    Serial.print(packet[i], HEX);
    Serial.print(' ');
  }
  Serial.println(F("\n--- Starting Parse Loop ---"));

  // Feed bytes to parser
  for (int i = 0; i < 6; i++) {
    processIncomingByte(packet[i]);
  }
}

void feedSimulatedMotorPacket(int8_t left, int8_t right) {
  uint8_t len = 3;
  uint8_t cmd = 0x20;
  uint8_t uLeft = (uint8_t)left;
  uint8_t uRight = (uint8_t)right;
  uint8_t cs = cmd ^ uLeft ^ uRight;

  uint8_t packet[7] = {SOF_BYTE, len, cmd, uLeft, uRight, cs, EOF_BYTE};

  Serial.print(F(" Raw Bytes Sent: "));
  for (int i = 0; i < 7; i++) {
    if (packet[i] < 16) Serial.print('0');
    Serial.print(packet[i], HEX);
    Serial.print(' ');
  }
  Serial.println(F("\n--- Starting Parse Loop ---"));

  for (int i = 0; i < 7; i++) {
    processIncomingByte(packet[i]);
  }
}

void feedCorruptedPacket() {
  uint8_t len = 2;
  uint8_t cmd = 0x10;
  uint8_t payload = 1;
  uint8_t wrongCs = 0xFF;  // Real CS is 0x11

  uint8_t packet[6] = {SOF_BYTE, len, cmd, payload, wrongCs, EOF_BYTE};

  Serial.print(F(" Raw Bytes Sent: "));
  for (int i = 0; i < 6; i++) {
    if (packet[i] < 16) Serial.print('0');
    Serial.print(packet[i], HEX);
    Serial.print(' ');
  }
  Serial.println(F("\n--- Starting Parse Loop ---"));

  for (int i = 0; i < 6; i++) {
    processIncomingByte(packet[i]);
  }
}

void printMenu() {
  Serial.println(F("\n--- TELEMETRY PACKET INTERACTIVE CLI ---"));
  Serial.println(F(" Send keyboard keys to generate binary structured packets:"));
  Serial.println(F(" 'l' : Send [LED ON] Packet  -> (02 02 10 01 11 03)"));
  Serial.println(F(" 'm' : Send [LED OFF] Packet -> (02 02 10 00 10 03)"));
  Serial.println(F(" 'e' : Send [MOTOR SPEEDS]   -> (02 03 20 50 CE 9E 03)"));
  Serial.println(F(" 'r' : Send [CORRUPT] Packet -> Checksum error injection"));
  Serial.println(F(" 'h' : Display this help menu"));
  Serial.println(F("----------------------------------------\n"));
}

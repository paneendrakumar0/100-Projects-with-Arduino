/*
 * 100 Projects with Arduino - Day 100 (The Capstone)
 * Project: Micro-ROS Serial Bridge Node (Differential Drive Twist Subscriber & Odometry Publisher)
 *
 * DESCRIPTION:
 * This is the grand capstone project of the 100-day Arduino Masterclass! We build a custom
 * Micro-ROS Serial Bridge Node. This sketch allows an Arduino to communicate directly with a
 * host PC running ROS (Robot Operating System) over a robust serial connection.
 *
 * The system implements two core ROS node operations:
 * 1. Subscriber ("/cmd_vel"): Listens for incoming Twist velocity commands (Linear X, Angular Z).
 *    Parses the command, executes differential drive kinematics, and calculates left/right
 *    wheel velocity targets (RPM).
 * 2. Publisher ("/odom"): Periodically (at 10 Hz) calculates and packs the robot's local 2D
 *    odometry state (X, Y, Theta) and transmits it to the PC in a structured binary frame.
 *
 * THE PROTOCOL PACKET FRAMING:
 * To ensure high reliability, data is packaged in framed binary packets:
 *   [SOF (0x02)] [Length] [Msg Type] [Payload...] [Checksum] [EOF (0x03)]
 *
 * Message Types:
 * - 0x10 : /cmd_vel (PC -> Arduino). Payload: 4 bytes (16-bit signed Linear X, 16-bit signed
 * Angular Z, scaled by 1000)
 * - 0x20 : /odom (Arduino -> PC). Payload: 6 bytes (16-bit signed X, 16-bit signed Y, 16-bit signed
 * Theta, scaled by 100)
 *
 * INTERACTIVE ROS MASTER SIMULATOR:
 * The Serial CLI acts as a ROS Master Node running on a PC. You can publish velocity commands
 * (e.g. "v 0.25 -0.50" to set linear velocity = 0.25 m/s, angular = -0.50 rad/s), and watch
 * the Arduino parse the packet, adjust motor goals, update its internal coordinate frames,
 * and publish /odom telemetry packets back.
 *
 * WIRING:
 * - Runs over USB Serial interface.
 */

// --- ROBOT CONSTANTS ---
const float WHEEL_TRACK = 0.15f;  // Distance between wheels (meters)
const float WHEEL_DIAM = 0.066f;  // Wheel diameter (meters)

// --- MSG TYPE DEFINITIONS ---
const uint8_t SOF_BYTE = 0x02;
const uint8_t EOF_BYTE = 0x03;
const uint8_t MSG_CMD_VEL = 0x10;
const uint8_t MSG_ODOM = 0x20;

// --- FSM PARSER VARIABLES ---
enum ParserState { STATE_WAIT_SOF, STATE_READ_LEN, STATE_READ_DATA, STATE_READ_CS, STATE_READ_EOF };

ParserState currentState = STATE_WAIT_SOF;
uint8_t rxBuffer[32];
uint8_t rxIndex = 0;
uint8_t packetLen = 0;
uint8_t calcChecksum = 0;

// --- ROBOT STATE (ODOMETRY) ---
float odomX = 0.0f;      // Meters
float odomY = 0.0f;      // Meters
float odomTheta = 0.0f;  // Radians

// Current wheel velocities (RPM)
float targetRPM_L = 0.0f;
float targetRPM_R = 0.0f;

// --- TIMING VARIABLES ---
const float dt = 0.1f;  // 10 Hz update frequency (0.1s period)
bool publisherActive = true;
unsigned long lastOdomPublish = 0;

void setup() {
  Serial.begin(115200);  // Standard high speed for ROS serial bridges

  Serial.println(F("=================================================="));
  Serial.println(F("Day 100: Micro-ROS Serial Bridge Node"));
  Serial.println(F("=================================================="));

  printMenu();
}

void loop() {
  // 1. Read and parse incoming ROS serial packets
  while (Serial.available() > 0) {
    uint8_t incomingByte = Serial.read();

    // Intercept human CLI command if waiting for packet
    if (currentState == STATE_WAIT_SOF && isAlphaCommand(incomingByte)) {
      handleCLICommand((char)incomingByte);
    } else {
      processIncomingByte(incomingByte);
    }
  }

  // 2. Periodically publish /odom message at 10 Hz
  if (publisherActive && (millis() - lastOdomPublish >= 100)) {
    lastOdomPublish = millis();
    updateAndPublishOdom();
  }
}

bool isAlphaCommand(uint8_t b) {
  return (b == 'v' || b == 'V' || b == 's' || b == 'S' || b == 'r' || b == 'R' || b == 'h' ||
          b == 'H');
}

// =============================================================
//  FINITE STATE MACHINE SERIAL PARSER
// =============================================================
void processIncomingByte(uint8_t val) {
  switch (currentState) {
    case STATE_WAIT_SOF:
      if (val == SOF_BYTE) {
        currentState = STATE_READ_LEN;
        rxIndex = 0;
        calcChecksum = 0;
      }
      break;

    case STATE_READ_LEN:
      packetLen = val;
      if (packetLen == 0 || packetLen > 30) {
        currentState = STATE_WAIT_SOF;  // Invalid packet size guard
      } else {
        currentState = STATE_READ_DATA;
      }
      break;

    case STATE_READ_DATA:
      rxBuffer[rxIndex++] = val;
      calcChecksum ^= val;
      if (rxIndex >= packetLen) {
        currentState = STATE_READ_CS;
      }
      break;

    case STATE_READ_CS:
      if (val == calcChecksum) {
        currentState = STATE_READ_EOF;
      } else {
        Serial.println(F("[BRIDGE ERROR] Checksum mismatch. Discarding..."));
        currentState = STATE_WAIT_SOF;
      }
      break;

    case STATE_READ_EOF:
      if (val == EOF_BYTE) {
        executeROSMessage();
      }
      currentState = STATE_WAIT_SOF;
      break;
  }
}

// =============================================================
//  ROS MESSAGE DECODERS & KINEMATICS
// =============================================================
void executeROSMessage() {
  uint8_t msgType = rxBuffer[0];

  if (msgType == MSG_CMD_VEL) {
    // Decode signed 16-bit scaled integers
    int16_t rawLinearX = (rxBuffer[1] << 8) | rxBuffer[2];
    int16_t rawAngularZ = (rxBuffer[3] << 8) | rxBuffer[4];

    // Convert back to floats (meters/sec and rad/sec)
    float linearX = (float)rawLinearX / 1000.0f;
    float angularZ = (float)rawAngularZ / 1000.0f;

    // --- DIFFERENTIAL KINEMATICS ---
    // Calculate target linear wheel velocities (m/s)
    float vL = linearX - (angularZ * WHEEL_TRACK / 2.0f);
    float vR = linearX + (angularZ * WHEEL_TRACK / 2.0f);

    // Convert m/s wheel speeds to RPM (Revolutions Per Minute)
    // RPM = (v / (pi * diameter)) * 60
    targetRPM_L = (vL / (PI * WHEEL_DIAM)) * 60.0f;
    targetRPM_R = (vR / (PI * WHEEL_DIAM)) * 60.0f;

    Serial.println(F("\n---------------- ROS TOPIC: /cmd_vel ---------------"));
    Serial.print(F(" Decoded Msg -> Linear X: "));
    Serial.print(linearX, 3);
    Serial.print(F(" m/s"));
    Serial.print(F(" | Angular Z: "));
    Serial.print(angularZ, 3);
    Serial.println(F(" rad/s"));
    Serial.print(F(" Actuator Target -> Left Wheel: "));
    Serial.print(targetRPM_L, 1);
    Serial.print(F(" RPM"));
    Serial.print(F(" | Right Wheel: "));
    Serial.print(targetRPM_R, 1);
    Serial.println(F(" RPM"));
    Serial.println(F("----------------------------------------------------"));
  }
}

/**
 * Periodically runs odometry dead reckoning based on active wheel velocities,
 * and publishes the packed /odom state back to the PC.
 */
void updateAndPublishOdom() {
  // 1. Convert wheel RPM speeds back to linear velocities (m/s)
  // v = (RPM / 60) * pi * diameter
  float vL = (targetRPM_L / 60.0f) * PI * WHEEL_DIAM;
  float vR = (targetRPM_R / 60.0f) * PI * WHEEL_DIAM;

  // 2. Perform Kinematic Integration
  float vCenter = (vL + vR) / 2.0f;
  float dTheta = (vR - vL) * dt / WHEEL_TRACK;

  float midTheta = odomTheta + (dTheta / 2.0f);
  odomX += vCenter * dt * cos(midTheta);
  odomY += vCenter * dt * sin(midTheta);
  odomTheta += dTheta;

  // Constrain theta to [-pi, +pi]
  if (odomTheta > PI)
    odomTheta -= 2.0f * PI;
  else if (odomTheta < -PI)
    odomTheta += 2.0f * PI;

  // 3. Pack data into /odom binary packet (6 bytes payload)
  // Scale floats to 16-bit signed integers (multiplied by 100)
  int16_t scaleX = (int16_t)(odomX * 100.0f);
  int16_t scaleY = (int16_t)(odomY * 100.0f);
  int16_t scaleT = (int16_t)(odomTheta * 100.0f);

  uint8_t len = 7;  // MsgType (1) + Payload (6)
  uint8_t cs = MSG_ODOM ^ (uint8_t)(scaleX >> 8) ^ (uint8_t)(scaleX & 0xFF) ^
               (uint8_t)(scaleY >> 8) ^ (uint8_t)(scaleY & 0xFF) ^ (uint8_t)(scaleT >> 8) ^
               (uint8_t)(scaleT & 0xFF);

  // Send packet to PC over USB
  Serial.print(F("[ROS PUBLISH /odom] Binary Packet: "));
  printHexByte(SOF_BYTE);
  printHexByte(len);
  printHexByte(MSG_ODOM);
  printHexByte((uint8_t)(scaleX >> 8));
  printHexByte((uint8_t)(scaleX & 0xFF));
  printHexByte((uint8_t)(scaleY >> 8));
  printHexByte((uint8_t)(scaleY & 0xFF));
  printHexByte((uint8_t)(scaleT >> 8));
  printHexByte((uint8_t)(scaleT & 0xFF));
  printHexByte(cs);
  printHexByte(EOF_BYTE);

  // Also print readable telemetry for the user
  Serial.print(F(" | Decoded -> X:"));
  Serial.print(odomX, 2);
  Serial.print(F("m, Y:"));
  Serial.print(odomY, 2);
  Serial.print(F("m, Yaw:"));
  Serial.print(odomTheta * 180.0f / PI, 1);
  Serial.println(F("deg"));
}

void printHexByte(uint8_t b) {
  if (b < 16) Serial.print('0');
  Serial.print(b, HEX);
  Serial.print(' ');
}

// =============================================================
//  ROS PC MASTER SIMULATOR (CLI INTERFACE)
// =============================================================
void handleCLICommand(char cmd) {
  switch (cmd) {
    case 'v':
    case 'V': {
      // Simulate ROS PC publishing `/cmd_vel` msg
      float linX = Serial.parseFloat();
      float angZ = Serial.parseFloat();

      // Convert to scaled 16-bit integers
      int16_t sLin = (int16_t)(linX * 1000.0f);
      int16_t sAng = (int16_t)(angZ * 1000.0f);

      uint8_t len = 5;
      uint8_t msgType = MSG_CMD_VEL;
      uint8_t val1 = (uint8_t)(sLin >> 8);
      uint8_t val2 = (uint8_t)(sLin & 0xFF);
      uint8_t val3 = (uint8_t)(sAng >> 8);
      uint8_t val4 = (uint8_t)(sAng & 0xFF);
      uint8_t cs = msgType ^ val1 ^ val2 ^ val3 ^ val4;

      // Pack binary frame
      uint8_t packet[9] = {SOF_BYTE, len, msgType, val1, val2, val3, val4, cs, EOF_BYTE};

      Serial.print(F("\n[PC SIMULATOR] Publishing `/cmd_vel` topic..."));
      Serial.println(F("\n--- Transmitting Binary Packet over UART ---"));

      // Feed packet into parser
      for (int i = 0; i < 9; i++) {
        processIncomingByte(packet[i]);
      }
      break;
    }

    case 's':
    case 'S':
      publisherActive = !publisherActive;
      Serial.print(F("[SYSTEM] /odom topic publishing: "));
      Serial.println(publisherActive ? F("ACTIVE") : F("PAUSED"));
      break;

    case 'r':
    case 'R':
      // Reset odometry system
      odomX = 0.0f;
      odomY = 0.0f;
      odomTheta = 0.0f;
      targetRPM_L = 0.0f;
      targetRPM_R = 0.0f;
      Serial.println(F("[SYSTEM] Resetting robot coordinate tracking (0,0,0)"));
      break;

    case 'h':
    case 'H':
      printMenu();
      break;

    default:
      break;
  }
}

void printMenu() {
  Serial.println(F("\n--- ROS MASTER COMMAND SIMULATOR ---"));
  Serial.println(F(" Simulate ROS publishing velocity commands:"));
  Serial.println(F(" 'v <linX> <angZ>' : Publish cmd_vel Twist message (e.g. 'v 0.20 -0.40')"));
  Serial.println(F(" System Toggles:"));
  Serial.println(F(" 's'              : Toggle /odom topic publisher loop (10 Hz)"));
  Serial.println(F(" 'r'              : Reset robot coordinates to (0,0,0) and stop motors"));
  Serial.println(F(" 'h'              : Display this command list"));
  Serial.println(F("-------------------------------------\n"));
}

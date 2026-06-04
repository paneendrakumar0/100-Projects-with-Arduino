/*
 * 100 Projects with Arduino - Day 39
 * Project: HC-05 Bluetooth Module (UART Remote Robot Controller)
 * 
 * DESCRIPTION:
 * This project interfaces an HC-05 Bluetooth transceiver module over the UART interface.
 * To achieve professional-level wireless mechatronics controls:
 * 1. Software Serial Bus: Uses SoftwareSerial on digital pins 10 (RX) and 11 (TX) to preserve
 *    the Arduino's hardware UART (pins 0/1) for PC-based Serial Monitor debugging.
 * 2. Character Buffer Parser: Implements a non-blocking character accumulator that parses ASCII
 *    command packets ending in semicolons (';'), preventing data fragmentation lag.
 * 3. Drive-State Router: Decodes parsed commands to update a 4-pin robot motor driver simulation
 *    (Forward, Reverse, Left, Right, Stop).
 * 4. Dual-Console Telemetry: Mirrors decoded Bluetooth actions to the PC Serial Monitor for diagnostics.
 * 
 * UART & BLUETOOTH HARDWARE THEORY:
 * - UART (Universal Asynchronous Receiver-Transmitter): A serial protocol that transmits data bit-by-bit
 *   without a shared clock signal. Synchronization is achieved using Start and Stop bits. Baud rate
 *   defines transmission speed (e.g. 9600 bits per second).
 * - RX/TX Voltage Mismatch:
 *   The Arduino Uno runs on 5V logic. The HC-05 runs on 3.3V logic. While the Arduino RX pin can read 
 *   3.3V logic safely, the Arduino TX pin outputs 5V. Supplying 5V directly to the HC-05 RX pin can overheat 
 *   and degrade the chip. A resistor voltage divider (1kΩ and 2kΩ) must be placed on the Arduino TX -> HC-05 RX 
 *   line to drop the 5V logic signal safely down to 3.3V.
 *     Voltage Out = 5V * [ 2kΩ / (1kΩ + 2kΩ) ] = 3.33V
 * 
 * WIRING:
 * - HC-05 Module -> Arduino Uno
 *   - VCC -> 5V
 *   - GND -> GND
 *   - TXD -> Pin 10 (Arduino Software RX)
 *   - RXD -> Connect to Pin 11 (Arduino Software TX) via a 1kΩ/2kΩ voltage divider!
 * - Simulated Motor Driver Outputs -> Arduino Uno
 *   - Left Motor Forward (L_Fwd)  -> Pin 5
 *   - Left Motor Reverse (L_Rev)  -> Pin 6
 *   - Right Motor Forward (R_Fwd) -> Pin 7
 *   - Right Motor Reverse (R_Rev) -> Pin 8
 */

#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const int BT_RX_PIN = 10; // Pin 10 receives data from HC-05 TX
const int BT_TX_PIN = 11; // Pin 11 transmits data to HC-05 RX (via voltage divider)

const int L_FWD_PIN = 5;
const int L_REV_PIN = 6;
const int R_FWD_PIN = 7;
const int R_REV_PIN = 8;

// Instantiate software serial object
SoftwareSerial bluetoothSerial(BT_RX_PIN, BT_TX_PIN);

// --- BUFFER PARSER CONFIGURATION ---
String packetBuffer = ""; // String buffer to accumulate incoming command characters

void setup() {
  // Initialize hardware serial for PC debug console
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 39: HC-05 Bluetooth UART Remote Controller");
  Serial.println("==================================================");

  // Initialize simulated motor pins
  pinMode(L_FWD_PIN, OUTPUT);
  pinMode(L_REV_PIN, OUTPUT);
  pinMode(R_FWD_PIN, OUTPUT);
  pinMode(R_REV_PIN, OUTPUT);
  
  // Initially stop motor drives
  stopRobot();

  // Initialize Bluetooth software serial at standard default HC-05 baud rate (9600)
  bluetoothSerial.begin(9600);
  
  Serial.println("[SYSTEM] SoftwareSerial active at 9600 Baud.");
  Serial.println("[SYSTEM] Connect your smartphone Bluetooth terminal to 'HC-05'.");
  Serial.println("[SYSTEM] Send commands: F; (Forward), B; (Backward), L; (Left), R; (Right), S; (Stop)");
}

void loop() {
  // Non-blocking check for incoming bytes from the Bluetooth module
  while (bluetoothSerial.available() > 0) {
    char incomingChar = (char)bluetoothSerial.read();
    
    // Process the character through the parser state machine
    parseBluetoothCharacter(incomingChar);
  }
}

/**
 * Accumulates characters until a packet terminator (';') is read.
 * Eliminates whitespace and carriage returns to prevent command pollution.
 */
void parseBluetoothCharacter(char c) {
  // Ignore carriage returns and newline characters
  if (c == '\r' || c == '\n') {
    return;
  }
  
  // Check for the packet terminator character (';')
  if (c == ';') {
    if (packetBuffer.length() > 0) {
      packetBuffer.toUpperCase(); // Normalize packet case
      executeRobotCommand(packetBuffer);
      packetBuffer = ""; // Reset buffer
    }
  } else {
    // Prevent buffer overflows by constraining string length
    if (packetBuffer.length() < 16) {
      packetBuffer += c;
    } else {
      Serial.println("[WARNING] Buffer overrun. Clearing packet buffer.");
      packetBuffer = "";
    }
  }
}

/**
 * Parses the command string and writes physical logic outputs.
 */
void executeRobotCommand(String cmd) {
  Serial.print("[BLUETOOTH CMD] Received: '");
  Serial.print(cmd);
  Serial.print("' -> ");
  
  if (cmd == "F") {
    Serial.println("ACTION: Move Forward");
    moveForward();
  } 
  else if (cmd == "B") {
    Serial.println("ACTION: Move Backward");
    moveBackward();
  } 
  else if (cmd == "L") {
    Serial.println("ACTION: Spin Left");
    spinLeft();
  } 
  else if (cmd == "R") {
    Serial.println("ACTION: Spin Right");
    spinRight();
  } 
  else if (cmd == "S") {
    Serial.println("ACTION: Hard Stop");
    stopRobot();
  } 
  else {
    Serial.println("ACTION: Unknown Command");
    // Send feedback text back to the smartphone screen
    bluetoothSerial.println("ERROR: Unknown Command;");
    return;
  }
  
  // Echo confirmation text back over Bluetooth to the terminal user
  bluetoothSerial.print("SUCCESS: ");
  bluetoothSerial.print(cmd);
  bluetoothSerial.println(";");
}

// --- ROBOT CHASSIS ACTUATION FUNCTIONS ---

void moveForward() {
  digitalWrite(L_REV_PIN, LOW);
  digitalWrite(R_REV_PIN, LOW);
  digitalWrite(L_FWD_PIN, HIGH);
  digitalWrite(R_FWD_PIN, HIGH);
}

void moveBackward() {
  digitalWrite(L_FWD_PIN, LOW);
  digitalWrite(R_FWD_PIN, LOW);
  digitalWrite(L_REV_PIN, HIGH);
  digitalWrite(R_REV_PIN, HIGH);
}

void spinLeft() {
  digitalWrite(L_FWD_PIN, LOW);
  digitalWrite(R_REV_PIN, LOW);
  digitalWrite(L_REV_PIN, HIGH);
  digitalWrite(R_FWD_PIN, HIGH);
}

void spinRight() {
  digitalWrite(L_REV_PIN, LOW);
  digitalWrite(R_FWD_PIN, LOW);
  digitalWrite(L_FWD_PIN, HIGH);
  digitalWrite(R_REV_PIN, HIGH);
}

void stopRobot() {
  digitalWrite(L_FWD_PIN, LOW);
  digitalWrite(L_REV_PIN, LOW);
  digitalWrite(R_FWD_PIN, LOW);
  digitalWrite(R_REV_PIN, LOW);
}

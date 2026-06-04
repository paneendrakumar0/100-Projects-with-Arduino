/*
 * 100 Projects with Arduino - Day 80
 * Project: I2C Multi-Node Master/Slave Communication (Wire Callback Architecture)
 * 
 * DESCRIPTION:
 * This project demonstrates how to build an inter-microcontroller network using the I2C (Inter-Integrated
 * Circuit) bus. To maximize educational clarity and follow clean engineering practices, this project
 * is contained in a single `.ino` file. By changing the `#define I2C_MODE` compiler switch at the top,
 * the user can flash this codebase onto two separate Arduino boards to make one the **Master** node
 * and the other the **Slave** node.
 * 
 * NODE BEHAVIORS:
 * 1. MASTER NODE (Mode = 1):
 *    - Periodically (every 1 second, non-blocking) requests a 2-byte sensor telemetry packet from
 *      the Slave at address 0x08.
 *    - Parses the incoming high/low bytes back into a 16-bit integer (representing sensor data).
 *    - Periodically sends a command packet (toggle LED) to the Slave.
 * 2. SLAVE NODE (Mode = 0, address 0x08):
 *    - Uses interrupt-driven callbacks (`Wire.onRequest` and `Wire.onReceive`) to process Master
 *      interactions.
 *    - onRequest(): Reads its analog input A0 and writes the 2-byte data package back to the Master.
 *    - onReceive(): Parses the incoming command package to toggle its local onboard LED (Pin 13).
 * 
 * I2C NETWORKING THEORY:
 * - Two-Wire Interface: Uses two bidirectional open-drain lines pulled HIGH by resistors:
 *   - SDA (Serial Data Line)
 *   - SCL (Serial Clock Line)
 * - Master-Slave Bus: The Master generates the clock signal and initiates all transmissions.
 * - Addressing: Slaves have unique 7-bit addresses (we use 0x08).
 * - Hardware Interrupts: Slaves do not poll. When an I2C start condition matching their address
 *   is detected, the hardware handles the clock sync and triggers the interrupt callbacks,
 *   allowing the main loop to execute other tasks concurrently.
 * 
 * WIRING:
 * - Connect two Arduinos (Master and Slave) pin-to-pin:
 *   - Master GND <-> Slave GND (CRITICAL: I2C requires a shared ground reference!)
 *   - Master SDA (A4) <-> Slave SDA (A4)
 *   - Master SCL (A5) <-> Slave SCL (A5)
 * - Optional: Connect a 4.7kΩ pull-up resistor from SDA to 5V, and SCL to 5V. (Arduino's internal
 *   pullups are activated by default in Wire.h, which is sufficient for short wire lengths).
 */

#include <Wire.h>

// =============================================================
//  CONFIGURATION SWITCH (0 = SLAVE MODE, 1 = MASTER MODE)
// =============================================================
#define I2C_MODE 1  // <-- Change to 0 for Slave board, 1 for Master board

// --- I2C SLAVE DEVICE ADDRESS ---
const uint8_t SLAVE_I2C_ADDR = 0x08;

// --- COMMON PIN DEFINITIONS ---
const int ONBOARD_LED = 13;
const int SENSOR_PIN  = A0; // Potentiometer or analog sensor input on Slave

// --- MASTER STATE & TIMING VARIABLES ---
#if (I2C_MODE == 1)
unsigned long lastRequestTime = 0;
const unsigned long REQUEST_INTERVAL_MS = 1000; // Request data every 1 second
bool masterLedState = false;
#endif

// --- SLAVE STATE VARIABLES ---
#if (I2C_MODE == 0)
volatile uint16_t slaveSensorValue = 0;
volatile bool slaveLedCommand = false;
#endif

void setup() {
  Serial.begin(9600);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);

  Serial.println(F("=================================================="));
  #if (I2C_MODE == 1)
    Serial.println(F("Day 80: I2C Master Controller Node"));
    Wire.begin(); // Join I2C Bus as Master (no address needed)
    Serial.println(F("[SYSTEM] Bus Initialized. Operating as Master."));
  #else
    Serial.println(F("Day 80: I2C Slave Peripheral Node"));
    Wire.begin(SLAVE_I2C_ADDR); // Join I2C Bus as Slave with address 0x08
    
    // Register hardware interrupt callbacks
    Wire.onRequest(handleMasterRequest);
    Wire.onReceive(handleMasterReceive);
    
    Serial.print(F("[SYSTEM] Slave initialized at Address: 0x"));
    Serial.println(SLAVE_I2C_ADDR, HEX);
    Serial.println(F("[SYSTEM] Waiting for Master transactions..."));
  #endif
  Serial.println(F("=================================================="));
}

void loop() {
  #if (I2C_MODE == 1)
    // =============================================================
    //  MASTER OPERATION LOOP
    // =============================================================
    unsigned long currentMillis = millis();

    // Periodic non-blocking loop to poll Slave telemetry and send commands
    if (currentMillis - lastRequestTime >= REQUEST_INTERVAL_MS) {
      lastRequestTime = currentMillis;

      // 1. Request Telemetry from Slave (2 bytes)
      Serial.print(F("[MASTER] Requesting 2 bytes from Slave 0x"));
      Serial.print(SLAVE_I2C_ADDR, HEX);
      Serial.println(F("..."));

      Wire.requestFrom(SLAVE_I2C_ADDR, (uint8_t)2);
      
      if (Wire.available() >= 2) {
        // Reassemble 16-bit integer from high/low bytes
        byte high = Wire.read();
        byte low  = Wire.read();
        uint16_t receivedValue = (high << 8) | low;

        Serial.print(F("[MASTER] Received Slave Sensor: "));
        Serial.println(receivedValue);
      } else {
        Serial.println(F("[WARNING] Slave failed to respond / Bus timeout."));
      }

      // 2. Transmit LED toggle command to Slave
      masterLedState = !masterLedState;
      Serial.print(F("[MASTER] Transmitting LED state: "));
      Serial.println(masterLedState ? F("HIGH") : F("LOW"));

      Wire.beginTransmission(SLAVE_I2C_ADDR);
      Wire.write(0x01);            // Command byte: 0x01 = LED Command
      Wire.write(masterLedState);  // Parameter byte: 1 or 0
      byte status = Wire.endTransmission();

      if (status != 0) {
        Serial.print(F("[ERROR] Transmission failed! Error code: "));
        Serial.println(status);
      }
      Serial.println();
    }

  #else
    // =============================================================
    //  SLAVE OPERATION LOOP
    // =============================================================
    // Read the local analog sensor continuously in the background
    // (This value will be served instantly whenonRequest interrupts trigger)
    uint16_t currentReading = analogRead(SENSOR_PIN);
    
    // Disable interrupts briefly to perform an atomic write to the volatile variable
    noInterrupts();
    slaveSensorValue = currentReading;
    interrupts();

    // Toggle local LED based on received command param
    digitalWrite(ONBOARD_LED, slaveLedCommand ? HIGH : LOW);
    
    delay(50); // Small limit to prevent ADC thrashing
  #endif
}

// =============================================================
//  I2C SLAVE INTERRUPT CALLBACKS (MODE = 0 ONLY)
// =============================================================
#if (I2C_MODE == 0)
/**
 * Interrupt Service Routine (ISR) triggered when Master requests data.
 * MUST be extremely fast: no print statements or delays!
 */
void handleMasterRequest() {
  // Send the 2-byte sensor value package (high byte first)
  byte high = (byte)(slaveSensorValue >> 8);
  byte low  = (byte)(slaveSensorValue & 0xFF);
  
  Wire.write(high);
  Wire.write(low);
}

/**
 * ISR triggered when Master transmits data to the Slave.
 */
void handleMasterReceive(int numBytes) {
  // Expecting 2 bytes: [Command, Parameter]
  if (numBytes >= 2) {
    byte command = Wire.read();
    byte parameter = Wire.read();

    if (command == 0x01) {
      slaveLedCommand = (parameter > 0);
    }
  }

  // Clear any extra bytes in the buffer
  while (Wire.available() > 0) {
    Wire.read();
  }
}
#endif

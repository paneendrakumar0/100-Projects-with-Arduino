/*
 * 100 Projects with Arduino - Day 40
 * Project: nRF24L01+ 2.4GHz RF Transceiver (Dual-Role SPI Packet Link)
 * 
 * DESCRIPTION:
 * This project interfaces the nRF24L01+ 2.4 GHz radio frequency transceiver module over the SPI bus.
 * To implement a robust, scalable wireless telemetry link:
 * 1. Dual-Role Firmware: Uses a hardware Jumper Pin (Pin 8) to determine role on boot. 
 *    If Pin 8 is jumpered to GND, the board operates as a Transmitter. If left open, it acts as a Receiver.
 *    This allows the user to compile and upload the exact same sketch to both transceiver nodes.
 * 2. Structured Multi-Byte Payload: Transmits a packed data structure containing a packet counter,
 *    an analog sensor reading, and a system uptime float, demonstrating binary serialization.
 * 3. SPI Control Interface: Configures Chip Enable (CE) and Chip Select Not (CSN) alongside hardware SPI pins.
 * 4. Automatic Acknowledgment: Configures the radio pipelines with Auto-ACK (Enhanced ShockBurst)
 *    and low power amplifier levels to prevent short-range receiver saturation.
 * 
 * RF24 & SPI HARDWARE THEORY:
 * - nRF24L01+ Transceiver: Runs in the 2.4 GHz ISM band. It splits the band into 125 channels separated
 *   by 1 MHz. It uses GFSK modulation and handles packet framing, preamble, CRC error checks, and Auto-ACK
 *   internally on the chip hardware, offloading the microcontroller.
 * - SPI Bus Wiring:
 *   The module requires high-speed SPI (MOSI=11, MISO=12, SCK=13).
 *   Additionally, CE (Pin 9) controls RX/TX active modes, and CSN (Pin 10) controls SPI bus selection.
 * - Shared Address Pipeline:
 *   The transmitter and receiver must open a reading/writing pipeline using the exact same 5-byte address
 *   (e.g., "00001") and operate on the same frequency channel (default channel 76).
 * 
 * WIRING:
 * - nRF24L01+ Module -> Arduino Uno
 *   - VCC -> 3.3V (WARNING: 5V will destroy the radio chip! nRF24L01 is NOT 5V tolerant on VCC!)
 *   - GND -> GND
 *   - CE  -> Pin 9
 *   - CSN -> Pin 10
 *   - SCK -> Pin 13
 *   - MOSI-> Pin 11
 *   - MISO-> Pin 12
 *   - IRQ -> Not Connected
 * - Role Select Jumper -> Arduino Uno
 *   - Pin 8 -> Ground (Connect with a jumper wire to GND to select TRANSMITTER role, leave open for RECEIVER)
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- PIN DEFINITIONS ---
const int CE_PIN  = 9;  // Radio Chip Enable
const int CSN_PIN = 10; // Radio Chip Select Not
const int ROLE_PIN = 8; // Ground this pin to select Transmitter, leave open for Receiver

// Instantiate the radio object
RF24 radio(CE_PIN, CSN_PIN);

// Shared pipeline address (5 bytes)
const byte radioAddress[6] = "NODE1";

// --- STRUCTURED PACKET PAYLOAD ---
struct TelemetryPacket {
  uint32_t packetID;   // 4 bytes: Monotonic packet sequence counter
  int sensorValue;     // 2 bytes: ADC reading (e.g. from A0)
  float uptimeSeconds; // 4 bytes: Arduino uptime in seconds
}; // Total size = 10 bytes

// --- TIMING VARIABLES ---
unsigned long lastTxTime = 0;
const unsigned long txIntervalMs = 1000; // Transmit once per second in TX mode

// Role state tracking
bool isTransmitter = false;
uint32_t packetCounter = 0;

void setup() {
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 40: nRF24L01+ 2.4GHz RF Transceiver SPI Link");
  Serial.println("==================================================");

  // Setup role selector pin using internal pull-up
  pinMode(ROLE_PIN, INPUT_PULLUP);
  
  // Read role selection pin (LOW = Transmitter, HIGH = Receiver)
  // We wait 100ms to allow pin voltage to stabilize after power-up
  delay(100);
  if (digitalRead(ROLE_PIN) == LOW) {
    isTransmitter = true;
  } else {
    isTransmitter = false;
  }

  // Print selected role to serial monitor
  Serial.print("[BOOT] Role configured: ");
  if (isTransmitter) {
    Serial.println("TRANSMITTER (Pin 8 grounded)");
  } else {
    Serial.println("RECEIVER (Pin 8 open/HIGH)");
  }

  // Initialize the radio chip
  if (!radio.begin()) {
    Serial.println("[ERROR] nRF24L01+ hardware initialization failed! Check wiring.");
    for (;;); // Halt setup
  }

  // Set the Power Amplifier level.
  // RF24_PA_MIN is used for desktop testing because close-range high power saturates the receiver,
  // causing packet loss. For long-range field deployment, use RF24_PA_MAX.
  radio.setPALevel(RF24_PA_MIN);
  
  // Set data rate to 1MBPS (options: RF24_250KBPS, RF24_1MBPS, RF24_2MBPS)
  radio.setDataRate(RF24_1MBPS);

  // Configure reading and writing pipelines
  if (isTransmitter) {
    radio.openWritingPipe(radioAddress);
    radio.stopListening(); // Put radio into Transmit Mode
    Serial.println("[RADIO] Opened writing pipe. Ready to broadcast.");
  } else {
    radio.openReadingPipe(1, radioAddress);
    radio.startListening(); // Put radio into Receive Mode (listening)
    Serial.println("[RADIO] Opened reading pipe. Listening for telemetry...");
  }
}

void loop() {
  if (isTransmitter) {
    // --- TRANSMITTER BEHAVIOR ---
    unsigned long currentMillis = millis();
    if (currentMillis - lastTxTime >= txIntervalMs) {
      lastTxTime = currentMillis;

      // Populate telemetry struct
      TelemetryPacket packet;
      packet.packetID = packetCounter++;
      packet.sensorValue = analogRead(A0); // Read analog pin A0 (floating noise if unconnected)
      packet.uptimeSeconds = (float)currentMillis / 1000.0;

      // Transmit the packet (blocking call, but returns quickly)
      // radio.write() returns true if the receiver acknowledged the packet (Auto-ACK)
      bool ackReceived = radio.write(&packet, sizeof(TelemetryPacket));

      Serial.print("[TX] Sent Packet #");
      Serial.print(packet.packetID);
      Serial.print(" | Sensor A0: ");
      Serial.print(packet.sensorValue);
      Serial.print(" | Uptime: ");
      Serial.print(packet.uptimeSeconds, 1);
      Serial.print("s | Status: ");
      if (ackReceived) {
        Serial.println("ACK Received OK");
      } else {
        Serial.println("TX Failed (No ACK)");
      }
    }
  } 
  else {
    // --- RECEIVER BEHAVIOR ---
    // Check if wireless packets are available in the FIFO buffer
    if (radio.available()) {
      TelemetryPacket packet;
      
      // Read the packet from the RX FIFO buffer
      radio.read(&packet, sizeof(TelemetryPacket));

      // Display decoded telemetry to the monitor console
      Serial.print("[RX] Telemetry Received -> Packet ID: ");
      Serial.print(packet.packetID);
      Serial.print(" | Sensor A0: ");
      Serial.print(packet.sensorValue);
      Serial.print(" | Sender Uptime: ");
      Serial.print(packet.uptimeSeconds, 1);
      Serial.println("s");
    }
  }
}

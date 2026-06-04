/*
 * 100 Projects with Arduino - Day 28
 * Project: Infrared (IR) Remote Control Decoder (NEC Protocol & State Controller)
 * 
 * DESCRIPTION:
 * This project interfaces a TSOP38238 / VS1838B infrared receiver to decode signals from an IR remote.
 * Using the modern IRremote library (v4.x):
 * 1. Decodes incoming carrier-modulated IR light bursts (specifically the standard NEC protocol).
 * 2. Implements a multi-channel receiver state machine to toggle three independent LED outputs
 *    (Red, Green, Blue) representing robotic subsystems.
 * 3. Includes an emergency stop command (Power Button) to turn off all outputs.
 * 4. Outputs verbose telemetry to the Serial Monitor to allow calibration and discovery of code hexes
 *    for any unknown custom remote.
 * 
 * INFRARED PROTOCOL THEORY (NEC Protocol):
 * - IR communication uses infrared light pulses modulated at a carrier frequency of 38 kHz to filter 
 *   out ambient IR light from the sun or lightbulbs.
 * - The standard NEC protocol uses pulse-distance modulation:
 *   - Start frame: 9ms leading pulse + 4.5ms space.
 *   - Bit 0: 562.5µs pulse + 562.5µs space (Total 1.125ms)
 *   - Bit 1: 562.5µs pulse + 1.6875ms space (Total 2.25ms)
 *   - Repeat code: 9ms pulse + 2.25ms space (sent if a key is held down).
 * - A standard frame consists of 32 bits: 8-bit Address, 8-bit inverted Address, 8-bit Command, 8-bit inverted Command.
 *   The inverted bytes are used to verify transmission integrity (parity).
 * 
 * HARDWARE CONNECTIONS:
 * - VS1838B / TSOP Pinout (facing the bulb curve):
 *   1. OUT -> Arduino Pin 11 (IR signal input)
 *   2. GND -> Arduino GND
 *   3. VCC -> Arduino 5V
 * - Red LED   -> Arduino Pin 5 (via 220 Ohm resistor)
 * - Green LED -> Arduino Pin 6 (via 220 Ohm resistor)
 * - Blue LED  -> Arduino Pin 7 (via 220 Ohm resistor)
 * 
 * LIBRARY REQUIREMENT:
 * This code requires the "IRremote" library by Armin Joachimsmeyer (install v4.x via Library Manager).
 * Do NOT use older 2.x versions as they use outdated and blocking APIs.
 */

// Define the signal receive pin before including the library header to allow internal configurations
#define IR_RECEIVE_PIN 11

#include <IRremote.hpp> // Uses modern v3.x/v4.x library structure

// --- PIN DEFINITIONS ---
const int RED_LED_PIN = 5;
const int GREEN_LED_PIN = 6;
const int BLUE_LED_PIN = 7;

// --- IR KEY CODES (Standard Chinese Mini IR Remotes - NEC Protocol) ---
// Note: If your remote sends different codes, press the buttons while the serial monitor
// is open, observe the printed Command code in HEX, and replace these values.
const uint16_t NEC_KEY_1     = 0x0C; // Button 1
const uint16_t NEC_KEY_2     = 0x18; // Button 2
const uint16_t NEC_KEY_3     = 0x5E; // Button 3
const uint16_t NEC_KEY_POWER = 0x45; // Power Button (used as System Clear / E-Stop)

void setup() {
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 28: IR Remote Decoder & Subsystem Controller");
  Serial.println("==================================================");

  // Initialize output pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // Start the IR Receiver
  // ENABLE_LED_FEEDBACK blinks the built-in Arduino LED (pin 13) when an IR signal is received
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.print("IR Receiver initialized on digital pin ");
  Serial.println(IR_RECEIVE_PIN);
  Serial.println("Ready to decode. Aim remote at sensor and press keys...");
}

void loop() {
  // Non-blocking check for incoming IR signals
  if (IrReceiver.decode()) {
    
    // Check if the received data is valid and not corrupted
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_CORRUPT) {
      Serial.println("[WARNING] Corrupted signal received. Retrying...");
    } 
    else {
      // Check if this is a repeat signal (user holding button down)
      bool isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT);
      
      // We only process fresh button clicks to prevent flickering toggles from repeat frames
      if (!isRepeat) {
        processIRCommand(IrReceiver.decodedIRData.command);
      } else {
        Serial.println("[IR] Key held down (Repeat Code detected).");
      }
    }
    
    // Essential: resume receiving the next signal
    IrReceiver.resume(); 
  }
}

/**
 * Parses the decoded NEC command and toggles corresponding outputs.
 * Prints command details to the serial port.
 */
void processIRCommand(uint16_t command) {
  // Print incoming command details for calibration/developer use
  Serial.print("[IR DECODED] Protocol: ");
  Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
  Serial.print(" | Address: 0x");
  Serial.print(IrReceiver.decodedIRData.address, HEX);
  Serial.print(" | Command Code: 0x");
  Serial.println(command, HEX);

  // State control router
  switch (command) {
    case NEC_KEY_1:
      toggleOutput(RED_LED_PIN, "RED LED (Subsystem A)");
      break;
      
    case NEC_KEY_2:
      toggleOutput(GREEN_LED_PIN, "GREEN LED (Subsystem B)");
      break;
      
    case NEC_KEY_3:
      toggleOutput(BLUE_LED_PIN, "BLUE LED (Subsystem C)");
      break;
      
    case NEC_KEY_POWER:
      Serial.println("[EMERGENCY STOP] Power key pressed. Shutting down all subsystems!");
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(BLUE_LED_PIN, LOW);
      break;
      
    default:
      Serial.println("[IR] Unknown command code. Update code constants if calibrating.");
      break;
  }
}

/**
 * Toggles a digital pin's state and prints a debug message.
 */
void toggleOutput(int pin, const char* name) {
  int currentState = digitalRead(pin);
  int nextState = !currentState;
  digitalWrite(pin, nextState);
  
  Serial.print("[SYSTEM] Toggled ");
  Serial.print(name);
  Serial.print(" -> ");
  Serial.println(nextState == HIGH ? "ON" : "OFF");
}

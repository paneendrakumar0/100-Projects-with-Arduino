/*
 * 100 Projects with Arduino - Day 16
 * Project: Relay Module Control (High Power Switching)
 * 
 * DESCRIPTION:
 * This project demonstrates how to interface an electromagnetic relay module with the Arduino.
 * To maintain professional coding standards, the relay is cycled at a regular, non-blocking 
 * 5-second interval using millis() timing. The sketch is configured to support both Active-Low 
 * and Active-High relay boards via flexible code definitions.
 * 
 * THEORY OF OPERATION:
 * 1. Galvanic Isolation: A microcontroller runs on 5V and can output a maximum of 20-40 mA per pin.
 *    High-power appliances (lightbulbs, AC motors, heaters) run on high voltages (12V, 24V, or 120V/240V AC)
 *    and draw amperes of current. Connecting these directly to the Arduino would destroy the chip.
 *    A relay provides **galvanic isolation**, separating the low-power control circuit from the
 *    high-power load circuit using an air gap.
 * 2. Electromagnetic Coil Mechanics:
 *    Inside the relay casing is an electromagnet (coil) surrounding an iron core.
 *    - When the Arduino pin supplies current, the coil is energized, generating a magnetic field.
 *    - This field attracts a pivoting steel arm (armature).
 *    - The armature physically moves the electrical contacts, switching the load circuit.
 * 3. Relay Module Terminal Physics:
 *    - Common (COM): The central terminal where the load supply wire is connected.
 *    - Normally Closed (NC): The load is connected to COM when the relay is de-energized.
 *      (Closed circuit = ON by default).
 *    - Normally Open (NO): The load is disconnected when the relay is de-energized.
 *      (Open circuit = OFF by default). It closes and completes the circuit only when energized.
 * 4. Optocoupled Isolation & Active-Low Triggering:
 *    Most Arduino relay modules use an **optocoupler** (an internal LED and phototransistor).
 *    This completely isolates the Arduino electrically. These modules are often **Active-Low**:
 *    - Writing `LOW` (GND) draws current through the opto-LED, activating the relay.
 *    - Writing `HIGH` (5V) stops current flow, deactivating the relay.
 * 
 * WIRING:
 * - Relay VCC  -> Arduino 5V
 * - Relay GND  -> Arduino GND
 * - Relay IN   -> Arduino Pin 7 (Digital output)
 * 
 * WARNING:
 * This project is designed for switching low-voltage loads (e.g. a 5V/12V DC motor, battery-powered LED strips).
 * **NEVER work with high-voltage mains electricity (120V/240V AC) unless you are a qualified electrician.**
 * High voltages can cause fatal shocks and fire hazards.
 */

// --- CONFIGURATION: RELAY ACTIVE STATE ---
// Standard Arduino relay modules with optocouplers are typically Active-Low (trigger on LOW).
// Set to 'LOW' for Active-Low modules, or 'HIGH' for Active-High modules.
#define RELAY_ACTIVE_STATE LOW

// Calculate the opposing off state mathematically
const int RELAY_ON = RELAY_ACTIVE_STATE;
const int RELAY_OFF = (RELAY_ACTIVE_STATE == LOW) ? HIGH : LOW;

// --- PIN DEFINITIONS ---
const int RELAY_PIN = 7; // Pin connected to the relay control input (IN)

// --- STATE VARIABLES ---
bool relayArmedState = false;         // Tracks the logical state of the relay (ON or OFF)
unsigned long lastSwitchTime = 0;     // Stores the timestamp of the last switch event
const unsigned long switchInterval = 5000; // Switch interval in ms (5 seconds)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure the relay control pin as an output
  pinMode(RELAY_PIN, OUTPUT);
  
  // Set the initial state to OFF immediately during boot to prevent accidental load triggering
  digitalWrite(RELAY_PIN, RELAY_OFF);
  
  Serial.println("==================================================");
  Serial.println("Day 16: Relay Module Controller (Galvanic Isolation)");
  Serial.println("==================================================");
  Serial.println("System armed. Cycling relay every 5 seconds...");
  Serial.println("[STATE] Relay is: DE-ENERGIZED (OFF)");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Cycle the relay at scheduled non-blocking interval
  if (currentTime - lastSwitchTime >= switchInterval) {
    lastSwitchTime = currentTime;
    
    // Toggle the logical state
    relayArmedState = !relayArmedState;
    
    // Apply the active state values to the pin
    if (relayArmedState) {
      digitalWrite(RELAY_PIN, RELAY_ON);
      Serial.println("[STATE] Relay is: ENERGIZED (ON) -> COM connected to NO");
    } else {
      digitalWrite(RELAY_PIN, RELAY_OFF);
      Serial.println("[STATE] Relay is: DE-ENERGIZED (OFF) -> COM connected to NC");
    }
  }
  
  // The loop is non-blocking. Background tasks like reading sensors can run here
}

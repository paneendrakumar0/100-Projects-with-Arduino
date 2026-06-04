/*
 * 100 Projects with Arduino - Day 17
 * Project: Sound Sensor Clap Switch (Echo Lockout Filter)
 * 
 * DESCRIPTION:
 * This project interfaces a digital sound sensor module with the Arduino to build a clap-activated
 * switch. Clapping your hands once toggles the state of an LED.
 * To maintain professional coding standards:
 * 1. The code implements a non-blocking Lockout Window (or sound debounce) of 250 milliseconds.
 *    Since a physical clap is a decaying sound wave that echoes, the sensor will trigger dozens of times
 *    during a single clap. The lockout window ignores these subsequent echoes, preventing double-toggles.
 * 2. It runs completely non-blockingly, allowing the CPU to check other systems.
 * 
 * THEORY OF OPERATION:
 * 1. Electret Condenser Microphone: The sensor uses a tiny capsule containing a flexible metallized 
 *    diaphragm placed close to a rigid backplate, forming a capacitor. Sound waves (air pressure waves)
 *    vibrate the diaphragm, changing the capacitance and generating a tiny AC voltage.
 * 2. LM393 Comparator IC: The module contains an operational amplifier acting as a comparator.
 *    - It compares the microphone's analog voltage to a reference threshold set by the onboard potentiometer.
 *    - When the sound is quiet: The output pin (OUT / DO) remains HIGH (5V).
 *    - When a loud sound (like a clap) exceeds the threshold: The output pin instantly pulls LOW (0V).
 * 3. Echo Decay & Lockout:
 *    A physical clap lasts about 50 to 100ms and contains multiple sound pressure peaks.
 *    To prevent the Arduino from register each peak as a separate clap (which would toggle the LED 
 *    on and off rapidly, ending in a random state), we lock out the sensor for 250ms after the first trigger.
 * 
 * WIRING:
 * - Sound Sensor VCC  -> Arduino 5V
 * - Sound Sensor GND  -> Arduino GND
 * - Sound Sensor OUT  -> Arduino Pin 3 (Digital Input)
 * - LED Anode (+)     -> 220 Ohm Resistor -> Arduino Pin 13 (or use onboard LED)
 * - LED Cathode (-)   -> Arduino GND
 */

// --- PIN DEFINITIONS ---
const int SOUND_PIN = 3; // Pin connected to the sound sensor digital output (OUT)
const int LED_PIN = 13;   // Pin connected to the target LED

// --- STATE VARIABLES ---
bool ledState = LOW;                  // Tracks the current state of the light (ON or OFF)
unsigned long lastClapTime = 0;       // Stores the timestamp of the last valid clap
const unsigned long lockoutDuration = 250; // Lockout window in milliseconds to ignore echoes (250ms is ideal)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure pins
  pinMode(SOUND_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Set initial state
  digitalWrite(LED_PIN, ledState);
  
  Serial.println("==================================================");
  Serial.println("Day 17: Sound Sensor Clap Switch (Anti-Echo)");
  Serial.println("==================================================");
  Serial.println("System armed. Clap your hands to toggle the light.");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Read the raw sensor state (LOW = Sound detected, HIGH = Quiet)
  int soundReading = digitalRead(SOUND_PIN);
  
  // Step 2: Check if sound is detected AND we are outside the lockout window
  if (soundReading == LOW && (currentTime - lastClapTime >= lockoutDuration)) {
    // Record the trigger time
    lastClapTime = currentTime;
    
    // Toggle the light state
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    
    // Log event
    Serial.print("[CLAP DETECTED] Toggling light to: ");
    if (ledState == HIGH) {
      Serial.println("ON");
    } else {
      Serial.println("OFF");
    }
  }
  
  // Non-blocking architecture - additional sensors or indicators can run here
}

/*
 * 100 Projects with Arduino - Day 6
 * Project: Photoresistor (LDR) Automatic Night Light with Hysteresis
 *
 * DESCRIPTION:
 * This project implements an automatic night light using a Light Dependent Resistor (LDR)
 * to measure ambient light. When the room gets dark, the LED automatically turns ON.
 * To make this code professional and robust for mechatronic applications, it implements:
 * 1. Hysteresis: This prevents the LED from rapidly flickering ON/OFF (known as relay chatter
 *    or noise bounce) when the light level is exactly at the trigger threshold.
 * 2. Non-blocking timing: Telemetry is logged to the Serial Monitor without freezing the CPU.
 *
 * THEORY OF OPERATION:
 * 1. The LDR (Photoresistor) is a semiconductor device whose resistance decreases as light
 *    intensity increases. In the dark, its resistance is very high (up to 1 Megohm). In bright
 *    light, its resistance drops to a few hundred Ohms.
 * 2. We use a voltage divider with a fixed 10k Ohm resistor. The voltage at the junction pin (A0)
 *    is read by the Arduino's 10-bit ADC.
 *    - Under bright light (low LDR resistance): Pin A0 voltage is pulled close to 5V (high ADC).
 *    - Under darkness (high LDR resistance): Pin A0 voltage drops close to 0V (low ADC).
 * 3. Hysteresis Logic:
 *    Instead of using a single threshold (e.g., "if light < 400, turn ON; else, turn OFF"), we use
 * two:
 *    - Turn ON Threshold (Dark): 350
 *    - Turn OFF Threshold (Light): 450
 *    If the reading is between 350 and 450, the LED simply maintains its current state. This
 * 100-point "dead band" prevents noise from causing rapid transitions.
 *
 * WIRING:
 * - 5V               -> LDR Pin A
 * - Analog Pin A0    -> Junction of LDR Pin B and 10k Ohm Resistor
 * - 10k Ohm Resistor -> Junction (A0) to GND
 * - LED Anode (+)    -> 220 Ohm Resistor -> Arduino Pin 9 (PWM)
 * - LED Cathode (-)  -> Arduino GND
 */

// --- PIN DEFINITIONS ---
const int LDR_PIN = A0;  // Analog input pin connected to the LDR divider junction
const int LED_PIN = 9;   // Output pin connected to the LED anode

// --- CONTROL PARAMETERS ---
const int DARK_THRESHOLD = 350;   // Turn LED ON when reading drops below this value (Darkness)
const int LIGHT_THRESHOLD = 450;  // Turn LED OFF when reading rises above this value (Light)
bool nightLightActive = false;    // Tracks the current state of our night light

// --- TIMING VARIABLES ---
unsigned long lastLogTime = 0;          // Stores last time we printed to the Serial Monitor
const unsigned long logInterval = 200;  // Logging frequency in milliseconds (5 Hz)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Set LED pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED OFF

  Serial.println("==================================================");
  Serial.println("Day 6: LDR Automatic Night Light with Hysteresis");
  Serial.println("==================================================");
  Serial.println("System Initialized. Monitor light levels below.");
}

void loop() {
  // Step 1: Read the light level (ADC value 0 - 1023)
  int lightLevel = analogRead(LDR_PIN);

  // Step 2: Apply Hysteresis Control Logic
  if (!nightLightActive && lightLevel < DARK_THRESHOLD) {
    // It has gotten sufficiently dark. Turn the light ON.
    nightLightActive = true;
    digitalWrite(LED_PIN, HIGH);
    Serial.println(">> Night Light Activated [ON] <<");
  } else if (nightLightActive && lightLevel > LIGHT_THRESHOLD) {
    // It has gotten sufficiently bright. Turn the light OFF.
    nightLightActive = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println(">> Night Light Deactivated [OFF] <<");
  }

  // If the light level is between DARK_THRESHOLD and LIGHT_THRESHOLD,
  // the LED state remains unchanged. This is the hysteresis dead band.

  // Step 3: Non-blocking telemetry output using millis()
  unsigned long currentTime = millis();
  if (currentTime - lastLogTime >= logInterval) {
    lastLogTime = currentTime;

    // Print telemetry log
    Serial.print("Ambient Light ADC: ");
    Serial.print(lightLevel);
    Serial.print(" | State: ");
    Serial.print(nightLightActive ? "ACTIVE (ON)" : "INACTIVE (OFF)");
    Serial.print(" | Margins: [ON < ");
    Serial.print(DARK_THRESHOLD);
    Serial.print(" | OFF > ");
    Serial.print(LIGHT_THRESHOLD);
    Serial.println("]");
  }

  // Non-blocking loop allows other systems to execute concurrently
}

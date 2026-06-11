/*
 * 100 Projects with Arduino - Day 8
 * Project: PIR Motion Sensor Alarm (Pyroelectric Infrared Sensor)
 *
 * DESCRIPTION:
 * This project interfaces a Passive Infrared (PIR) motion sensor with the Arduino to build
 * an intrusion alarm. When motion is detected, the system triggers the onboard LED and an
 * active buzzer. It implements:
 * 1. Warm-up Calibration Loop: PIR sensors require 30 to 60 seconds to calibrate to the room's
 *    ambient infrared signature. The sketch includes a visual startup calibration delay.
 * 2. Edge-Triggered State Tracking: Telemetry messages print only when motion *starts* or *stops*,
 *    keeping the Serial Monitor clean and professional.
 *
 * THEORY OF OPERATION:
 * 1. All objects with a temperature above absolute zero emit thermal radiation in the form of
 *    infrared (IR) light.
 * 2. The PIR sensor contains pyroelectric crystals that generate a tiny electrical charge when
 *    exposed to changes in infrared radiation. It is split into two halves. When a warm body (like
 *    a human or animal) walks past, it crosses one half before the other, creating a differential
 * voltage.
 * 3. The sensor's dome is a Fresnel Lens that focuses IR light from the room onto the sensor chip.
 * 4. Triggering States:
 *    - Idle: Output is LOW (0V).
 *    - Motion Detected: Output is HIGH (5V). The duration of this HIGH signal is adjusted via the
 *      onboard potentiometer (usually 3 seconds to 5 minutes).
 *
 * WIRING:
 * - PIR VCC   -> Arduino 5V
 * - PIR GND   -> Arduino GND
 * - PIR OUT   -> Arduino Pin 2 (Digital input)
 * - LED Anode -> 220 Ohm Resistor -> Arduino Pin 13 (or use onboard LED)
 * - Buzzer (+) -> Arduino Pin 8 (Active buzzer)
 * - Buzzer (-) -> Arduino GND
 */

// --- PIN DEFINITIONS ---
const int PIR_PIN = 2;     // Digital input pin connected to the PIR OUT pin
const int LED_PIN = 13;    // Output pin connected to the LED
const int BUZZER_PIN = 8;  // Output pin connected to the active piezo buzzer

// --- STATE VARIABLES ---
int lastPirState = LOW;  // Stores the previous state of the PIR sensor
const int CALIBRATION_TIME =
    15;  // PIR warm-up time in seconds (standard is 30-60s; 15s for quick demo)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Configure pin modes
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Make sure indicators are OFF initially
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("==================================================");
  Serial.println("Day 8: PIR Motion Sensor Alarm System");
  Serial.println("==================================================");

  // PIR Warm-up Calibration Phase
  Serial.print("PIR Sensor Warming Up & Calibrating (approx ");
  Serial.print(CALIBRATION_TIME);
  Serial.println("s)...");

  for (int i = 0; i < CALIBRATION_TIME; i++) {
    Serial.print(CALIBRATION_TIME - i);
    Serial.print("... ");

    // Blink the LED quickly during warm-up to indicate the system is not yet armed
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(750);
  }
  Serial.println("\nSYSTEM ARMED. PIR Sensor active!");
}

void loop() {
  // Step 1: Read the state of the PIR sensor (HIGH = Motion, LOW = Idle)
  int currentPirState = digitalRead(PIR_PIN);

  // Step 2: Detect state changes (edge triggering)
  if (currentPirState != lastPirState) {
    if (currentPirState == HIGH) {
      // Transition from LOW to HIGH: Motion has just started!
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("[ALERT] Motion Detected! Alarm Triggered.");
    } else {
      // Transition from HIGH to LOW: Motion has stopped and sensor timed out.
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("[INFO] Area Secure. Alarm cleared.");
    }

    // Save the current state as the last state for the next loop
    lastPirState = currentPirState;
  }

  // Non-blocking loop - we can add other code here (like sensor logs or motor sweeps)
}

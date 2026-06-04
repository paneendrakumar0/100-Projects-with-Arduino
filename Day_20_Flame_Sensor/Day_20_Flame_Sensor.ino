/*
 * 100 Projects with Arduino - Day 20
 * Project: Flame Sensor Fire Alarm System (Dual Analog/Digital Alert)
 * 
 * DESCRIPTION:
 * This project interfaces a photodiode-based flame sensor with the Arduino to build a fire alarm
 * system. When a flame is detected, a loud, pulsing piezo alarm sounds, and a status LED flashes.
 * To implement professional mechatronics and safety engineering standards:
 * 1. Dual-Path Reading: The code monitors the sensor's Analog Output (to measure flame size/distance)
 *    and its Digital Output (which acts as a fast comparator trigger).
 * 2. Adaptive Telemetry Rate: When idle, the system logs data once every 500ms. If a flame is detected,
 *    it shifts to a high-speed 100ms logging rate to capture emergency event details.
 * 3. Non-Blocking Pulse Alarm: The siren beeps and LED flashes are scheduled non-blockingly using
 *    millis(), ensuring the system remains responsive to immediate shutdown commands.
 * 
 * WIRING:
 * - Flame Sensor VCC -> Arduino 5V
 * - Flame Sensor GND -> Arduino GND
 * - Flame Sensor AO  -> Arduino Analog Pin A0 (Measures flame intensity)
 * - Flame Sensor DO  -> Arduino Digital Pin 2 (Fast threshold interrupt/input)
 * - Red LED Anode    -> 220 Ohm Resistor -> Arduino Pin 11
 * - Passive Buzzer (+)-> 100 Ohm Resistor -> Arduino Pin 12
 * - All Cathodes (-) -> Arduino GND
 */

// --- PIN DEFINITIONS ---
const int FLAME_ANALOG_PIN = A0;  // Analog input connected to sensor analog out
const int FLAME_DIGITAL_PIN = 2; // Digital input connected to sensor digital out
const int ALARM_LED_PIN = 11;    // Digital output driving the status warning LED
const int ALARM_BUZZER_PIN = 12; // Digital output driving the warning alarm buzzer

// --- CALIBRATION PARAMETERS ---
// Analog value decreases as infrared light increases.
// - Normal Room Light: 900 - 1023 (High resistance, low IR)
// - Flame detected: < 600 (Low resistance, high IR)
const int FLAME_THRESHOLD = 600; // Trigger alarm if analog reading drops below this level

// --- STATE VARIABLES ---
bool isAlarmActive = false;      // Tracks if a fire alarm is currently triggered
bool alarmPulseState = false;    // Tracks the current pulse state of the active buzzer/LED

// --- TIMING VARIABLES ---
unsigned long lastLogTime = 0;       // Stores last telemetry log timestamp
unsigned long lastPulseTime = 0;     // Stores last alarm pulse toggle timestamp
const unsigned long idleLogInterval = 500; // Log frequency when safe (500ms)
const unsigned long alertLogInterval = 100; // High-speed log frequency during alert (100ms)
const unsigned long alarmPulseDelay = 150;  // Alarm beep rate (150ms ON / 150ms OFF)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure pin modes
  pinMode(FLAME_DIGITAL_PIN, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(ALARM_BUZZER_PIN, OUTPUT);
  
  // Initialize outputs to OFF
  digitalWrite(ALARM_LED_PIN, LOW);
  digitalWrite(ALARM_BUZZER_PIN, LOW);
  
  Serial.println("==================================================");
  Serial.println("Day 20: Flame Sensor Fire Alarm System");
  Serial.println("==================================================");
  Serial.println("System armed. Monitoring optical IR spectrum...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Read the sensor data
  int rawAnalog = analogRead(FLAME_ANALOG_PIN);
  int digitalTrigger = digitalRead(FLAME_DIGITAL_PIN); // LOW means flame detected on comparator
  
  // Step 2: Determine if alarm should trigger
  // Trigger if either the analog threshold is breached OR the digital comparator activates
  if (rawAnalog < FLAME_THRESHOLD || digitalTrigger == LOW) {
    if (!isAlarmActive) {
      isAlarmActive = true;
      Serial.println("\n>>> [WARNING] FLAME DETECTED! INITIATING SIREN <<<");
    }
  } else {
    if (isAlarmActive) {
      isAlarmActive = false;
      digitalWrite(ALARM_LED_PIN, LOW);
      noTone(ALARM_BUZZER_PIN);
      Serial.println("\n>>> [SECURE] Flame extinguished. Silencing alarm. <<<");
    }
  }
  
  // Step 3: Run the Non-Blocking Alarm Siren (Pulsing Beep and Flash)
  if (isAlarmActive) {
    if (currentTime - lastPulseTime >= alarmPulseDelay) {
      lastPulseTime = currentTime;
      
      // Toggle pulse state
      alarmPulseState = !alarmPulseState;
      
      if (alarmPulseState) {
        digitalWrite(ALARM_LED_PIN, HIGH);
        // Play high-pitched alarm chirp (2500 Hz)
        tone(ALARM_BUZZER_PIN, 2500);
      } else {
        digitalWrite(ALARM_LED_PIN, LOW);
        noTone(ALARM_BUZZER_PIN);
      }
    }
  }
  
  // Step 4: Adaptive Telemetry logging using non-blocking scheduler
  unsigned long currentInterval = isAlarmActive ? alertLogInterval : idleLogInterval;
  if (currentTime - lastLogTime >= currentInterval) {
    lastLogTime = currentTime;
    
    Serial.print("Sensor AO: ");
    Serial.print(rawAnalog);
    Serial.print(" | DO Trigger: ");
    Serial.print(digitalTrigger == LOW ? "ACTIVE (LOW)" : "INACTIVE (HIGH)");
    Serial.print(" | Alarm State: ");
    Serial.println(isAlarmActive ? "!!! DANGER !!!" : "SAFE");
  }
}

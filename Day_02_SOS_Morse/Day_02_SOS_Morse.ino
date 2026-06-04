/*
 * 100 Projects with Arduino - Day 2
 * Project: SOS Morse Code Generator (Non-Blocking Timing & Arrays)
 * 
 * DESCRIPTION:
 * This project demonstrates how to make the Arduino blink an LED and beep an active
 * piezo buzzer in the standard Morse code SOS pattern (••• ——— •••).
 * Instead of using the blocking delay() function (which freezes the controller),
 * this code utilizes a non-blocking sequence player powered by a state array and
 * millis() timing. This is the professional way to handle sequence generation in robotics.
 * 
 * TIMING SPECIFICATION (Standard International Morse Code):
 * - Time Unit: 1 Unit = 200 ms
 * - Dot (•): 1 Unit ON
 * - Dash (—): 3 Units ON
 * - Element Gap (between dots/dashes of same letter): 1 Unit OFF
 * - Letter Gap: 3 Units OFF
 * - Word Gap: 7 Units OFF
 * 
 * WIRING:
 * - Active Buzzer (+) -> Arduino Pin 8
 * - Active Buzzer (-) -> Arduino GND
 * - LED Anode (+) -> Arduino Pin 13
 * - LED Cathode (-) -> 220 Ohm Resistor -> Arduino GND
 */

// --- PIN DEFINITIONS ---
const int LED_PIN = 13;      // Onboard LED pin
const int BUZZER_PIN = 8;    // Pin driving the active piezo buzzer

// --- TIMING CONSTANTS ---
const unsigned long UNIT_TIME = 200; // Base time unit in milliseconds (adjust to change Morse speed)

// --- SEQUENCE REPRESENTATION ---
// We represent the SOS sequence as a list of "events".
// Each event has a state (ON = true, OFF = false) and a duration in units.
struct MorseEvent {
  bool state;            // Output state (ON/OFF)
  unsigned int units;    // Duration of this state in time units
};

// The SOS Pattern:
// S: dot (1), gap (1), dot (1), gap (1), dot (1)      --> Letter S
// Gap between letters (3 units)
// O: dash (3), gap (1), dash (3), gap (1), dash (3)   --> Letter O
// Gap between letters (3 units)
// S: dot (1), gap (1), dot (1), gap (1), dot (1)      --> Letter S
// Gap between words (7 units)
const MorseEvent sosPattern[] = {
  // --- LETTER 'S' (dot, gap, dot, gap, dot) ---
  {true, 1},  // Dot 1
  {false, 1}, // Gap
  {true, 1},  // Dot 2
  {false, 1}, // Gap
  {true, 1},  // Dot 3
  
  // --- GAP BETWEEN LETTERS 'S' and 'O' ---
  {false, 3}, // 3 Units OFF total (we count from the end of the last dot)
  
  // --- LETTER 'O' (dash, gap, dash, gap, dash) ---
  {true, 3},  // Dash 1
  {false, 1}, // Gap
  {true, 3},  // Dash 2
  {false, 1}, // Gap
  {true, 3},  // Dash 3
  
  // --- GAP BETWEEN LETTERS 'O' and 'S' ---
  {false, 3}, 
  
  // --- LETTER 'S' (dot, gap, dot, gap, dot) ---
  {true, 1},  // Dot 1
  {false, 1}, // Gap
  {true, 1},  // Dot 2
  {false, 1}, // Gap
  {true, 1},  // Dot 3
  
  // --- GAP BETWEEN WORDS (Repeating the SOS message) ---
  {false, 7}  // 7 Units OFF
};

// Calculate the number of steps in our pattern
const int totalSteps = sizeof(sosPattern) / sizeof(sosPattern[0]);

// --- STATE VARIABLES ---
int currentStep = 0;                  // Tracks our index in the sosPattern array
unsigned long stepStartTime = 0;       // Stores the millisecond timestamp when the current step started
unsigned long currentStepDuration = 0; // The calculated millisecond duration of the current step

void setup() {
  // Initialize digital pins as outputs
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Open Serial port at 9600 bps for telemetry output
  Serial.begin(9600);
  Serial.println("==================================================");
  Serial.println("Day 2: Non-Blocking SOS Morse Code Generator");
  Serial.println("==================================================");
  Serial.println("System Initialized. Outputting SOS...");

  // Initialize the first step
  startStep(0);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if the current step's duration has elapsed
  if (currentTime - stepStartTime >= currentStepDuration) {
    // Move to the next step in the sequence
    currentStep++;
    
    // If we reached the end of the pattern, loop back to the beginning (Step 0)
    if (currentStep >= totalSteps) {
      currentStep = 0;
      Serial.println("--- Restarting SOS Message Loop ---");
    }
    
    // Initialize the next step
    startStep(currentStep);
  }
  
  // Note: Because this loop is completely non-blocking, we could read analog sensors,
  // check pushbuttons, or run PID motor loops here without disrupting the SOS signal!
}

/**
 * Initializes and starts a specific step in the Morse code sequence.
 * Sets the output state (LED & Buzzer) and calculates the millisecond duration.
 */
void startStep(int stepIndex) {
  // Record the start time of the new step
  stepStartTime = millis();
  
  // Retrieve the step details from our pattern array
  bool state = sosPattern[stepIndex].state;
  unsigned int units = sosPattern[stepIndex].units;
  
  // Calculate duration in milliseconds
  currentStepDuration = units * UNIT_TIME;
  
  // Apply the state to both our LED and active piezo buzzer
  if (state) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.print("• [ON]  Duration: ");
    Serial.print(currentStepDuration);
    Serial.println(" ms");
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.print("  [OFF] Duration: ");
    Serial.print(currentStepDuration);
    Serial.println(" ms");
  }
}

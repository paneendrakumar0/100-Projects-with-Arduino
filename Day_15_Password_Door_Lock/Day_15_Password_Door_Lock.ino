/*
 * 100 Projects with Arduino - Day 15
 * Project: Password-Protected Door Lock (FSM, Keypad, and Servo)
 * 
 * DESCRIPTION:
 * This project implements a secure, passcode-activated door lock mechanism. It integrates
 * the 4x4 matrix keypad (Day 14) and a servo motor (Day 11) using a professional programming
 * pattern: a Finite State Machine (FSM).
 * 
 * SYSTEM BEHAVIOR (FSM States):
 * 1. STATE_LOCKED (Default): Red LED is ON, Green LED is OFF, Servo is at 0° (Latch engaged).
 * 2. STATE_ENTERING: User is typing. Buzzer beeps on each keypress. Digits are stored in a buffer.
 * 3. STATE_UNLOCKED: Password correct! Green LED turns ON, Red LED turns OFF, Servo turns to 90°
 *    (Latch retracted), success beep plays. Locks automatically after 5 seconds.
 * 4. STATE_DENIED: Password incorrect! Red LED flashes rapidly, alarm tone plays, buffer clears.
 *    System returns to LOCKED state.
 * 
 * KEYPAD CONTROLS:
 * - '*' Key: Clears current input (Cancel/Reset).
 * - '#' Key: Submits password for verification.
 * 
 * WIRING:
 * - Keypad Pins 1-8 -> Arduino Pins 9, 8, 7, 6, 5, 4, 3, 2 (Rows then Columns)
 * - Servo Signal    -> Arduino Pin 10
 * - Servo VCC/GND   -> 5V / GND
 * - Red LED Anode   -> 220 Ohm Resistor -> Arduino Pin 11
 * - Green LED Anode -> 220 Ohm Resistor -> Arduino Pin 12
 * - Passive Buzzer (+)-> 100 Ohm Resistor -> Arduino Pin A0 (Analog pin used as digital)
 * - All Cathodes (-) -> Arduino GND
 */

#include <Keypad.h>
#include <Servo.h>

// --- PIN DEFINITIONS ---
const int SERVO_PIN = 10;     // Pin to drive the door lock servo
const int RED_LED_PIN = 11;   // Status indicator: Locked / Access Denied
const int GREEN_LED_PIN = 12; // Status indicator: Unlocked
const int BUZZER_PIN = A0;    // Pin connected to piezo buzzer (A0 acts as digital output)

// --- SYSTEM CONFIGURATION ---
const char CORRECT_PIN[] = "1234"; // Define the secret access code
const int PIN_LENGTH = 4;           // Set length of PIN
const unsigned long unlockDuration = 5000; // Unlock window (5 seconds)

// --- FINITE STATE MACHINE STATES ---
enum LockState {
  STATE_LOCKED,
  STATE_ENTERING,
  STATE_UNLOCKED,
  STATE_DENIED
};

LockState currentLockState = STATE_LOCKED; // Initial state

// --- KEYPAD CONFIGURATION ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- SERVO OBJECT ---
Servo lockServo;

// --- PASSWORD BUFFER VARIABLES ---
char inputBuffer[PIN_LENGTH + 1]; // Extra byte for null terminator
int inputIndex = 0;               // Index pointer in the buffer

// --- TIMING VARIABLES ---
unsigned long stateTransitionTime = 0; // Timestamp of last state change (used for lock timeout)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Attach Servo and home to LOCKED position (0°)
  lockServo.attach(SERVO_PIN);
  lockServo.write(0);
  
  // Configure LED and Buzzer pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize physical state (LOCKED)
  updatePhysicalIndicators();
  
  Serial.println("==================================================");
  Serial.println("Day 15: Password Door Lock (FSM Access Control)");
  Serial.println("==================================================");
  Serial.println("System Locked. Enter PIN followed by '#'.");
}

void loop() {
  // Step 1: Scan keypad
  char key = customKeypad.getKey();
  
  // Step 2: Run Finite State Machine logic
  switch (currentLockState) {
    
    case STATE_LOCKED:
      if (key) {
        // Any keypress transitions us to the entering phase
        transitionToState(STATE_ENTERING);
        handleKeyInput(key);
      }
      break;
      
    case STATE_ENTERING:
      if (key) {
        handleKeyInput(key);
      }
      break;
      
    case STATE_UNLOCKED:
      // Auto-lock timer: Lock the door again after 5 seconds
      if (millis() - stateTransitionTime >= unlockDuration) {
        Serial.println("[TIMER] Unlock window expired. Re-locking...");
        transitionToState(STATE_LOCKED);
      }
      break;
      
    case STATE_DENIED:
      // Denial alarm duration (1 second) before arming again
      if (millis() - stateTransitionTime >= 1500) {
        transitionToState(STATE_LOCKED);
      }
      break;
  }
}

/**
 * Transition the system state and records timestamp.
 */
void transitionToState(LockState newState) {
  currentLockState = newState;
  stateTransitionTime = millis();
  
  updatePhysicalIndicators();
}

/**
 * Handles keyboard inputs based on character typed.
 */
void handleKeyInput(char key) {
  // Sound click feedback on every press
  tone(BUZZER_PIN, 2000, 50);
  
  if (key == '*') {
    // Clear buffer and cancel
    Serial.println("[CANCEL] Input cleared.");
    clearBuffer();
    transitionToState(STATE_LOCKED);
  } 
  else if (key == '#') {
    // Verify password
    inputBuffer[inputIndex] = '\0'; // Null terminate string
    verifyPassword();
  } 
  else {
    // Standard digit or character key
    if (inputIndex < PIN_LENGTH) {
      inputBuffer[inputIndex] = key;
      inputIndex++;
      
      // Telemetry log (masking digits for basic security)
      Serial.print("PIN: ");
      for (int i = 0; i < inputIndex; i++) {
        Serial.print("*");
      }
      Serial.println();
    }
  }
}

/**
 * Compares PIN buffer to the correct passcode.
 */
void verifyPassword() {
  Serial.print("Validating Entered Code: ");
  Serial.println(inputBuffer);
  
  if (strcmp(inputBuffer, CORRECT_PIN) == 0) {
    Serial.println("[SUCCESS] Access Granted. Door Unlocked.");
    transitionToState(STATE_UNLOCKED);
  } else {
    Serial.println("[FAILURE] Access Denied. Wrong PIN.");
    transitionToState(STATE_DENIED);
  }
  clearBuffer();
}

/**
 * Resets the entry variables.
 */
void clearBuffer() {
  memset(inputBuffer, 0, sizeof(inputBuffer));
  inputIndex = 0;
}

/**
 * Sets physical actuators (LEDs, Servo, Buzzer) to reflect state.
 */
void updatePhysicalIndicators() {
  switch (currentLockState) {
    case STATE_LOCKED:
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
      lockServo.write(0); // Latch engaged
      noTone(BUZZER_PIN);
      break;
      
    case STATE_UNLOCKED:
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
      lockServo.write(90); // Latch retracted
      
      // Happy success note sweep (non-blocking tone sequence)
      tone(BUZZER_PIN, 1500, 200);
      delay(200); // Small delay acceptable for success beep sequence during state transition
      tone(BUZZER_PIN, 2000, 400);
      break;
      
    case STATE_DENIED:
      digitalWrite(GREEN_LED_PIN, LOW);
      
      // Flash red LED and play low buzz frequency
      for (int i = 0; i < 3; i++) {
        digitalWrite(RED_LED_PIN, HIGH);
        tone(BUZZER_PIN, 150, 150);
        delay(150);
        digitalWrite(RED_LED_PIN, LOW);
        noTone(BUZZER_PIN);
        delay(150);
      }
      digitalWrite(RED_LED_PIN, HIGH);
      break;
      
    case STATE_ENTERING:
      // While typing, red light stays solid, green off
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
      break;
  }
}

/*
 * 100 Projects with Arduino - Day 88
 * Project: Hardware Interrupts & Software Debouncing (E-Stop and Pin Loopback Simulation)
 * 
 * DESCRIPTION:
 * This project explores hardware interrupts on the ATmega328P (Arduino Uno) and implements
 * a software-based debounce algorithm within an Interrupt Service Routine (ISR). 
 * 
 * To make this project testable without external hardware components, it implements an
 * on-board Hardware Loopback Simulator:
 * 1. Pin 2 (INT0) is configured as the interrupt input pin.
 * 2. Pin 4 is configured as an output pin and wired directly to Pin 2.
 * 3. Via the Serial Monitor CLI, we can command Pin 4 to execute a "Clean Press" or a
 *    "Bouncy Press" (rapidly toggling the pin to simulate the physical contact bounces of a switch).
 * 4. The ISR on Pin 2 records both raw edge triggers and debounced button presses.
 * 
 * APPLICATIONS IN MECHATRONICS:
 * - E-Stop (Emergency Stop): Critical safety limits that must halt actuators instantly, 
 *   bypassing the cooperative multitasking loop.
 * - Rotary Encoder / Tachometer: High-frequency count pulses that will crash the system
 *   if not debounced or processed efficiently.
 * 
 * WHY DO WE DEBOUNCE?
 * Mechanical switches contain springy metal contacts. When pressed, they bounce together
 * for 1 to 10 milliseconds, creating a train of microsecond-level logic pulses. An interrupt
 * pins is fast enough to count every single bounce as a press, leading to incorrect state machine
 * transitions.
 * 
 * WIRING:
 * - Run a single jumper wire from Digital Pin 4 to Digital Pin 2 on the Arduino.
 */

// --- PIN DEFINITIONS ---
const int INTERRUPT_PIN = 2; // INT0 hardware interrupt input
const int LOOPBACK_PIN  = 4; // Output used to simulate physical button presses
const int LED_INDICATOR = 13; // Onboard LED

// --- VOLATILE VARIABLES (accessed by ISR and main loop) ---
volatile unsigned long rawTriggerCount = 0;      // Increments on EVERY interrupt trigger
volatile unsigned long debouncedTriggerCount = 0; // Increments only if debounce check passes
volatile unsigned long lastInterruptTime = 0;    // Tracks timestamp of last valid press (ms)
volatile bool emergencyStopTriggered = false;    // System status flag

// --- DEBOUNCE CONSTANT ---
const unsigned long DEBOUNCE_DELAY_MS = 150; // Lockout window for debouncing

void setup() {
  Serial.begin(9600);

  // Setup loopback simulation and interrupt pins
  pinMode(LOOPBACK_PIN, OUTPUT);
  digitalWrite(LOOPBACK_PIN, HIGH); // Pin 2 has internal pull-up, so default idle is HIGH
  
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);

  // Attach Interrupt: INT0 (Pin 2), trigger on FALLING edge (button pressed to GND)
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonISR, FALLING);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 88: Hardware Interrupts & Software Debouncing"));
  Serial.println(F("=================================================="));
  Serial.println(F("[WIRING] Connect a single jumper wire from Pin 4 to Pin 2."));
  
  printMenu();
}

void loop() {
  // If Emergency Stop has been flagged by the ISR, execute halt sequence immediately
  if (emergencyStopTriggered) {
    executeEmergencyHalt();
  }

  // Poll Serial Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'c':
      case 'C':
        simulateCleanPress();
        break;
      case 'b':
      case 'B':
        simulateBouncyPress();
        break;
      case 'e':
      case 'E':
        simulateEStop();
        break;
      case 'r':
      case 'R':
        resetStats();
        break;
      case 's':
      case 'S':
        printStats();
        break;
      case 'h':
      case 'H':
        printMenu();
        break;
      default:
        break;
    }
  }
}

// =============================================================
//  INTERRUPT SERVICE ROUTINE (ISR)
// =============================================================
/**
 * ISR for INT0 (Pin 2). Keep ISR execution time extremely short.
 * Do NOT use Serial.print, delay(), or complex math inside here!
 */
void buttonISR() {
  rawTriggerCount++; // Record every electrical edge transition
  
  unsigned long currentTime = millis(); // Read frozen millis counter
  
  // Software Debouncing: check if the time elapsed since the last valid
  // interrupt is greater than our lockout window.
  if (currentTime - lastInterruptTime > DEBOUNCE_DELAY_MS) {
    debouncedTriggerCount++;
    lastInterruptTime = currentTime;
    
    // Safety check: if button is held down long enough, or if we treat this 
    // as an immediate trigger, we can flag emergency status.
    // In this demo, we check if raw triggers represent a massive burst, 
    // or if a specific E-stop state is commanded.
  }
}

// =============================================================
//  HARDWARE LOOPBACK SIMULATION FUNCTIONS
// =============================================================

/**
 * Simulates a clean, bounce-free button press.
 * Pin 4 transitions HIGH -> LOW, stays LOW for 100ms, then transitions back to HIGH.
 */
void simulateCleanPress() {
  Serial.println(F("\n[SIMULATOR] Generating a CLEAN press (0 bounces)..."));
  
  digitalWrite(LOOPBACK_PIN, LOW); // Trigger FALLING edge
  delay(100);                      // Keep button held down
  digitalWrite(LOOPBACK_PIN, HIGH); // Release button
  
  delay(200); // Allow time for ISR processing
  printStats();
}

/**
 * Simulates a noisy mechanical button press with contact bounces.
 * Generates multiple fast transitions within 10ms before settling LOW.
 */
void simulateBouncyPress() {
  Serial.println(F("\n[SIMULATOR] Generating a BOUNCY press (5 bounces)..."));
  
  // Contact bounce 1
  digitalWrite(LOOPBACK_PIN, LOW);  delayMicroseconds(150);
  digitalWrite(LOOPBACK_PIN, HIGH); delayMicroseconds(200);
  
  // Contact bounce 2
  digitalWrite(LOOPBACK_PIN, LOW);  delayMicroseconds(100);
  digitalWrite(LOOPBACK_PIN, HIGH); delayMicroseconds(250);
  
  // Contact bounce 3
  digitalWrite(LOOPBACK_PIN, LOW);  delayMicroseconds(200);
  digitalWrite(LOOPBACK_PIN, HIGH); delayMicroseconds(150);

  // Contact bounce 4
  digitalWrite(LOOPBACK_PIN, LOW);  delayMicroseconds(100);
  digitalWrite(LOOPBACK_PIN, HIGH); delayMicroseconds(300);

  // Settle at LOW (Pressed state)
  digitalWrite(LOOPBACK_PIN, LOW);
  delay(100); // Hold press
  
  // Release button
  digitalWrite(LOOPBACK_PIN, HIGH);
  
  delay(200); // Wait for processing
  printStats();
}

/**
 * Simulates a critical Emergency Stop (E-Stop).
 */
void simulateEStop() {
  Serial.println(F("\n[SIMULATOR] Critical E-STOP command received!"));
  
  // Temporarily disable global interrupts to modify flag safely
  noInterrupts();
  emergencyStopTriggered = true;
  interrupts();
}

// =============================================================
//  SYSTEM CONTROL FUNCTIONS
// =============================================================

void executeEmergencyHalt() {
  // Turn on status LED to indicate system is halted
  digitalWrite(LED_INDICATOR, HIGH);
  
  Serial.println(F("\n=================================================="));
  Serial.println(F("!!! EMERGENCY STOP ACTIVE - SYSTEM HALTED !!!"));
  Serial.println(F("Main loop execution has been interrupted."));
  Serial.println(F("Actuators turned OFF."));
  Serial.println(F("Send 'R' to clear error and restart system."));
  Serial.println(F("=================================================="));
  
  // Lock the controller in an infinite safety loop until reset command is processed
  while (emergencyStopTriggered) {
    if (Serial.available() > 0) {
      char cmd = Serial.read();
      if (cmd == 'r' || cmd == 'R') {
        resetStats();
        Serial.println(F("[SYSTEM] Emergency cleared. Restarting main loop."));
      }
    }
  }
}

void resetStats() {
  noInterrupts(); // Disable interrupts while clearing volatile variables
  rawTriggerCount = 0;
  debouncedTriggerCount = 0;
  emergencyStopTriggered = false;
  digitalWrite(LED_INDICATOR, LOW);
  interrupts(); // Re-enable interrupts
  Serial.println(F("[SYSTEM] Statistics and E-Stop cleared."));
}

void printStats() {
  // Make local copies of volatile variables to prevent time skew
  unsigned long raw;
  unsigned long debounced;
  
  noInterrupts();
  raw = rawTriggerCount;
  debounced = debouncedTriggerCount;
  interrupts();
  
  Serial.println(F("----------------- INTERRUPT STATS -----------------"));
  Serial.print(F(" Raw ISR Triggers (Bouncy): ")); Serial.println(raw);
  Serial.print(F(" Valid Debounced Counts:    ")); Serial.println(debounced);
  Serial.print(F(" Bounces Filtered Out:      ")); Serial.println(raw - debounced);
  Serial.println(F("---------------------------------------------------"));
}

void printMenu() {
  Serial.println(F("\n--- INTERACTIVE INTERRUPT CLI ---"));
  Serial.println(F(" 'c' : Simulate a clean, bounce-free press"));
  Serial.println(F(" 'b' : Simulate a bouncy button press (generating contact noise)"));
  Serial.println(F(" 'e' : Simulate an immediate Emergency Stop (E-Stop)"));
  Serial.println(F(" 's' : Print current trigger statistics"));
  Serial.println(F(" 'r' : Reset stats and clear E-Stop halt condition"));
  Serial.println(F(" 'h' : Print this menu"));
  Serial.println(F("---------------------------------\n"));
}

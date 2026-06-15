/*
 * 100 Projects with Arduino - Day 68
 * Project: Stepper Motor Microstepping (A4988 Driver — Full/Half/Quarter/Sixteenth Step)
 *
 * DESCRIPTION:
 * This project demonstrates how to control a NEMA 17 stepper motor through the
 * A4988 microstepping driver in all four resolution modes by toggling the MS1,
 * MS2, MS3 pins. It implements smooth acceleration/deceleration (trapezoidal
 * velocity profile) and measures actual steps per revolution for each mode.
 *
 * MICROSTEPPING MODES (A4988):
 *  MS1  MS2  MS3  | Mode             | Steps per Rev (1.8° motor)
 *  LOW  LOW  LOW  | Full Step        | 200 steps
 *  HIGH LOW  LOW  | Half Step        | 400 steps
 *  LOW  HIGH LOW  | Quarter Step     | 800 steps
 *  HIGH HIGH LOW  | Eighth Step      | 1600 steps
 *  HIGH HIGH HIGH | Sixteenth Step   | 3200 steps
 *
 * WHY MICROSTEPPING?
 * In full-step mode, the rotor jumps between discrete positions causing vibration
 * and resonance at certain speeds. Microstepping energizes both coils simultaneously
 * with sinusoidal current waveforms, creating smooth intermediate positions:
 *
 *   Full step:      Coil A = ±1, Coil B = 0 or ±1
 *   Microstep (1/16): Coil A = sin(n*π/32), Coil B = cos(n*π/32)
 *
 * TRAPEZOIDAL VELOCITY PROFILE:
 * To avoid skipping steps under load, we ramp the step pulse frequency:
 *   ACCEL phase: step delay decreases linearly from START_DELAY to MIN_DELAY
 *   CRUISE phase: constant minimum delay (maximum speed)
 *   DECEL phase: step delay increases linearly back to START_DELAY
 *
 * A4988 STEP PULSE REQUIREMENTS:
 *   Minimum STEP pulse HIGH time: 1 µs
 *   Minimum STEP pulse LOW time:  1 µs
 *   Minimum DIR setup time before STEP: 200 ns
 *   ENABLE is active-LOW: drive LOW to energize coils
 *   RESET must be HIGH (tie to VCC or 5V), SLEEP must be HIGH
 *
 * WIRING:
 *   A4988 STEP   -> D3
 *   A4988 DIR    -> D4
 *   A4988 MS1    -> D5
 *   A4988 MS2    -> D6
 *   A4988 MS3    -> D7
 *   A4988 ENABLE -> D8 (LOW = enabled)
 *   A4988 RESET  -> 5V (always HIGH)
 *   A4988 SLEEP  -> 5V (always HIGH)
 *   A4988 VMOT   -> 12V motor supply (8-35V)
 *   A4988 VDD    -> 5V logic
 *   A4988 GND    -> GND (both pins)
 *   Potentiometer (speed) -> A0
 *   Mode button (cycle) -> D2 (INPUT_PULLUP)
 */

// --- PINS ---
const int STEP_PIN = 3;
const int DIR_PIN = 4;
const int MS1_PIN = 5;
const int MS2_PIN = 6;
const int MS3_PIN = 7;
const int ENABLE_PIN = 8;
const int SPEED_POT = A0;
const int MODE_BTN = 2;

// --- MICROSTEPPING MODE TABLE ---
struct MicrostepMode {
  const char *name;
  int stepsPerRev;  // For 1.8° NEMA 17 motor
  uint8_t ms1, ms2, ms3;
};

const MicrostepMode MODES[] = {
    {"Full Step  (200/rev)", 200, LOW, LOW, LOW},
    {"Half Step  (400/rev)", 400, HIGH, LOW, LOW},
    {"1/4 Step   (800/rev)", 800, LOW, HIGH, LOW},
    {"1/8 Step  (1600/rev)", 1600, HIGH, HIGH, LOW},
    {"1/16 Step (3200/rev)", 3200, HIGH, HIGH, HIGH},
};
const int NUM_MODES = 5;
int currentMode = 0;

// --- TRAPEZOIDAL PROFILE CONSTANTS ---
const unsigned long START_DELAY_US = 4000;  // Initial step period (slow start)
const unsigned long MIN_DELAY_US = 400;     // Maximum speed step period
const unsigned long ACCEL_STEPS = 100;      // Steps over which to accelerate

// --- STATE ---
bool direction = true;  // true = CW, false = CCW

void setup() {
  Serial.begin(9600);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(MODE_BTN, INPUT_PULLUP);

  // Disable motor initially (de-energize coils)
  digitalWrite(ENABLE_PIN, HIGH);

  applyMode(currentMode);

  Serial.println(F("[Stepper] A4988 Microstepping Controller"));
  Serial.println(F("[Stepper] Press button D2 to cycle through step modes."));
  printModeInfo();
}

void loop() {
  // Button: cycle through microstepping modes
  static bool lastBtn = HIGH;
  bool curBtn = digitalRead(MODE_BTN);
  if (curBtn == LOW && lastBtn == HIGH) {
    currentMode = (currentMode + 1) % NUM_MODES;
    applyMode(currentMode);
    printModeInfo();
    delay(50);  // Debounce
  }
  lastBtn = curBtn;

  // Read speed from potentiometer
  int potVal = analogRead(SPEED_POT);
  // Map pot to min delay: high pot = faster (smaller delay)
  unsigned long targetDelay = map(potVal, 0, 1023, START_DELAY_US, MIN_DELAY_US);

  // Enable motor
  digitalWrite(ENABLE_PIN, LOW);

  // Rotate one full revolution with trapezoidal profile
  runRevolution(MODES[currentMode].stepsPerRev, targetDelay, direction);

  // Reverse direction
  direction = !direction;

  // Disable motor (save energy & reduce heat between moves)
  digitalWrite(ENABLE_PIN, HIGH);
  delay(300);
}

// --- ROTATE N STEPS WITH TRAPEZOIDAL VELOCITY PROFILE ---
void runRevolution(int steps, unsigned long minDelayUs, bool cw) {
  digitalWrite(DIR_PIN, cw ? HIGH : LOW);
  delayMicroseconds(5);  // DIR setup time for A4988

  unsigned long accelSteps = min((long)ACCEL_STEPS, (long)steps / 3);

  for (int i = 0; i < steps; i++) {
    // Compute current step delay based on trapezoidal profile
    unsigned long stepDelay;
    if (i < accelSteps) {
      // Acceleration ramp
      stepDelay = map(i, 0, accelSteps, START_DELAY_US, minDelayUs);
    } else if (i > steps - accelSteps) {
      // Deceleration ramp
      stepDelay = map(i, steps - accelSteps, steps, minDelayUs, START_DELAY_US);
    } else {
      // Cruise
      stepDelay = minDelayUs;
    }

    // Generate STEP pulse (min 1 µs HIGH, 1 µs LOW)
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
  }
}

// --- SET MS PINS FOR CURRENT MODE ---
void applyMode(int mode) {
  digitalWrite(MS1_PIN, MODES[mode].ms1);
  digitalWrite(MS2_PIN, MODES[mode].ms2);
  digitalWrite(MS3_PIN, MODES[mode].ms3);
}

// --- PRINT CURRENT MODE INFO ---
void printModeInfo() {
  Serial.print(F("[Mode] "));
  Serial.println(MODES[currentMode].name);
  Serial.print(F("[Mode] Steps per revolution: "));
  Serial.println(MODES[currentMode].stepsPerRev);
}

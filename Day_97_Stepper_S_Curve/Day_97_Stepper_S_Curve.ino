/*
 * 100 Projects with Arduino - Day 97
 * Project: Stepper Motor S-Curve Acceleration Profiler
 *
 * DESCRIPTION:
 * This project implements a real-time S-Curve (Raised Cosine) Acceleration Profile Generator
 * for stepper motors.
 *
 * TRAPEZOIDAL vs S-CURVE ACCELERATION:
 * 1. Trapezoidal (Linear): Acceleration changes instantly from 0 to max at the start and end
 *    of the ramp. This creates infinite "jerk" (the rate of change of acceleration), which causes
 *    mechanical vibration, resonance, and motor stall at high speeds.
 * 2. S-Curve (Smooth): Acceleration ramp is rounded off, gradually ramping up and down. This
 *    minimizes jerk, resulting in smooth motion, quiet operation, and higher achievable speeds.
 *
 * THE MATHEMATICAL MODEL:
 * We precompute the step intervals (delay in microseconds between steps) for a 100-step ramp.
 * The velocity v(i) at step index i (from 0 to N-1) is modeled using a raised cosine:
 *   v(i) = vStart + (vMax - vStart) * [ (1 - cos(pi * i / (N - 1))) / 2 ]
 *
 * The corresponding step delay interval in microseconds is:
 *   delay(i) = 1,000,000 / v(i)
 *
 * INTERACTIVE CLI & PLOTTER TELEMETRY:
 * The Serial CLI lets the user set target steps and switch between three profiles:
 * - Constant Speed (No ramp)
 * - Linear (Trapezoidal) Ramping
 * - S-Curve Ramping
 * During movement, the current speed is printed to the Serial Monitor in real-time,
 * allowing visualization of the speed curves on the Serial Plotter.
 *
 * WIRING:
 * - Stepper Driver STEP Pin -> Pin 9
 * - Stepper Driver DIR Pin  -> Pin 8
 * - Onboard LED             -> Pin 13 (Toggles on step execution)
 */

// --- PIN DEFINITIONS ---
const int STEP_PIN = 9;
const int DIR_PIN = 8;
const int LED_PIN = 13;

// --- STEPPER CONSTANTS ---
const int RAMP_STEPS = 100;  // Number of steps in the acceleration/deceleration ramp
float vStart = 150.0f;       // Starting velocity (steps/second)
float vMax = 1200.0f;        // Maximum peak velocity (steps/second)

// --- MEMORY ARRAYS FOR RAMP PROFILES ---
uint16_t sCurveIntervals[RAMP_STEPS];  // Step delay intervals in microseconds (S-Curve)
uint16_t linearIntervals[RAMP_STEPS];  // Step delay intervals in microseconds (Linear)

// --- MOTION STATE VARIABLES ---
enum ProfileType { PROFILE_CONSTANT, PROFILE_LINEAR, PROFILE_SCURVE };

ProfileType currentProfile = PROFILE_SCURVE;
long targetSteps = 0;
long stepsRemaining = 0;
long stepsCompleted = 0;
bool motorRunning = false;

void setup() {
  Serial.begin(9600);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // Precompute the profiles based on configured speeds
  generateProfileTables();

  Serial.println(F("=================================================="));
  Serial.println(F("Day 97: Stepper Motor S-Curve Speed Profiler"));
  Serial.println(F("=================================================="));

  printMenu();
}

void loop() {
  // Run the motion controller (Non-blocking stepping logic)
  if (motorRunning) {
    executeMotionStep();
  }

  // Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCLICommand(cmd);
  }
}

// =============================================================
//  PROFILE GENERATOR MATHEMATICS
// =============================================================
void generateProfileTables() {
  Serial.println(F("[SYSTEM] Precomputing speed profile lookup tables..."));

  for (int i = 0; i < RAMP_STEPS; i++) {
    float fraction = (float)i / (float)(RAMP_STEPS - 1);

    // 1. S-Curve (Raised Cosine) Math:
    // S-curve velocity goes from vStart to vMax following a smooth s-shape
    float sCurveFraction = (1.0f - cos(PI * fraction)) / 2.0f;
    float sCurveVel = vStart + (vMax - vStart) * sCurveFraction;
    sCurveIntervals[i] = (uint16_t)(1000000.0f / sCurveVel);

    // 2. Linear (Trapezoidal) Math:
    // Linear velocity increases straight from vStart to vMax
    float linearVel = vStart + (vMax - vStart) * fraction;
    linearIntervals[i] = (uint16_t)(1000000.0f / linearVel);
  }
  Serial.println(F("[SYSTEM] Precomputation completed. Ready."));
}

// =============================================================
//  MOTION CONTROL STEP EXECUTION
// =============================================================

/**
 * Non-blocking stepper driver step planner.
 * Calculates delay for current step and toggles step pin when delay expires.
 */
void executeMotionStep() {
  static unsigned long lastStepMicros = 0;
  static uint16_t currentIntervalUs = 10000;

  unsigned long currentMicros = micros();

  // Wait until the step interval has expired
  if (currentMicros - lastStepMicros >= currentIntervalUs) {
    lastStepMicros = currentMicros;

    // Trigger physical step pulse (minimum 10 microseconds HIGH)
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN, LOW);

    stepsCompleted++;
    stepsRemaining--;

    // Toggle status LED every 10 steps to show activity
    if (stepsCompleted % 10 == 0) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }

    // Determine current speed based on ramp position
    float speedHz = 0.0f;

    if (stepsRemaining == 0) {
      // Motion complete
      motorRunning = false;
      Serial.println(F("\n[MOTION] Target reached. Motor Stopped."));
      return;
    }

    // Calculate velocity profile interval
    if (currentProfile == PROFILE_CONSTANT) {
      // Constant velocity: always use max velocity interval
      currentIntervalUs = (uint16_t)(1000000.0f / vMax);
      speedHz = vMax;
    } else {
      // Ramping velocity
      uint16_t* profileTable =
          (currentProfile == PROFILE_SCURVE) ? sCurveIntervals : linearIntervals;

      long totalSteps = stepsCompleted + stepsRemaining;

      if (stepsCompleted < RAMP_STEPS) {
        // Acceleration phase
        currentIntervalUs = profileTable[stepsCompleted];
      } else if (stepsRemaining < RAMP_STEPS) {
        // Deceleration phase
        currentIntervalUs = profileTable[stepsRemaining];
      } else {
        // Flat peak velocity phase
        currentIntervalUs = (uint16_t)(1000000.0f / vMax);
      }

      speedHz = 1000000.0f / (float)currentIntervalUs;
    }

    // Output telemetry data formatted for the Serial Plotter (1 out of every 5 steps to throttle
    // output)
    if (stepsCompleted % 2 == 0) {
      Serial.print(F("Step:"));
      Serial.print(stepsCompleted);
      Serial.print(F(",SpeedHz:"));
      Serial.println(speedHz, 1);
    }
  }
}

// =============================================================
//  CLI COMMAND INTERFACE
// =============================================================
void handleCLICommand(char cmd) {
  if (cmd == '\n' || cmd == '\r') return;

  switch (cmd) {
    case 'g':
    case 'G':
      // Read target steps from Serial
      targetSteps = Serial.parseInt();
      if (targetSteps == 0) return;

      // Configure direction
      if (targetSteps > 0) {
        digitalWrite(DIR_PIN, HIGH);
        Serial.print(F("\n[MOTION] Clockwise Move: "));
      } else {
        digitalWrite(DIR_PIN, LOW);
        targetSteps = -targetSteps;
        Serial.print(F("\n[MOTION] Counter-Clockwise Move: "));
      }

      Serial.print(targetSteps);
      Serial.print(F(" steps using "));

      if (currentProfile == PROFILE_CONSTANT)
        Serial.println(F("CONSTANT SPEED profile."));
      else if (currentProfile == PROFILE_LINEAR)
        Serial.println(F("LINEAR TRAPEZOIDAL profile."));
      else
        Serial.println(F("S-CURVE profile."));

      // Start movement
      stepsRemaining = targetSteps;
      stepsCompleted = 0;
      motorRunning = true;
      break;

    // Profile settings selectors
    case '0':
      currentProfile = PROFILE_CONSTANT;
      Serial.println(F("[CONFIG] Profile set to: CONSTANT (No acceleration)"));
      break;
    case '1':
      currentProfile = PROFILE_LINEAR;
      Serial.println(F("[CONFIG] Profile set to: LINEAR (Trapezoidal ramp)"));
      break;
    case '2':
      currentProfile = PROFILE_SCURVE;
      Serial.println(F("[CONFIG] Profile set to: S-CURVE (Raised Cosine ramp)"));
      break;

    // Adjust speed boundaries
    case 'u':
      vMax = constrain(vMax + 100.0f, vStart + 100.0f, 2500.0f);
      generateProfileTables();
      Serial.print(F("[CONFIG] Peak Velocity increased to: "));
      Serial.print(vMax);
      Serial.println(F(" steps/sec"));
      break;
    case 'd':
      vMax = constrain(vMax - 100.0f, vStart + 100.0f, 2500.0f);
      generateProfileTables();
      Serial.print(F("[CONFIG] Peak Velocity decreased to: "));
      Serial.print(vMax);
      Serial.println(F(" steps/sec"));
      break;

    case 'h':
    case 'H':
      printMenu();
      break;

    default:
      break;
  }
}

void printMenu() {
  Serial.println(F("\n--- STEPPER PROFILER CLI ---"));
  Serial.println(F(" Configure profile:"));
  Serial.println(F(" '0' : Set profile to CONSTANT SPEED"));
  Serial.println(F(" '1' : Set profile to LINEAR TRAPEZOIDAL RAMP"));
  Serial.println(F(" '2' : Set profile to S-CURVE RAMP"));
  Serial.println(F(" Execute movement:"));
  Serial.println(F(" 'g <steps>' : Command steps relative movement (e.g., 'g 1000' or 'g -600')"));
  Serial.println(F(" Tunings:"));
  Serial.println(F(" 'u' : Increase peak speed (+100 steps/sec)"));
  Serial.println(F(" 'd' : Decrease peak speed (-100 steps/sec)"));
  Serial.println(F(" 'h' : Display this help command reference"));
  Serial.println(F("-----------------------------\n"));
}

/*
 * 100 Projects with Arduino - Day 35
 * Project: High-Performance Bipolar Stepper Control (A4988 & Trapezoidal Ramping)
 *
 * DESCRIPTION:
 * This project interfaces a NEMA 17 bipolar stepper motor using the A4988 carrier driver board.
 * To implement professional CNC/robotics-grade motion control:
 * 1. Step/Direction Interface: Simplifies GPIO overhead by utilizing only two signal pins (STEP,
 * DIR) to control speed and shaft angle.
 * 2. Trapezoidal Velocity Ramping: Implements linear acceleration and deceleration profiles
 *    (non-blocking speed updates) to prevent motor stalling and slippage due to rotor inertia.
 * 3. Microsecond Pulse Gating: Uses micros() timing to trigger step pulses non-blockingly at
 * frequencies up to several kilohertz.
 * 4. Driver Power-Gating: Integrates the ENABLE pin to disable power delivery to the motor coils
 *    during standstill, conserving power and preventing driver heating.
 *
 * BIPOLAR STEPPER & DRIVER THEORY:
 * - Bipolar Steppers: Have two internal coils (A and B) with no center-taps. Changing direction
 * requires reversing current flow inside the entire coil. This requires a dual H-bridge driver.
 * - A4988 Microstepping Driver: Contains a dual H-bridge with adjustable current-limiting
 * (chopping). Instead of raw coil switching, it regulates current to sinusoidal fractions
 * (microstepping). This divides a standard 1.8° full step into 1/2, 1/4, 1/8, or 1/16 subdivisions,
 * resulting in:
 *     - Finer angular resolution (200 steps * 16 = 3200 steps/rev in 1/16 mode).
 *     - Drastically reduced resonance (vibrations) and silent operation.
 * - Trapezoidal Velocity Profile:
 *   Due to rotor and load inertia, a stepper motor cannot instantly start at high speeds; doing so
 * causes the magnetic field to outpace the rotor, leading to motor stall (slipping poles). We must
 * ramp speed:
 *     - Phase 1: Constant Acceleration (increasing step frequency).
 *     - Phase 2: Cruise (constant maximum velocity).
 *     - Phase 3: Constant Deceleration (decreasing step frequency).
 *
 * WIRING:
 * - A4988 Driver -> Arduino Uno
 *   - STEP        -> Pin 5
 *   - DIR         -> Pin 6
 *   - ENABLE      -> Pin 7
 *   - RESET & SLEEP pins on A4988 MUST be tied together!
 *   - GND         -> Arduino GND
 *   - VDD         -> Arduino 5V (Logic power)
 * - Power Connections:
 *   - VMOT        -> Positive (+) terminal of external motor battery (8V-35V)
 *   - GND (Power) -> Negative (-) terminal of external battery
 * - Motor Connections:
 *   - 1A & 1B     -> Stepper Coil A leads (usually Red & Blue)
 *   - 2A & 2B     -> Stepper Coil B leads (usually Black & Green)
 */

// --- PIN DEFINITIONS ---
const int STEP_PIN = 5;
const int DIR_PIN = 6;
const int ENABLE_PIN = 7;  // LOW = outputs enabled, HIGH = outputs disabled

// --- MOTION CONFIGURATION (Based on 1/16 Microstepping Mode) ---
const long STEPS_PER_REV = 3200;         // 200 full steps * 16 microsteps = 3200 steps/revolution
const float MAX_SPEED_SPS = 1600.0;      // Target max speed in Steps Per Second (0.5 rev/sec)
const float MIN_SPEED_SPS = 100.0;       // Starting speed in Steps Per Second
const float ACCELERATION_SPS2 = 1200.0;  // Acceleration in Steps Per Second squared

// --- STATE MACHINE FOR MOTION PROFILE ---
enum ProfileState { STATE_ACCEL, STATE_CRUISE, STATE_DECEL, STATE_STANDSTILL };
ProfileState currentProfileState = STATE_STANDSTILL;

// --- STEP SCHEDULER VARIABLES ---
unsigned long lastStepMicros = 0;
unsigned long stepPeriodUs = 0;  // Delay between step pulses in microseconds

float currentSpeedSPS = 0.0;
long stepsExecuted = 0;
long totalTargetSteps = 0;
long accelStepsLimit = 0;  // Number of steps taken during acceleration phase
bool directionClockwise = true;

// --- STATE CONTROL TIMING ---
unsigned long standstillTimerStart = 0;
const unsigned long standstillDuration = 2000;  // Standstill delay (2 seconds)

// Timer for acceleration calculations
unsigned long lastRampUpdateMs = 0;
const unsigned long rampUpdateIntervalMs = 5;  // Re-calculate speed every 5ms

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 35: A4988 Bipolar Stepper Trapezoidal Controller");
  Serial.println("==================================================");

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  // Disable driver outputs initially to keep coils cold
  digitalWrite(ENABLE_PIN, HIGH);

  // Start the motion state machine by triggering a movement command
  commandMove(3200 * 2, true);  // Spin exactly 2 full rotations (6400 microsteps) Clockwise
}

void loop() {
  unsigned long currentMicros = micros();
  unsigned long currentMillis = millis();

  // --- 1. NON-BLOCKING STEP PULSE GENERATOR ---
  if (currentProfileState != STATE_STANDSTILL) {
    if (currentMicros - lastStepMicros >= stepPeriodUs) {
      lastStepMicros = currentMicros;

      // Trigger a 2 microsecond step pulse (minimum 1µs width required by A4988)
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP_PIN, LOW);

      stepsExecuted++;

      // Monitor phase transitions based on steps executed
      checkPhaseTransitions();
    }
  }

  // --- 2. SPEED ACCELERATION RAMP CALCULATOR (Runs every 5ms) ---
  if (currentProfileState != STATE_STANDSTILL) {
    if (currentMillis - lastRampUpdateMs >= rampUpdateIntervalMs) {
      float dt = (currentMillis - lastRampUpdateMs) / 1000.0;  // Convert elapsed time to seconds
      lastRampUpdateMs = currentMillis;

      calculateVelocityRamp(dt);
    }
  }

  // --- 3. STANDSTILL PAUSE HANDLER ---
  if (currentProfileState == STATE_STANDSTILL) {
    if (currentMillis - standstillTimerStart >= standstillDuration) {
      // Toggle direction and command a new 2-revolution sweep
      directionClockwise = !directionClockwise;
      commandMove(STEPS_PER_REV * 2, directionClockwise);
    }
  }
}

/**
 * Commands a new trapezoidal movement profile.
 */
void commandMove(long steps, bool clockwise) {
  totalTargetSteps = steps;
  stepsExecuted = 0;
  accelStepsLimit = 0;

  directionClockwise = clockwise;
  digitalWrite(DIR_PIN, directionClockwise ? HIGH : LOW);

  // Enable the driver outputs to energize the motor coils
  digitalWrite(ENABLE_PIN, LOW);

  // Initialize speed variables
  currentSpeedSPS = MIN_SPEED_SPS;
  stepPeriodUs = 1000000.0 / currentSpeedSPS;

  currentProfileState = STATE_ACCEL;
  lastStepMicros = micros();
  lastRampUpdateMs = millis();

  Serial.print("\n[MOTION] Commanded ");
  Serial.print(steps);
  Serial.print(" steps | Direction: ");
  Serial.println(directionClockwise ? "CW" : "CCW");
}

/**
 * Updates current velocity along the acceleration/deceleration slopes.
 */
void calculateVelocityRamp(float dt) {
  switch (currentProfileState) {
    case STATE_ACCEL:
      // Speed increases linearly: V = V_old + a * dt
      currentSpeedSPS += ACCELERATION_SPS2 * dt;
      if (currentSpeedSPS >= MAX_SPEED_SPS) {
        currentSpeedSPS = MAX_SPEED_SPS;
        currentProfileState = STATE_CRUISE;
        Serial.print("[RAMP] Cruise Speed Reached: ");
        Serial.print(currentSpeedSPS, 0);
        Serial.print(" SPS (Steps taken to accel: ");
        Serial.print(stepsExecuted);
        Serial.println(")");
      }
      break;

    case STATE_DECEL:
      // Speed decreases linearly: V = V_old - a * dt
      currentSpeedSPS -= ACCELERATION_SPS2 * dt;
      if (currentSpeedSPS < MIN_SPEED_SPS) {
        currentSpeedSPS = MIN_SPEED_SPS;
      }
      break;

    case STATE_CRUISE:
      // Speed remains flat at max velocity
      currentSpeedSPS = MAX_SPEED_SPS;
      break;

    case STATE_STANDSTILL:
      currentSpeedSPS = 0.0;
      break;
  }

  // Convert current speed to microsecond period delay
  if (currentSpeedSPS > 0) {
    stepPeriodUs = 1000000.0 / currentSpeedSPS;
  }
}

/**
 * Evaluates step count parameters to switch between profile phases.
 */
void checkPhaseTransitions() {
  // Transition 1: Check deceleration trigger point
  // We must begin decelerating when the remaining steps are equal to or less than the
  // steps it took to accelerate.
  if (currentProfileState == STATE_ACCEL || currentProfileState == STATE_CRUISE) {
    long stepsRemaining = totalTargetSteps - stepsExecuted;

    // Record how many steps it took to reach cruise/decel transition
    if (currentProfileState == STATE_ACCEL) {
      accelStepsLimit = stepsExecuted;
    }

    if (stepsRemaining <= accelStepsLimit) {
      currentProfileState = STATE_DECEL;
      Serial.println("[RAMP] Deceleration Phase Initiated.");
    }
  }

  // Transition 2: Complete movement execution
  if (stepsExecuted >= totalTargetSteps) {
    currentProfileState = STATE_STANDSTILL;
    digitalWrite(ENABLE_PIN, HIGH);  // Disable H-bridges to shut down holding current (saves power)
    standstillTimerStart = millis();
    Serial.println("[MOTION] Target reached. Coils disabled. Entering 2s standstill...");
  }
}

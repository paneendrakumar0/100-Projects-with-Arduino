/*
 * 100 Projects with Arduino - Day 34
 * Project: Stepper Motor Control (ULN2003 / 28BYJ-48 Custom Phase Driver)
 *
 * DESCRIPTION:
 * This project interfaces a 28BYJ-48 unipolar stepper motor using a ULN2003 Darlington driver
 * board. Rather than using standard library wrappers, this code writes a custom stepper driver from
 * scratch:
 * 1. Implements a non-blocking, microsecond-level step scheduler using micros() to regulate
 * rotational speed.
 * 2. Uses an 8-state Half-Step sequencing lookup table to achieve high resolution and smooth torque
 * delivery.
 * 3. Incorporates a State Machine to sweep the output shaft exactly 360° clockwise (4096 steps),
 *    pause for 2 seconds, sweep 360° counter-clockwise, and repeat.
 * 4. Includes automatic phase shutdown (de-energizing coils) when stopped to prevent motor
 * overheating.
 *
 * STEPPER MOTOR THEORY OF OPERATION:
 * - Unipolar Steppers: Have 4 coils (phases) with common center-taps connected to VCC (5-12V).
 *   Rotation is accomplished by sequentially grounding individual phases (IN1-IN4) to draw magnetic
 * flux.
 * - Darlington Transistor Array (ULN2003): The Arduino cannot sink the current (~300mA) required by
 * the coils. The ULN2003 contains seven Darlington transistor pairs that act as low-side switches.
 * Grounding an input pin on the Arduino turns on the corresponding Darlington switch, sinking coil
 * current to Ground.
 * - Gear Ratio and Steps:
 *   The 28BYJ-48 has 32 magnetic poles (steps) per internal rotor revolution.
 *   Using Half-Step sequencing doubles this resolution to 64 steps per internal revolution.
 *   The motor has an internal gear reduction ratio of 64:1 (more precisely 63.6839:1).
 *   Therefore, the number of half-steps required for one full 360° rotation of the output shaft is:
 *     64 (internal steps) * 64 (gear ratio) = 4096 steps.
 *
 * WIRING:
 * - ULN2003 Board -> Arduino Uno
 *   - IN1 -> Pin 4
 *   - IN2 -> Pin 5
 *   - IN3 -> Pin 6
 *   - IN4 -> Pin 7
 *   - GND -> Arduino GND (Shared power reference)
 * - Power Connections:
 *   - ULN2003 VCC (5-12V) -> External Positive (+) supply terminal
 *   - ULN2003 GND        -> External Negative (-) supply terminal
 *
 * WARNING: The 28BYJ-48 motor can draw over 300mA when energized. Powering it directly from the
 * Arduino's 5V pin can overload the USB port or onboard regulator. Always use an external 5V supply
 * and tie grounds together.
 */

// --- PIN DEFINITIONS ---
const int IN1_PIN = 4;  // Phase A
const int IN2_PIN = 5;  // Phase B
const int IN3_PIN = 6;  // Phase C
const int IN4_PIN = 7;  // Phase D

// --- STEPPER DRIVER SEQUENCE LOOKUP TABLE ---
// In Half-Step mode, we cycle through 8 distinct coil combinations.
// 1 = Energized (Input pin HIGH), 0 = De-energized (Input pin LOW).
const int stepCount = 8;
const byte halfStepSequence[8] = {
    0b1000,  // Step 0: Coil A
    0b1100,  // Step 1: Coil A + B
    0b0100,  // Step 2: Coil B
    0b0110,  // Step 3: Coil B + C
    0b0010,  // Step 4: Coil C
    0b0011,  // Step 5: Coil C + D
    0b0001,  // Step 6: Coil D
    0b1001   // Step 7: Coil D + A
};

// --- SYSTEM STATE MACHINE ---
enum MotionState {
  STATE_CW_SWEEP,   // Rotate 360 degrees Clockwise
  STATE_CW_PAUSE,   // Pause at end of CW rotation
  STATE_CCW_SWEEP,  // Rotate 360 degrees Counter-Clockwise
  STATE_CCW_PAUSE   // Pause at end of CCW rotation
};

MotionState currentMotionState = STATE_CW_SWEEP;

// --- STEP TIMING & COUNTER VARIABLES ---
unsigned long lastStepTime = 0;  // Timestamp of the last step trigger (microseconds)
unsigned long stepInterval =
    1200;  // Interval between steps in microseconds (sets speed. Min ~1000 for 28BYJ-48)
unsigned long pauseTimerStart = 0;         // Timestamp of pause periods (milliseconds)
const unsigned long pauseDuration = 2000;  // Pause duration between sweeps (2 seconds)

int activeSequenceStep = 0;              // Index in halfStepSequence (0 to 7)
long totalStepsExecuted = 0;             // Tracks steps taken in the current sweep direction
const long stepsPerFullRotation = 4096;  // 4096 half-steps = 360 degrees output shaft rotation

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 34: ULN2003 28BYJ-48 Stepper Custom Phase Driver");
  Serial.println("==================================================");

  // Initialize digital pins as outputs
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);

  // De-energize all coils initially
  releaseCoils();

  lastStepTime = micros();
  Serial.println("[SYSTEM] Driver active. Starting Clockwise sweep...");
}

void loop() {
  unsigned long currentTimeMs = millis();
  unsigned long currentTimeUs = micros();

  switch (currentMotionState) {
    case STATE_CW_SWEEP:
      // Execute steps in the clockwise direction
      if (currentTimeUs - lastStepTime >= stepInterval) {
        lastStepTime = currentTimeUs;

        stepCW();  // Increment step index and energize phase
        totalStepsExecuted++;

        // Print progress telemetry at intervals
        if (totalStepsExecuted % 512 == 0) {
          int degrees = (int)((float)totalStepsExecuted * 360.0 / stepsPerFullRotation);
          Serial.print("[MOTION] CW Sweep: ");
          Serial.print(degrees);
          Serial.println(" degrees completed.");
        }

        // Check if full 360 degree rotation completed
        if (totalStepsExecuted >= stepsPerFullRotation) {
          totalStepsExecuted = 0;
          releaseCoils();  // Shut down coils to save power
          pauseTimerStart = currentTimeMs;
          currentMotionState = STATE_CW_PAUSE;
          Serial.println("[STATE] CW rotation complete. Entering 2s pause (coils de-energized).");
        }
      }
      break;

    case STATE_CW_PAUSE:
      // Non-blocking wait for 2 seconds
      if (currentTimeMs - pauseTimerStart >= pauseDuration) {
        currentMotionState = STATE_CCW_SWEEP;
        lastStepTime = micros();
        Serial.println("[STATE] Pause finished. Starting Counter-Clockwise sweep...");
      }
      break;

    case STATE_CCW_SWEEP:
      // Execute steps in the counter-clockwise direction
      if (currentTimeUs - lastStepTime >= stepInterval) {
        lastStepTime = currentTimeUs;

        stepCCW();  // Decrement step index and energize phase
        totalStepsExecuted++;

        if (totalStepsExecuted % 512 == 0) {
          int degrees = (int)((float)totalStepsExecuted * 360.0 / stepsPerFullRotation);
          Serial.print("[MOTION] CCW Sweep: ");
          Serial.print(degrees);
          Serial.println(" degrees completed.");
        }

        if (totalStepsExecuted >= stepsPerFullRotation) {
          totalStepsExecuted = 0;
          releaseCoils();
          pauseTimerStart = currentTimeMs;
          currentMotionState = STATE_CCW_PAUSE;
          Serial.println("[STATE] CCW rotation complete. Entering 2s pause (coils de-energized).");
        }
      }
      break;

    case STATE_CCW_PAUSE:
      if (currentTimeMs - pauseTimerStart >= pauseDuration) {
        currentMotionState = STATE_CW_SWEEP;
        lastStepTime = micros();
        Serial.println("[STATE] Pause finished. Starting Clockwise sweep...");
      }
      break;
  }
}

// --- COIL PHASE CONTROLLER FUNCTIONS ---

/**
 * Sweeps the phase step index forward (Clockwise) and writes outputs.
 */
void stepCW() {
  activeSequenceStep++;
  if (activeSequenceStep >= stepCount) {
    activeSequenceStep = 0;
  }
  writeCoils(halfStepSequence[activeSequenceStep]);
}

/**
 * Sweeps the phase step index backward (Counter-Clockwise) and writes outputs.
 */
void stepCCW() {
  activeSequenceStep--;
  if (activeSequenceStep < 0) {
    activeSequenceStep = stepCount - 1;
  }
  writeCoils(halfStepSequence[activeSequenceStep]);
}

/**
 * Translates a sequence byte to active pin logic states.
 */
void writeCoils(byte pinMask) {
  // Extract individual bit states from the masked sequence byte
  digitalWrite(IN1_PIN, (pinMask & 0b1000) ? HIGH : LOW);
  digitalWrite(IN2_PIN, (pinMask & 0b0100) ? HIGH : LOW);
  digitalWrite(IN3_PIN, (pinMask & 0b0010) ? HIGH : LOW);
  digitalWrite(IN4_PIN, (pinMask & 0b0001) ? HIGH : LOW);
}

/**
 * Disables logic on all phase pins.
 * Prevents holding current from overheating coils when motor is static.
 */
void releaseCoils() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}

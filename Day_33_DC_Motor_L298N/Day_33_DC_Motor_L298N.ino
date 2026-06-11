/*
 * 100 Projects with Arduino - Day 33
 * Project: DC Motor Speed & Direction Control (L298N H-Bridge Driver)
 *
 * DESCRIPTION:
 * This project interfaces a brushed DC motor using the L298N dual H-Bridge driver module.
 * To achieve professional-level robotic control standards:
 * 1. Non-Blocking State Machine: Avoids delay() loops, using a millis()-based sequence to handle
 *    gradual acceleration, cruising, deceleration, and electronic braking.
 * 2. Bi-directional H-Bridge Control: Configures digital input pins (IN1, IN2) to switch H-bridge
 *    polarity for forward and reverse motion.
 * 3. Pulse Width Modulation (PWM) Speed Control: Employs analogWrite() on the Enable pin (ENA) to
 * gate average voltage delivery, regulating motor speeds smoothly.
 * 4. Verbose Telemetry Output: Prints speed percentages, current drive states, and active pin
 * settings to the Serial Monitor.
 *
 * H-BRIDGE & DC MOTOR THEORY:
 * - Brushed DC Motors: Rotate when voltage is applied across their two terminals. Reversing the
 * voltage polarity reverses the magnetic flux direction, reversing shaft rotation.
 * - H-Bridge Circuitry: Direct microcontroller pins cannot output the high current (hundreds of mA
 * to Amps) needed to drive motors. An H-Bridge uses 4 transistors (BJTs or MOSFETs) in an "H" shape
 * layout. Selectively turning diagonal pairs of transistors ON directs current through the motor in
 * either direction.
 * - Flyback Protection: DC motors are inductive loads. When current is suddenly cut off, the
 * magnetic field collapses, creating a high-voltage spike (back electromotive force, or Back-EMF).
 * The L298N module includes flyback (freewheeling) diodes that shunt these dangerous voltage spikes
 * safely to Ground, preventing microcontroller damage.
 *
 * WIRING:
 * - L298N Board -> Arduino Uno
 *   - ENA (Enable A) -> Pin 9 (Must be a PWM-capable pin!)
 *   - IN1 (Input 1)  -> Pin 7
 *   - IN2 (Input 2)  -> Pin 8
 *   - GND            -> Arduino GND (CRITICAL: Shared ground between motor battery and Arduino!)
 * - Power Connections:
 *   - L298N GND      -> Negative (-) terminal of external power source (e.g. 9V battery or DC
 * supply)
 *   - L298N 12V      -> Positive (+) terminal of external power source
 *   - L298N OUT1/OUT2-> Terminals of the brushed DC motor
 *
 * WARNING: Never power motors directly from the Arduino 5V/3.3V power pins! Motors draw stall
 * currents that can trigger thermal shutdown or permanently damage the Arduino's voltage regulator.
 */

// --- PIN DEFINITIONS ---
const int ENA_PIN = 9;  // PWM Speed Control Pin
const int IN1_PIN = 7;  // Direction Control Pin 1
const int IN2_PIN = 8;  // Direction Control Pin 2

// --- STATE DEFINITIONS ---
enum MotorState {
  STATE_FORWARD_ACCEL,
  STATE_FORWARD_CRUISE,
  STATE_FORWARD_DECEL,
  STATE_BRAKING_1,
  STATE_REVERSE_ACCEL,
  STATE_REVERSE_CRUISE,
  STATE_REVERSE_DECEL,
  STATE_BRAKING_2
};

// --- TIMING CONFIGURATION ---
MotorState currentMotorState = STATE_FORWARD_ACCEL;
unsigned long stateTimerStart = 0;
const unsigned long stateDuration = 4000;  // Duration of each state in milliseconds (4 seconds)

// --- SPEED VARIABLES ---
int motorSpeedPWM = 0;

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 33: L298N H-Bridge DC Motor State Controller");
  Serial.println("==================================================");

  // Initialize output pins
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Set motor initial state to stopped
  stopMotor();

  stateTimerStart = millis();
  Serial.println("[SYSTEM] Initialization complete. Beginning test cycle...");
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - stateTimerStart;

  // Step 1: Manage State transitions every 'stateDuration' interval
  if (elapsedTime >= stateDuration) {
    stateTimerStart = currentTime;
    elapsedTime = 0;

    // Cycle to the next state in sequence
    switch (currentMotorState) {
      case STATE_FORWARD_ACCEL:
        currentMotorState = STATE_FORWARD_CRUISE;
        break;
      case STATE_FORWARD_CRUISE:
        currentMotorState = STATE_FORWARD_DECEL;
        break;
      case STATE_FORWARD_DECEL:
        currentMotorState = STATE_BRAKING_1;
        break;
      case STATE_BRAKING_1:
        currentMotorState = STATE_REVERSE_ACCEL;
        break;
      case STATE_REVERSE_ACCEL:
        currentMotorState = STATE_REVERSE_CRUISE;
        break;
      case STATE_REVERSE_CRUISE:
        currentMotorState = STATE_REVERSE_DECEL;
        break;
      case STATE_REVERSE_DECEL:
        currentMotorState = STATE_BRAKING_2;
        break;
      case STATE_BRAKING_2:
        currentMotorState = STATE_FORWARD_ACCEL;
        break;
    }

    Serial.print("\n[STATE CHANGE] -> Transitioned to state: ");
    printStateName(currentMotorState);
    Serial.println();
  }

  // Step 2: Execute speed updates based on current state (non-blocking mapping)
  executeMotorCalculations(elapsedTime);
}

/**
 * Calculates and writes PWM values dynamically depending on active state.
 */
void executeMotorCalculations(unsigned long elapsed) {
  float progress = (float)elapsed / stateDuration;  // Normalized time progress (0.0 to 1.0)

  switch (currentMotorState) {
    case STATE_FORWARD_ACCEL:
      // Accelerating Forward: IN1=HIGH, IN2=LOW, Speed ramping 0 -> 255
      setDirectionForward();
      motorSpeedPWM = (int)(progress * 255.0);
      writeMotorSpeed(motorSpeedPWM);
      printTelemetry("ACCEL FORWARD", (motorSpeedPWM * 100) / 255);
      break;

    case STATE_FORWARD_CRUISE:
      // Cruising Forward: Speed clamped at max (255)
      setDirectionForward();
      motorSpeedPWM = 255;
      writeMotorSpeed(motorSpeedPWM);
      printTelemetry("CRUISE FORWARD", 100);
      break;

    case STATE_FORWARD_DECEL:
      // Decelerating Forward: Speed ramping 255 -> 0
      setDirectionForward();
      motorSpeedPWM = (int)((1.0 - progress) * 255.0);
      writeMotorSpeed(motorSpeedPWM);
      printTelemetry("DECEL FORWARD", (motorSpeedPWM * 100) / 255);
      break;

    case STATE_BRAKING_1:
      // Brake state: Active electronic braking (IN1=LOW, IN2=LOW, ENA=HIGH)
      brakeMotor();
      printTelemetry("BRAKE ACTIVE", 0);
      break;

    case STATE_REVERSE_ACCEL:
      // Accelerating Reverse: IN1=LOW, IN2=HIGH, Speed ramping 0 -> 255
      setDirectionReverse();
      motorSpeedPWM = (int)(progress * 255.0);
      writeMotorSpeed(motorSpeedPWM);
      printTelemetry("ACCEL REVERSE", (motorSpeedPWM * 100) / 255);
      break;

    case STATE_REVERSE_CRUISE:
      // Cruising Reverse: Speed clamped at max (255)
      setDirectionReverse();
      motorSpeedPWM = 255;
      writeMotorSpeed(motorSpeedPWM);
      printTelemetry("CRUISE REVERSE", 100);
      break;

    case STATE_REVERSE_DECEL:
      // Decelerating Reverse: Speed ramping 255 -> 0
      setDirectionReverse();
      motorSpeedPWM = (int)((1.0 - progress) * 255.0);
      writeMotorSpeed(motorSpeedPWM);
      printTelemetry("DECEL REVERSE", (motorSpeedPWM * 100) / 255);
      break;

    case STATE_BRAKING_2:
      // Brake state
      brakeMotor();
      printTelemetry("BRAKE ACTIVE", 0);
      break;
  }
}

// --- HARDWARE DIRECT CONTROL FUNCTIONS ---

/**
 * Configure direction pins for Forward rotation (IN1=HIGH, IN2=LOW).
 */
void setDirectionForward() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
}

/**
 * Configure direction pins for Reverse rotation (IN1=LOW, IN2=HIGH).
 */
void setDirectionReverse() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
}

/**
 * Halts motor rotation smoothly by disabling the Enable pin (floating state).
 */
void stopMotor() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 0);
}

/**
 * Actively locks the motor armature electronically by grounding both coils (IN1=LOW, IN2=LOW,
 * ENA=HIGH).
 */
void brakeMotor() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 255);  // Maximum braking torque
}

/**
 * Writes the raw 8-bit duty cycle value to the enable pin.
 */
void writeMotorSpeed(int speed) {
  // Constrain value between 0 and 255 to prevent overflow issues
  int clampedSpeed = constrain(speed, 0, 255);
  analogWrite(ENA_PIN, clampedSpeed);
}

// --- TELEMETRY AND DIAGNOSTIC UTILITIES ---

/**
 * Prints the active driver configurations to the console at throttled intervals.
 */
void printTelemetry(const char* action, int percent) {
  static unsigned long lastTele = 0;
  if (millis() - lastTele >= 500) {  // Limit telemetry updates to 2 Hz to prevent Serial congestion
    lastTele = millis();
    Serial.print("[DRIVE] Action: ");
    Serial.print(action);
    Serial.print(" | Target Speed: ");
    Serial.print(percent);
    Serial.print("% | Pins: IN1=");
    Serial.print(digitalRead(IN1_PIN));
    Serial.print(" IN2=");
    Serial.println(digitalRead(IN2_PIN));
  }
}

/**
 * Translates state enum variables to human-readable strings.
 */
void printStateName(MotorState state) {
  switch (state) {
    case STATE_FORWARD_ACCEL:
      Serial.print("FORWARD ACCELERATION");
      break;
    case STATE_FORWARD_CRUISE:
      Serial.print("FORWARD CRUISE");
      break;
    case STATE_FORWARD_DECEL:
      Serial.print("FORWARD DECELERATION");
      break;
    case STATE_BRAKING_1:
      Serial.print("BRAKE SEQUENCE A");
      break;
    case STATE_REVERSE_ACCEL:
      Serial.print("REVERSE ACCELERATION");
      break;
    case STATE_REVERSE_CRUISE:
      Serial.print("REVERSE CRUISE");
      break;
    case STATE_REVERSE_DECEL:
      Serial.print("REVERSE DECELERATION");
      break;
    case STATE_BRAKING_2:
      Serial.print("BRAKE SEQUENCE B");
      break;
  }
}

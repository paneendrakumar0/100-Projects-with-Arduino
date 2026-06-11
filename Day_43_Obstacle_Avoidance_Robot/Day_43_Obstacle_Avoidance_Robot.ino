/*
 * 100 Projects with Arduino - Day 43
 * Project: Autonomous Obstacle-Avoidance Robot (State Machine Navigation)
 *
 * DESCRIPTION:
 * This project implements an autonomous mobile robot platform utilizing differential drive
 * kinematics. To achieve professional mechatronic system standards:
 * 1. Non-Blocking Navigation State Machine: Manages driving, stopping, scanning, deciding, and
 * turning states using a millis() scheduler, ensuring sensor loops and motor triggers execute
 * concurrently.
 * 2. Active Obstacle Scanning: Mounts an HC-SR04 rangefinder on a micro servo. When blocked, the
 * robot stops, sweeps the servo left and right to record clearances, and selects the direction with
 * more space.
 * 3. Differential Drive Steering: Controls two DC motors via an L298N H-bridge (speed PWM,
 * direction pins).
 * 4. High-Integrity Safe Mode: Automatically falls back to a 180° spin escape sequence if both left
 * and right paths are blocked.
 *
 * ROBOT NAVIGATION PHYSICS:
 * - Differential Drive Kinematics: Steering is accomplished by varying the relative velocity of the
 * left and right wheels.
 *   - Forward: Both motors spin forward.
 *   - Turn Left: Left motor spins backward, Right motor spins forward (zero-radius spin-in-place).
 *   - Stop: Both motors de-energized or actively braked.
 * - Sonar Spatial Resolution: Acoustic waves expand in a 15° cone. When scanning left and right,
 * the servo must hold position for 300ms to allow the acoustic pulses to complete transit and the
 * servo to settle before triggering range pings.
 *
 * WIRING:
 * - L298N Motor Driver -> Arduino Uno
 *   - ENA (Left Speed PWM)  -> Pin 5
 *   - IN1 (Left Dir 1)      -> Pin 4
 *   - IN2 (Left Dir 2)      -> Pin 3
 *   - ENB (Right Speed PWM) -> Pin 6
 *   - IN3 (Right Dir 1)     -> Pin 7
 *   - IN4 (Right Dir 2)     -> Pin 8
 *   - GND                   -> Arduino GND (Common Ground!)
 * - SG90 Sensor Servo -> Arduino Uno
 *   - Signal                -> Pin 9
 * - HC-SR04 Sensor -> Arduino Uno
 *   - TRIG                  -> Pin 11
 *   - ECHO                  -> Pin 12
 * - Power:
 *   - L298N 12V Screw       -> Battery Positive (+)
 *   - L298N GND Screw       -> Battery Negative (-) and Arduino GND
 */

#include <Servo.h>

// --- PIN DEFINITIONS ---
const int L_ENA_PIN = 5;  // Left motor PWM speed
const int L_IN1_PIN = 4;  // Left motor direction pin 1
const int L_IN2_PIN = 3;  // Left motor direction pin 2

const int R_ENB_PIN = 6;  // Right motor PWM speed
const int R_IN3_PIN = 7;  // Right motor direction pin 1
const int R_IN4_PIN = 8;  // Right motor direction pin 2

const int SERVO_PIN = 9;  // Sensor sweep servo
const int TRIG_PIN = 11;  // Sonar Trigger
const int ECHO_PIN = 12;  // Sonar Echo

// --- SYSTEM CONSTANTS ---
const float CRITICAL_DIST_CM = 25.0;  // Distance threshold to trigger obstacle avoidance
const int MOTOR_BASE_SPEED = 180;     // Standard PWM cruise speed (0-255)
const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 30;
const int SERVO_LEFT = 150;

// --- NAVIGATION STATE MACHINE ---
enum DriveState {
  STATE_DRIVE_FORWARD,  // Cruising forward, polling distance
  STATE_STOP_ROBOT,     // Halt wheels, prepare to scan
  STATE_SCAN_RIGHT,     // Rotate servo right and ping
  STATE_SCAN_LEFT,      // Rotate servo left and ping
  STATE_EVALUATE_PATH,  // Choose path of max clearance
  STATE_EXECUTE_TURN,   // Spin wheels in place
  STATE_ESCAPE_SPIN     // Both sides blocked, execute 180 spin
};

DriveState currentDriveState = STATE_DRIVE_FORWARD;

// --- TIMING AND CALIBRATION VARIABLES ---
unsigned long stateTimerStart = 0;
unsigned long scanTimerStart = 0;
const unsigned long servoSettleTimeMs = 350;  // Delay for servo physical transit
unsigned long turnDurationMs = 0;             // Configured dynamically based on angle

float leftDistance = 0.0;
float rightDistance = 0.0;

// Instantiate servo object
Servo sensorServo;

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 43: Autonomous Obstacle-Avoidance Mobile Robot");
  Serial.println("==================================================");

  // Initialize motor pins
  pinMode(L_ENA_PIN, OUTPUT);
  pinMode(L_IN1_PIN, OUTPUT);
  pinMode(L_IN2_PIN, OUTPUT);
  pinMode(R_ENB_PIN, OUTPUT);
  pinMode(R_IN3_PIN, OUTPUT);
  pinMode(R_IN4_PIN, OUTPUT);

  // Initialize rangefinder pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Attach servo and center sensor
  sensorServo.attach(SERVO_PIN);
  sensorServo.write(SERVO_CENTER);

  haltMotors();
  delay(500);  // Settle boot states

  Serial.println("[SYSTEM] Calibration complete. Autonomous navigation active.");
}

void loop() {
  unsigned long currentMillis = millis();

  switch (currentDriveState) {
    case STATE_DRIVE_FORWARD: {
      // Cruising Forward
      driveForward(MOTOR_BASE_SPEED);

      // Monitor forward distance
      float frontDist = getPingDistance();
      if (frontDist > 0.0 && frontDist < CRITICAL_DIST_CM) {
        Serial.print("[OBSTACLE] Blockage detected at: ");
        Serial.print(frontDist, 1);
        Serial.println(" cm. Halting robot.");

        haltMotors();
        currentDriveState = STATE_STOP_ROBOT;
        stateTimerStart = currentMillis;
      }
      break;
    }

    case STATE_STOP_ROBOT:
      // Allow wheels to stop sliding before scanning
      if (currentMillis - stateTimerStart >= 300) {
        Serial.println("[SCAN] Initiating spatial sweep...");
        sensorServo.write(SERVO_RIGHT);
        currentDriveState = STATE_SCAN_RIGHT;
        scanTimerStart = currentMillis;
      }
      break;

    case STATE_SCAN_RIGHT:
      // Wait for servo to reach right position, then take reading
      if (currentMillis - scanTimerStart >= servoSettleTimeMs) {
        rightDistance = getPingDistance();
        Serial.print("[SCAN] Right clearance: ");
        Serial.print(rightDistance, 1);
        Serial.println(" cm");

        sensorServo.write(SERVO_LEFT);
        currentDriveState = STATE_SCAN_LEFT;
        scanTimerStart = currentMillis;
      }
      break;

    case STATE_SCAN_LEFT:
      // Wait for servo to reach left position, then take reading
      if (currentMillis - scanTimerStart >= servoSettleTimeMs) {
        leftDistance = getPingDistance();
        Serial.print("[SCAN] Left clearance: ");
        Serial.print(leftDistance, 1);
        Serial.println(" cm");

        sensorServo.write(SERVO_CENTER);  // Recenter sensor
        currentDriveState = STATE_EVALUATE_PATH;
        stateTimerStart = currentMillis;
      }
      break;

    case STATE_EVALUATE_PATH:
      // Wait for servo to recenter before moving
      if (currentMillis - stateTimerStart >= servoSettleTimeMs) {
        // Router decisions
        if (leftDistance < CRITICAL_DIST_CM && rightDistance < CRITICAL_DIST_CM) {
          Serial.println("[ROUTE] Dead End. Both paths blocked. Initiating escape spin.");
          currentDriveState = STATE_ESCAPE_SPIN;
          stateTimerStart = currentMillis;
          turnDurationMs = 1200;  // Time needed to rotate 180 degrees in place
          spinLeft(MOTOR_BASE_SPEED);
        } else if (leftDistance >= rightDistance) {
          Serial.println("[ROUTE] Steering LEFT (Max clearance path).");
          currentDriveState = STATE_EXECUTE_TURN;
          stateTimerStart = currentMillis;
          turnDurationMs = 600;  // Time needed to rotate ~90 degrees
          spinLeft(MOTOR_BASE_SPEED);
        } else {
          Serial.println("[ROUTE] Steering RIGHT (Max clearance path).");
          currentDriveState = STATE_EXECUTE_TURN;
          stateTimerStart = currentMillis;
          turnDurationMs = 600;
          spinRight(MOTOR_BASE_SPEED);
        }
      }
      break;

    case STATE_EXECUTE_TURN:
    case STATE_ESCAPE_SPIN:
      // Monitor turn timer non-blockingly
      if (currentMillis - stateTimerStart >= turnDurationMs) {
        haltMotors();
        Serial.println("[STATE] Turn execution finished. Resume cruising.");
        currentDriveState = STATE_DRIVE_FORWARD;
      }
      break;
  }
}

// --- ULTRASONIC ACQUISITION GATING ---

float getPingDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Gated echo search (20ms timeout limit)
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 20000);
  if (duration == 0) {
    return 400.0;  // Return max range if no echo (path clear)
  }
  return (float)duration * 0.0343 / 2.0;
}

// --- DIFFERENTIAL DRIVE ACTUATION CONTROL ---

void driveForward(int speed) {
  // Left Motor Forward: IN1=HIGH, IN2=LOW
  digitalWrite(L_IN1_PIN, HIGH);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, speed);

  // Right Motor Forward: IN3=HIGH, IN4=LOW
  digitalWrite(R_IN3_PIN, HIGH);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, speed);
}

void spinLeft(int speed) {
  // Spin Left in-place: Left wheels backward, Right wheels forward
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, HIGH);
  analogWrite(L_ENA_PIN, speed);

  digitalWrite(R_IN3_PIN, HIGH);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, speed);
}

void spinRight(int speed) {
  // Spin Right in-place: Left wheels forward, Right wheels backward
  digitalWrite(L_IN1_PIN, HIGH);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, speed);

  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, HIGH);
  analogWrite(R_ENB_PIN, speed);
}

void haltMotors() {
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, 0);

  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, 0);
}

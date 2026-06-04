/*
 * 100 Projects with Arduino - Day 44
 * Project: Basic Line-Following Robot (2-Sensor Differential Control)
 * 
 * DESCRIPTION:
 * This project implements a basic autonomous line-following robot using a two-sensor TCRT5000 infrared array.
 * To satisfy mechatronics design standards:
 * 1. Analog Hysteresis Gating: Reads raw analog sensor values and applies software threshold filtering,
 *    eliminating the erratic jitter associated with hardware comparator adjustments.
 * 2. Differential Drive Steer Logic: Controls two DC motors via the L298N H-bridge. Steering adjustments
 *    are achieved by slowing/stopping one wheel while running the other (differential steering).
 * 3. Diagnostic Console Telemetry: Prints raw IR sensor values and the resolved navigation state
 *    (Straight, Left Turn, Right Turn, Stop) at throttled intervals for easy debugging.
 * 
 * IR SENSING & STEERING PHYSICS:
 * - Infrared Reflection: TCRT5000 sensors contain an IR emitting LED and a phototransistor receiver.
 *   - White surfaces reflect high amounts of IR light, turning the phototransistor ON (outputting LOW analog values).
 *   - Black lines absorb IR light, turning the phototransistor OFF (outputting HIGH analog values).
 * - Steering State Table:
 *   Let Threshold = 500. Output logic:
 *     - Left Sensor < 500 (White) AND Right Sensor < 500 (White) -> Line is centered. Drive Straight.
 *     - Left Sensor > 500 (Black) AND Right Sensor < 500 (White) -> Robot drifted right. Turn Left.
 *     - Left Sensor < 500 (White) AND Right Sensor > 500 (Black) -> Robot drifted left. Turn Right.
 *     - Left Sensor > 500 (Black) AND Right Sensor > 500 (Black) -> T-junction or Stop Line. Stop Robot.
 * 
 * WIRING:
 * - TCRT5000 IR Sensor Array -> Arduino Uno
 *   - Left Sensor AOUT  -> Pin A1
 *   - Right Sensor AOUT -> Pin A2
 *   - VCC               -> 5V
 *   - GND               -> GND
 * - L298N Motor Driver -> Arduino Uno
 *   - ENA (Left Speed)  -> Pin 5
 *   - IN1 (Left Dir 1)  -> Pin 4
 *   - IN2 (Left Dir 2)  -> Pin 3
 *   - ENB (Right Speed) -> Pin 6
 *   - IN3 (Right Dir 1) -> Pin 7
 *   - IN4 (Right Dir 2) -> Pin 8
 *   - GND               -> GND (Common Ground!)
 */

// --- PIN DEFINITIONS ---
const int LEFT_SENSOR_PIN = A1;  // Analog input for Left IR sensor
const int RIGHT_SENSOR_PIN = A2; // Analog input for Right IR sensor

const int L_ENA_PIN = 5; // Left motor speed
const int L_IN1_PIN = 4; // Left motor direction 1
const int L_IN2_PIN = 3; // Left motor direction 2

const int R_ENB_PIN = 6; // Right motor speed
const int R_IN3_PIN = 7; // Right motor direction 1
const int R_IN4_PIN = 8; // Right motor direction 2

// --- CONTROL PARAMETERS ---
const int LINE_THRESHOLD = 500;  // Threshold value (0-1023). Greater = Black, Lesser = White
const int CRUISE_SPEED = 150;    // Motor speed PWM duty cycle (0-255)
const int TURN_SPEED = 120;      // Motor speed PWM during turns (differential pivot)

// --- TIMING VARIABLES ---
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryIntervalMs = 250; // Throttle debug prints to 4 Hz

void setup() {
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 44: Basic 2-Sensor Line-Following Robot");
  Serial.println("==================================================");

  // Initialize motor control pins
  pinMode(L_ENA_PIN, OUTPUT);
  pinMode(L_IN1_PIN, OUTPUT);
  pinMode(L_IN2_PIN, OUTPUT);
  pinMode(R_ENB_PIN, OUTPUT);
  pinMode(R_IN3_PIN, OUTPUT);
  pinMode(R_IN4_PIN, OUTPUT);

  // Stop motors initially
  haltRobot();
  
  Serial.println("[SYSTEM] Robot initialized. Place on line to start.");
}

void loop() {
  // Read raw analog values from the IR sensors (0 to 1023)
  int rawLeft = analogRead(LEFT_SENSOR_PIN);
  int rawRight = analogRead(RIGHT_SENSOR_PIN);

  // Apply threshold evaluation
  bool leftOnLine  = (rawLeft > LINE_THRESHOLD);
  bool rightOnLine = (rawRight > LINE_THRESHOLD);

  // --- DIFFERENTIAL DRIVE ROUTING MACHINE ---
  String navigationState = "";

  if (!leftOnLine && !rightOnLine) {
    // Case 1: Both sensors see white floor. Center is on line. Move straight forward.
    navigationState = "STRAIGHT";
    driveStraight(CRUISE_SPEED);
  } 
  else if (leftOnLine && !rightOnLine) {
    // Case 2: Left sensor hit the black line. Robot drifted right. Pivot Left.
    navigationState = "TURN_LEFT";
    steerLeft(TURN_SPEED);
  } 
  else if (!leftOnLine && rightOnLine) {
    // Case 3: Right sensor hit the black line. Robot drifted left. Pivot Right.
    navigationState = "TURN_RIGHT";
    steerRight(TURN_SPEED);
  } 
  else if (leftOnLine && rightOnLine) {
    // Case 4: Both sensors detect black. Stop robot (T-junction or end of line).
    navigationState = "STOP_LINE";
    haltRobot();
  }

  // --- THROTTLED DIAGNOSTICS ---
  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryTime >= telemetryIntervalMs) {
    lastTelemetryTime = currentMillis;

    Serial.print("[TRACK] Raw Left: ");
    Serial.print(rawLeft);
    Serial.print(" (");
    Serial.print(leftOnLine ? "BLACK" : "WHITE");
    Serial.print(") | Raw Right: ");
    Serial.print(rawRight);
    Serial.print(" (");
    Serial.print(rightOnLine ? "BLACK" : "WHITE");
    Serial.print(") | State: ");
    Serial.println(navigationState);
  }
}

// --- ROBOT CHASSIS DIFFERENTIAL STEERING FUNCTIONS ---

/**
 * Commands both wheels forward at equal speeds.
 */
void driveStraight(int speed) {
  // Left Motor Forward: IN1=HIGH, IN2=LOW
  digitalWrite(L_IN1_PIN, HIGH);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, speed);

  // Right Motor Forward: IN3=HIGH, IN4=LOW
  digitalWrite(R_IN3_PIN, HIGH);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, speed);
}

/**
 * Pivots left by locking/stopping the left wheel and running the right wheel forward.
 */
void steerLeft(int speed) {
  // Stop Left Wheel
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, 0);

  // Drive Right Wheel Forward
  digitalWrite(R_IN3_PIN, HIGH);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, speed);
}

/**
 * Pivots right by running the left wheel forward and locking/stopping the right wheel.
 */
void steerRight(int speed) {
  // Drive Left Wheel Forward
  digitalWrite(L_IN1_PIN, HIGH);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, speed);

  // Stop Right Wheel
  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, 0);
}

/**
 * Cuts power to both motors to halt movement.
 */
void haltRobot() {
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, 0);

  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, 0);
}

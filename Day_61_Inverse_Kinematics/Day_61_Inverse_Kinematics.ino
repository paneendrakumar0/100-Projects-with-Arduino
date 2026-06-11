/*
 * 100 Projects with Arduino - Day 61
 * Project: Robotic Arm 2-DOF Inverse Kinematics (Geometric IK Solver)
 *
 * DESCRIPTION:
 * This project implements a 2-Degrees-of-Freedom (2-DOF) robotic arm controlled
 * by a joystick in Cartesian space. Instead of directly mapping joystick position
 * to servo angles (forward kinematics), we use INVERSE KINEMATICS (IK) to compute
 * the required joint angles (theta1, theta2) for any target (x, y) coordinate.
 *
 * ARM GEOMETRY:
 *   - Link 1 (upper arm): length L1 (mm)
 *   - Link 2 (forearm):   length L2 (mm)
 *   - Base: shoulder servo
 *   - Elbow: elbow servo
 *   - End effector: gripper tip (goal position)
 *
 * GEOMETRIC IK SOLUTION:
 *  1. Distance from shoulder to target:
 *     d = sqrt(x^2 + y^2)
 *
 *  2. Elbow angle (Law of Cosines):
 *     cos(theta2) = (d^2 - L1^2 - L2^2) / (2 * L1 * L2)
 *     theta2 = acos(cos_theta2)         [0 degrees = fully extended]
 *
 *  3. Shoulder angle:
 *     k1 = L1 + L2 * cos(theta2)
 *     k2 = L2 * sin(theta2)
 *     theta1 = atan2(y, x) - atan2(k2, k1)
 *
 *  4. "Elbow-up" vs "elbow-down" configuration:
 *     Two valid solutions exist for any reachable point.
 *     We use "elbow-up" (theta2 positive = arm bends upward).
 *
 * WORKSPACE REACHABILITY:
 *   The arm can only reach points where |L1 - L2| <= d <= L1 + L2.
 *   Points outside this annular region are singularities; we clamp d to valid range.
 *
 * WIRING:
 *   Joystick VRx -> A0
 *   Joystick VRy -> A1
 *   Joystick SW  -> D2 (optional button for elbow-up/elbow-down toggle)
 *   Shoulder Servo signal -> D9
 *   Elbow Servo signal    -> D10
 */

#include <Servo.h>

// --- ARM LINK LENGTHS (millimeters) ---
const float L1 = 100.0f;  // Upper arm length (mm)
const float L2 = 80.0f;   // Forearm length (mm)

// --- SERVO OBJECTS ---
Servo shoulderServo;
Servo elbowServo;

// --- SERVO PINS ---
const int SHOULDER_PIN = 9;
const int ELBOW_PIN = 10;

// --- JOYSTICK PINS ---
const int JOY_X_PIN = A0;
const int JOY_Y_PIN = A1;
const int JOY_SW_PIN = 2;

// --- WORKSPACE LIMITS (mm) ---
// X: forward/back from shoulder (0 to L1+L2)
// Y: height above/below shoulder (-L1 to L1+L2)
const float X_MIN = -(L1 + L2);
const float X_MAX = (L1 + L2);
const float Y_MIN = -(L1 + L2);
const float Y_MAX = (L1 + L2);

// --- SERVO ANGLE LIMITS (degrees) ---
const int SERVO_MIN_DEG = 0;
const int SERVO_MAX_DEG = 180;

// --- ELBOW CONFIGURATION ---
bool elbowUp = true;  // true = elbow-up solution

// --- INVERSE KINEMATICS ---
struct IKResult {
  float theta1_deg;  // Shoulder angle (degrees)
  float theta2_deg;  // Elbow angle (degrees)
  bool reachable;    // Whether target is within workspace
};

IKResult solveIK(float x, float y) {
  IKResult result;

  float d_sq = x * x + y * y;
  float d = sqrt(d_sq);

  // Clamp d to reachable workspace annulus
  float d_min = fabsf(L1 - L2) + 0.01f;  // Avoid singularity at full stretch / full fold
  float d_max = L1 + L2 - 0.01f;
  result.reachable = (d >= d_min && d <= d_max);
  if (d < d_min) d = d_min;
  if (d > d_max) d = d_max;

  // Step 1: Law of cosines for elbow angle
  float cos_theta2 = (d_sq - L1 * L1 - L2 * L2) / (2.0f * L1 * L2);
  // Clamp to [-1, 1] to avoid NaN in acos due to floating point errors
  cos_theta2 = constrain(cos_theta2, -1.0f, 1.0f);
  float theta2_rad = acos(cos_theta2);  // [0, pi]

  // Elbow-up = positive theta2, Elbow-down = negative theta2
  if (!elbowUp) theta2_rad = -theta2_rad;

  // Step 2: Shoulder angle
  float k1 = L1 + L2 * cos(theta2_rad);
  float k2 = L2 * sin(theta2_rad);
  float theta1_rad = atan2(y, x) - atan2(k2, k1);

  // Convert radians to degrees
  result.theta1_deg = degrees(theta1_rad);
  result.theta2_deg = degrees(theta2_rad);

  // Clamp to servo limits
  result.theta1_deg = constrain(result.theta1_deg, SERVO_MIN_DEG, SERVO_MAX_DEG);
  result.theta2_deg = constrain(result.theta2_deg, SERVO_MIN_DEG, SERVO_MAX_DEG);

  return result;
}

void setup() {
  Serial.begin(9600);

  shoulderServo.attach(SHOULDER_PIN);
  elbowServo.attach(ELBOW_PIN);

  // Move to neutral position (arm pointing straight up)
  shoulderServo.write(90);
  elbowServo.write(90);
  delay(500);

  pinMode(JOY_SW_PIN, INPUT_PULLUP);

  Serial.println(F("[IK] 2-DOF Robotic Arm Inverse Kinematics active."));
  Serial.print(F("[IK] L1 = "));
  Serial.print(L1);
  Serial.print(F(" mm, L2 = "));
  Serial.println(L2);
  Serial.print(F("[IK] Workspace radius: "));
  Serial.print(fabsf(L1 - L2));
  Serial.print(F(" to "));
  Serial.print(L1 + L2);
  Serial.println(F(" mm"));
}

void loop() {
  // Read joystick and map to target coordinate space
  int rawX = analogRead(JOY_X_PIN);
  int rawY = analogRead(JOY_Y_PIN);

  // Map joystick to workspace coordinates
  float targetX = map(rawX, 0, 1023, (int)X_MIN, (int)X_MAX);
  float targetY = map(rawY, 0, 1023, (int)Y_MIN, (int)Y_MAX);

  // Toggle elbow-up/elbow-down on button press
  static bool lastBtn = HIGH;
  bool curBtn = digitalRead(JOY_SW_PIN);
  if (curBtn == LOW && lastBtn == HIGH) {
    elbowUp = !elbowUp;
    Serial.print(F("[IK] Elbow config: "));
    Serial.println(elbowUp ? F("UP") : F("DOWN"));
    delay(50);  // Debounce
  }
  lastBtn = curBtn;

  // Solve IK
  IKResult ik = solveIK(targetX, targetY);

  // Write servo angles
  shoulderServo.write((int)ik.theta1_deg);
  elbowServo.write((int)ik.theta2_deg);

  // Serial telemetry (throttled)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.print(F("[IK] Target=("));
    Serial.print(targetX, 0);
    Serial.print(F(","));
    Serial.print(targetY, 0);
    Serial.print(F(") | Theta1="));
    Serial.print(ik.theta1_deg, 1);
    Serial.print(F("° Theta2="));
    Serial.print(ik.theta2_deg, 1);
    Serial.print(F("°"));
    if (!ik.reachable) Serial.print(F(" [CLAMPED - Out of workspace]"));
    Serial.println();
  }

  delay(20);  // 50 Hz update rate
}

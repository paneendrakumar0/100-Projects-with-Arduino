/*
 * 100 Projects with Arduino - Day 95
 * Project: 3-DOF Robotic Arm Inverse Kinematics (Geometric IK Solver)
 * 
 * DESCRIPTION:
 * This project implements a real-time Inverse Kinematics (IK) solver for a 3-Degree-of-Freedom
 * (3-DOF) articulated robotic arm. 
 * 
 * THE ROBOTICS PROBLEM:
 * - Forward Kinematics (FK): Given joint angles (Base, Shoulder, Elbow), find where the hand 
 *   (end-effector) is in 3D space (x, y, z). (Mathematically simple: basic trigonometry).
 * - Inverse Kinematics (IK): Given a target coordinate (x, y, z) where we want the robot to reach,
 *   calculate the required joint angles (theta1, theta2, theta3). (Mathematically complex:
 *   transcendental trigonometric equations with multiple solutions or singularities).
 * 
 * THE MATHEMATICAL SOLVER (GEOMETRIC METHOD):
 * 1. Base Yaw (theta1): Rotates in the horizontal XY plane.
 *      theta1 = atan2(y, x)
 * 2. Planar Projection: Projects the 3D target coordinates (x, y, z) into a 2D vertical plane
 *      with radial distance r = sqrt(x^2 + y^2) and height z' = z - H_base.
 * 3. Law of Cosines: Solves the elbow (theta3) and shoulder (theta2) angles inside the triangle 
 *    formed by link 1 (L1), link 2 (L2), and the diagonal distance S = sqrt(r^2 + z'^2).
 * 
 * INTERACTIVE CLI & PATH SIMULATOR:
 * The Serial Monitor CLI allows entering coordinate targets (x, y, z) directly. 
 * The program also features an automated "Circle Sweep" simulation mode that traces a 3D circle, 
 * calculating joint angles continuously at 50 Hz, designed for plotting on the Serial Plotter.
 * 
 * WIRING:
 * - Servo 1 (Base Yaw)      -> Digital Pin 9
 * - Servo 2 (Shoulder Pitch) -> Digital Pin 10
 * - Servo 3 (Elbow Pitch)    -> Digital Pin 11
 * - Servos require external 5V/6V power supply (do not power directly from Arduino 5V pin!).
 */

#include <Servo.h>

// --- ROBOT GEOMETRY (Dimensions in cm) ---
const float L1 = 15.0f;     // Link 1 (Shoulder to Elbow length)
const float L2 = 12.0f;     // Link 2 (Elbow to Wrist/End-Effector length)
const float H_BASE = 8.0f;  // Height of base rotation joint from ground

// --- PIN DEFINITIONS ---
const int SERVO_BASE_PIN  = 9;
const int SERVO_SHLDR_PIN = 10;
const int SERVO_ELBOW_PIN = 11;

// --- SERVO INSTANCES ---
Servo servoBase;
Servo servoShoulder;
Servo servoElbow;

// --- ROBOT STATE ---
float targetX = 12.0f;
float targetY = 12.0f;
float targetZ = 10.0f;

float jointTheta1 = 90.0f; // Base Angle (Degrees)
float jointTheta2 = 90.0f; // Shoulder Angle (Degrees)
float jointTheta3 = 90.0f; // Elbow Angle (Degrees)

bool simulationActive = false;

void setup() {
  Serial.begin(9600);

  // Attach Servos
  servoBase.attach(SERVO_BASE_PIN);
  servoShoulder.attach(SERVO_SHLDR_PIN);
  servoElbow.attach(SERVO_ELBOW_PIN);

  // Home Servos
  writeServos(90, 90, 90);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 95: 3-DOF Robotic Arm Inverse Kinematics"));
  Serial.println(F("=================================================="));
  
  // Test solve for default position
  if (solveInverseKinematics(targetX, targetY, targetZ, jointTheta1, jointTheta2, jointTheta3)) {
    writeServos(jointTheta1, jointTheta2, jointTheta3);
  }
  
  printMenu();
}

void loop() {
  // 1. Run Automated Path Simulation (Traces a Circle in 3D space)
  if (simulationActive) {
    runCircleTraceSimulation();
  }

  // 2. Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'x': targetX = Serial.parseFloat(); break;
      case 'y': targetY = Serial.parseFloat(); break;
      case 'z': targetZ = Serial.parseFloat(); break;
      case 's':
      case 'S':
        // Run solver manually and print
        Serial.print(F("[INPUT] Target Coordinate set to: X=")); Serial.print(targetX);
        Serial.print(F(", Y=")); Serial.print(targetY);
        Serial.print(F(", Z=")); Serial.println(targetZ);
        
        if (solveInverseKinematics(targetX, targetY, targetZ, jointTheta1, jointTheta2, jointTheta3)) {
          writeServos(jointTheta1, jointTheta2, jointTheta3);
          printJointAngles();
        }
        break;
      case 't':
      case 'T':
        simulationActive = !simulationActive;
        Serial.print(F("[SYSTEM] 3D Circle Trace Path Simulation: "));
        Serial.println(simulationActive ? F("STARTED") : F("STOPPED"));
        break;
      case 'h':
      case 'H':
        printMenu();
        break;
      default:
        break;
    }
  }
  
  delay(20); // 50 Hz loop frequency
}

// =============================================================
//  INVERSE KINEMATICS MATHEMATICAL SOLVER
// =============================================================

/**
 * Calculates joint angles (degrees) for target coordinate (x,y,z).
 * Returns true if the position is reachable (valid solution exists).
 */
bool solveInverseKinematics(float x, float y, float z, float& t1, float& t2, float& t3) {
  // 1. Solve Base Rotation Angle (Theta 1)
  // Base rotates on the XY plane.
  float theta1_rad = atan2(y, x);

  // 2. Project 3D coordinates onto 2D plane (r, z')
  float r = sqrt(x * x + y * y); // Radial distance in XY plane
  float z_prime = z - H_BASE;    // Vertical offset relative to shoulder joint

  // 3. Check for workspace reachability
  // Calculate direct diagonal distance from shoulder to end-effector
  float S = sqrt(r * r + z_prime * z_prime);
  
  if (S > (L1 + L2)) {
    Serial.println(F("[ERROR] Target out of range! (Exceeds physical reach)"));
    return false;
  }
  if (S < abs(L1 - L2)) {
    Serial.println(F("[ERROR] Target too close! (Workspace singularity)"));
    return false;
  }

  // 4. Solve Elbow Angle (Theta 3) using Law of Cosines
  // cos(beta) = (L1^2 + L2^2 - S^2) / (2 * L1 * L2)
  // Where beta is the interior angle between L1 and L2.
  float cosBeta = (L1 * L1 + L2 * L2 - S * S) / (2.0f * L1 * L2);
  cosBeta = constrain(cosBeta, -1.0f, 1.0f); // Guard against floating point inaccuracies
  float beta = acos(cosBeta);

  // Joint angle theta3 is the relative angle between L1 extension and L2.
  // We solve for elbow-up configuration: theta3 = PI - beta.
  float theta3_rad = PI - beta;

  // 5. Solve Shoulder Angle (Theta 2)
  // theta2 = phi1 - phi2
  // phi1 = angle of target diagonal line from ground horizontal = atan2(z_prime, r)
  // phi2 = interior angle between L1 and target diagonal S
  // cos(phi2) = (L1^2 + S^2 - L2^2) / (2 * L1 * S)
  float phi1 = atan2(z_prime, r);
  float cosPhi2 = (L1 * L1 + S * S - L2 * L2) / (2.0f * L1 * S);
  cosPhi2 = constrain(cosPhi2, -1.0f, 1.0f);
  float phi2 = acos(cosPhi2);
  
  float theta2_rad = phi1 + phi2; // Elbow-up configuration adds these angles

  // 6. Convert Radians to Degrees
  t1 = theta1_rad * 180.0f / PI;
  t2 = theta2_rad * 180.0f / PI;
  t3 = theta3_rad * 180.0f / PI;

  // Convert to physical servo mounting reference offsets
  // (Base center = 90 deg, Shoulder upright = 90 deg, Elbow straight = 90 deg)
  t1 = 90.0f + t1;  // Assuming base rotates +/- 90 degrees around center
  t2 = 90.0f - t2;  // Upward pitch from horizontal
  t3 = 180.0f - t3; // Angle relative to L1

  // Final range guards (Servo physical limits: 0 to 180 degrees)
  if (t1 < 0.0f || t1 > 180.0f || t2 < 0.0f || t2 > 180.0f || t3 < 0.0f || t3 > 180.0f) {
    Serial.println(F("[ERROR] Target reachable, but requires joint angles outside servo limits (0-180)!"));
    return false;
  }

  return true;
}

void writeServos(float t1, float t2, float t3) {
  servoBase.write((int)t1);
  servoShoulder.write((int)t2);
  servoElbow.write((int)t3);
}

// =============================================================
//  PATH GENERATOR & UTILITIES
// =============================================================

/**
 * Traces a 3D circle in space.
 * Radius = 4cm, centered at X=12cm, Y=12cm, Z=10cm.
 */
void runCircleTraceSimulation() {
  static float angle = 0.0f;
  const float radius = 4.0f;
  
  // Center coordinates
  const float cx = 12.0f;
  const float cy = 12.0f;
  const float cz = 10.0f;
  
  // Parametric circle equations
  float targetX_sim = cx + radius * cos(angle);
  float targetY_sim = cy + radius * sin(angle);
  float targetZ_sim = cz + radius * sin(angle * 2.0f) * 0.5f; // Add a slight figure-8 height wave
  
  angle += 0.05f;
  if (angle >= 2.0f * PI) angle -= 2.0f * PI;

  float t1, t2, t3;
  if (solveInverseKinematics(targetX_sim, targetY_sim, targetZ_sim, t1, t2, t3)) {
    writeServos(t1, t2, t3);
    
    // Output values formatted for the Serial Plotter
    Serial.print(F("TargetX:"));   Serial.print(targetX_sim);
    Serial.print(F(",TargetY:"));   Serial.print(targetY_sim);
    Serial.print(F(",TargetZ:"));   Serial.print(targetZ_sim);
    Serial.print(F(",BaseAngle:"));  Serial.print(t1);
    Serial.print(F(",Shoulder:"));   Serial.print(t2);
    Serial.print(F(",Elbow:"));      Serial.println(t3);
  }
}

void printJointAngles() {
  Serial.println(F("----------------- SOLVED JOINT ANGLES -----------------"));
  Serial.print(F(" Base (Servo D9)     : ")); Serial.print(jointTheta1, 1); Serial.println(F(" degrees"));
  Serial.print(F(" Shoulder (Servo D10): ")); Serial.print(jointTheta2, 1); Serial.println(F(" degrees"));
  Serial.print(F(" Elbow (Servo D11)   : ")); Serial.print(jointTheta3, 1); Serial.println(F(" degrees"));
  Serial.println(F("-------------------------------------------------------"));
}

void printMenu() {
  Serial.println(F("\n--- ROBOTIC ARM IK CLI ---"));
  Serial.println(F(" Configure target coordinates and solve:"));
  Serial.println(F(" 'x <val>' : Set target X coordinate (e.g. 'x 12')"));
  Serial.println(F(" 'y <val>' : Set target Y coordinate (e.g. 'y 12')"));
  Serial.println(F(" 'z <val>' : Set target Z coordinate (e.g. 'z 10')"));
  Serial.println(F(" 's'       : Solve & Move to coordinates"));
  Serial.println(F(" 't'       : Toggle automatic 3D Circle path trace simulation (for Plotter)"));
  Serial.println(F(" 'h'       : Print this help command reference"));
  Serial.println(F("--------------------------\n"));
}

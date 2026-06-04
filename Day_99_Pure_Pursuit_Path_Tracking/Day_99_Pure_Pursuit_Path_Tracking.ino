/*
 * 100 Projects with Arduino - Day 99
 * Project: Autonomous Path Tracking using the Pure Pursuit Algorithm
 * 
 * DESCRIPTION:
 * This project implements the Pure Pursuit path-tracking algorithm, which is the industry
 * standard for steering control in autonomous ground vehicles (AGVs) and mobile robots.
 * 
 * Given a pre-defined path of 2D waypoints, the robot continuously calculates the steering 
 * curvature required to follow the path by looking ahead a fixed distance (Ld).
 * 
 * CORE MATHEMATICS & GEOMETRY:
 * 1. Global to Local Transformation:
 *      Transform the target waypoint (gx, gy) from the global coordinate frame to the robot's
 *      local coordinate frame (lx, ly):
 *        dx = gx - robotX
 *        dy = gy - robotY
 *        lx =  dx * cos(robotTheta) + dy * sin(robotTheta)
 *        ly = -dx * sin(robotTheta) + dy * cos(robotTheta)
 * 2. Curvature (gamma):
 *      The curvature of the circular arc connecting the robot to the look-ahead point is:
 *        gamma = (2 * ly) / (Ld * Ld)
 * 3. Differential Drive Mapping:
 *      Map the curvature gamma to the left and right wheel velocities:
 *        vL = vTarget * (1.0 - gamma * W / 2)
 *        vR = vTarget * (1.0 + gamma * W / 2)
 * 
 * LOOK-AHEAD TUNING BEHAVIOR:
 * - Small Ld (e.g., 5cm): Aggressive tracking, tracks path tightly, but can cause oscillations.
 * - Large Ld (e.g., 20cm): Smooth tracking, no oscillations, but cuts corners heavily.
 * 
 * CLI SIMULATOR:
 * The 20 Hz simulation tracks the robot's coordinates and updates wheel speeds dynamically.
 * Plots robot position (X, Y) vs the active target waypoint on the Serial Plotter.
 */

// --- ROBOT CONFIGURATION (Dimensions in cm) ---
const float WHEEL_TRACK = 15.0f; // Track width (distance between wheels)
float Ld = 12.0f;                // Look-Ahead Distance (tuning parameter)
float vTarget = 8.0f;            // Target linear speed (cm/s)

// --- PATH WAYPOINTS ---
struct Waypoint {
  float x;
  float y;
};

// A path consisting of 5 waypoints (forming a sweeping curve)
const int NUM_WAYPOINTS = 5;
const Waypoint path[NUM_WAYPOINTS] = {
  { 0.0f,  0.0f  },
  { 20.0f, 5.0f  },
  { 40.0f, 25.0f },
  { 60.0f, 20.0f },
  { 80.0f, 0.0f  }
};

int activeWaypointIdx = 1; // Start by targeting the second waypoint

// --- ROBOT STATE ---
float robotX = 0.0f;
float robotY = 0.0f;
float robotTheta = 0.0f; // Heading (radians)

// --- CONTROL VARIABLES ---
bool trackingActive = false;
bool printPlotter = true;
const float dt = 0.05f; // 20 Hz loop (dt = 0.05s)

void setup() {
  Serial.begin(115200);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 99: Pure Pursuit Autonomous Path Tracking"));
  Serial.println(F("=================================================="));
  
  printMenu();
}

void loop() {
  static unsigned long lastUpdate = 0;
  
  if (trackingActive && (millis() - lastUpdate >= 50)) {
    lastUpdate = millis();
    runPurePursuitStep();
  }

  // Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCLICommand(cmd);
  }
}

// =============================================================
//  PURE PURSUIT ENGINE
// =============================================================
void runPurePursuitStep() {
  // 1. Check if we reached the final goal (within 4cm of last waypoint)
  float distToGoal = getDistance(robotX, robotY, path[NUM_WAYPOINTS-1].x, path[NUM_WAYPOINTS-1].y);
  if (distToGoal < 4.0f) {
    trackingActive = false;
    Serial.println(F("\n[GOAL] Destination reached! Path tracking successfully completed."));
    return;
  }

  // 2. Select look-ahead target waypoint
  // Find the first waypoint in the path that is at least Ld distance away
  while (activeWaypointIdx < NUM_WAYPOINTS - 1) {
    float dist = getDistance(robotX, robotY, path[activeWaypointIdx].x, path[activeWaypointIdx].y);
    if (dist < Ld) {
      activeWaypointIdx++; // Move to next look-ahead target segment
    } else {
      break;
    }
  }

  float targetX = path[activeWaypointIdx].x;
  float targetY = path[activeWaypointIdx].y;

  // 3. Transform target to robot's local frame
  float dx = targetX - robotX;
  float dy = targetY - robotY;
  
  // lx = forward displacement, ly = lateral displacement (positive left)
  float lx =  dx * cos(robotTheta) + dy * sin(robotTheta);
  float ly = -dx * sin(robotTheta) + dy * cos(robotTheta);

  // 4. Calculate steering curvature (gamma)
  // gamma = 2 * ly / (Ld^2)
  float gamma = (2.0f * ly) / (Ld * Ld);

  // 5. Calculate wheel velocities
  float vL = vTarget * (1.0f - (gamma * WHEEL_TRACK / 2.0f));
  float vR = vTarget * (1.0f + (gamma * WHEEL_TRACK / 2.0f));

  // 6. Simulate Odometry/Kinematics update for 2D motion (dt = 0.05s)
  float vCenter = (vL + vR) / 2.0f;
  float dTheta = (vR - vL) * dt / WHEEL_TRACK;
  
  float midTheta = robotTheta + (dTheta / 2.0f);
  robotX += vCenter * dt * cos(midTheta);
  robotY += vCenter * dt * sin(midTheta);
  robotTheta += dTheta;

  // Constrain theta to [-pi, +pi]
  if (robotTheta > PI) robotTheta -= 2.0f * PI;
  else if (robotTheta < -PI) robotTheta += 2.0f * PI;

  // 7. Output Plotter Telemetry
  if (printPlotter) {
    Serial.print(F("RobotX:"));     Serial.print(robotX, 2);
    Serial.print(F(",RobotY:"));     Serial.print(robotY, 2);
    Serial.print(F(",TargetX:"));    Serial.print(targetX, 2);
    Serial.print(F(",TargetY:"));    Serial.print(targetY, 2);
    Serial.print(F(",Curvature:"));  Serial.print(gamma * 10.0f); // Scaled for plotting visibility
    Serial.print(F(",Heading:"));    Serial.println(robotTheta * 180.0f / PI, 1);
  }
}

float getDistance(float x1, float y1, float x2, float y2) {
  return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// =============================================================
//  CLI COMMAND INTERFACE
// =============================================================
void handleCLICommand(char cmd) {
  if (cmd == '\n' || cmd == '\r') return;

  switch (cmd) {
    case 's':
    case 'S':
      trackingActive = !trackingActive;
      Serial.print(F("[SYSTEM] Path Tracking status: "));
      Serial.println(trackingActive ? F("ACTIVE") : F("PAUSED"));
      break;

    case 'r':
    case 'R':
      // Reset robot to starting origin
      robotX = 0.0f;
      robotY = 0.0f;
      robotTheta = 0.0f;
      activeWaypointIdx = 1;
      Serial.println(F("[SYSTEM] Robot position reset to (0, 0, 0)"));
      break;

    // Tune Look-Ahead Distance Ld
    case '+':
    case 'l':
      Ld = constrain(Ld + 2.0f, 4.0f, 30.0f);
      Serial.print(F("[CONFIG] Look-Ahead Distance Ld increased to: ")); 
      Serial.print(Ld); Serial.println(F(" cm"));
      break;
    case '-':
    case 'd':
      Ld = constrain(Ld - 2.0f, 4.0f, 30.0f);
      Serial.print(F("[CONFIG] Look-Ahead Distance Ld decreased to: ")); 
      Serial.print(Ld); Serial.println(F(" cm"));
      break;

    case 'p':
    case 'P':
      printPlotter = !printPlotter;
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
  Serial.println(F("\n--- PURE PURSUIT PATH TRACKER CLI ---"));
  Serial.println(F(" 's'        : Start / Pause autonomous path tracking"));
  Serial.println(F(" 'r'        : Reset robot coordinates to (0,0,0) and restart path"));
  Serial.println(F(" '+' or 'l' : Increase Look-Ahead distance Ld (+2 cm)"));
  Serial.println(F(" '-' or 'd' : Decrease Look-Ahead distance Ld (-2 cm)"));
  Serial.println(F(" 'p'        : Toggle Serial Plotter data format ON/OFF"));
  Serial.println(F(" 'h'        : Display this help command list"));
  Serial.println(F("--------------------------------------\n"));
}

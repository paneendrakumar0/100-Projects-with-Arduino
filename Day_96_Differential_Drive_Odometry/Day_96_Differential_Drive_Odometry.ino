/*
 * 100 Projects with Arduino - Day 96
 * Project: Differential Drive Robot Odometry & Dead Reckoning
 * 
 * DESCRIPTION:
 * This project implements local position estimation (odometry or dead reckoning) for a 
 * two-wheeled differential drive mobile robot. 
 * 
 * By reading rotary encoders attached to the left and right wheels, the robot tracks its
 * coordinate position (X, Y) and heading (Theta) relative to its startup position (0, 0, 0).
 * 
 * CORE MATHEMATICS (RUNGE-KUTTA MIDPOINT INTEGRATION):
 * 1. Wheel Displacement:
 *      dL = ticksLeft * DistancePerTick
 *      dR = ticksRight * DistancePerTick
 * 2. Linear Center Displacement:
 *      dC = (dL + dR) / 2
 * 3. Angular Yaw Displacement:
 *      dTheta = (dR - dL) / WheelTrack
 * 4. Coordinate Update (Midpoint approximation):
 *      thetaNew = thetaOld + dTheta
 *      XNew = xOld + dC * cos(thetaOld + dTheta / 2)
 *      YNew = yOld + dC * sin(thetaOld + dTheta / 2)
 * 
 * INTERACTIVE CLI & KINEMATICS SIMULATOR:
 * To allow testing without physical encoders, the CLI allows manually incrementing/decrementing
 * encoder ticks or launching automated path simulations (straight line, circular orbit, pivot turns).
 * Telemetry is formatted for the Serial Plotter to visualize the robot's trajectory in 2D space.
 * 
 * WIRING (For real-world encoders):
 * - Left Encoder Signal (Interrupt A)  -> Pin 2 (INT0)
 * - Right Encoder Signal (Interrupt B) -> Pin 3 (INT1)
 * - Encoders powered by 5V and GND.
 */

// --- ROBOT PHYSICAL CONSTANTS (Dimensions in cm) ---
const float WHEEL_DIAMETER   = 6.6f;   // Diameter of robot wheels (e.g. 66mm standard yellow wheel)
const int   TICKS_PER_REV    = 20;     // Encoder slots per revolution (standard plastic disc)
const float WHEEL_TRACK      = 15.0f;  // Width between wheel contact patches (track width)

// Distance traveled per single encoder slot transition
// Distance = (pi * diameter) / ticks_per_rev
const float DISTANCE_PER_TICK = (PI * WHEEL_DIAMETER) / (float)TICKS_PER_REV;

// --- ODOMETRY STATE VARIABLES ---
volatile long leftEncoderTicks = 0;
volatile long rightEncoderTicks = 0;

float robotX = 0.0f;     // Position X (cm)
float robotY = 0.0f;     // Position Y (cm)
float robotTheta = 0.0f; // Heading angle (Radians, 0 = facing along positive X axis)

// --- SIMULATION VARIABLES ---
bool autoSimActive = false;
char simPattern = 'f'; // 'f' = forward, 'c' = circle, 't' = spin turn
unsigned long lastSimUpdate = 0;
const unsigned long SIM_INTERVAL = 100; // 10 Hz updates

void setup() {
  Serial.begin(9600);

  // Configure hardware encoder pins (uses internal pullups)
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  // Attach external interrupts to count ticks
  attachInterrupt(digitalPinToInterrupt(2), leftEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(3), rightEncoderISR, RISING);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 96: Differential Drive Robot Odometry"));
  Serial.println(F("=================================================="));
  
  printMenu();
}

void loop() {
  // 1. Run Automated Path Simulator if active
  if (autoSimActive && (millis() - lastSimUpdate >= SIM_INTERVAL)) {
    lastSimUpdate = millis();
    generateSimulatedTicks();
  }

  // 2. Perform Odometry Update Loop (Non-blocking)
  static unsigned long lastOdomUpdate = 0;
  if (millis() - lastOdomUpdate >= 50) { // 20 Hz calculation frequency
    lastOdomUpdate = millis();
    updateRobotOdometry();
  }

  // 3. Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCLICommand(cmd);
  }
}

// =============================================================
//  INTERRUPT SERVICE ROUTINES (ISRS)
// =============================================================
void leftEncoderISR() {
  // In a real robot, we would check a direction pin to increment or decrement.
  // In this basic setup, we assume forward motion.
  leftEncoderTicks++;
}

void rightEncoderISR() {
  rightEncoderTicks++;
}

// =============================================================
//  ODOMETRY CORE KINEMATIC RESOLVER
// =============================================================
void updateRobotOdometry() {
  static long lastTicksL = 0;
  static long lastTicksR = 0;

  // Read volatile variables atomically
  long currentTicksL, currentTicksR;
  noInterrupts();
  currentTicksL = leftEncoderTicks;
  currentTicksR = rightEncoderTicks;
  interrupts();

  // Calculate delta ticks since last cycle
  long deltaL = currentTicksL - lastTicksL;
  long deltaR = currentTicksR - lastTicksR;
  
  // Save current ticks for next iteration
  lastTicksL = currentTicksL;
  lastTicksR = currentTicksR;

  // If the robot hasn't moved, skip integration to save CPU cycles
  if (deltaL == 0 && deltaR == 0) return;

  // 1. Calculate physical displacements for each wheel
  float dL = (float)deltaL * DISTANCE_PER_TICK;
  float dR = (float)deltaR * DISTANCE_PER_TICK;

  // 2. Calculate linear center displacement
  float dC = (dL + dR) / 2.0f;

  // 3. Calculate rotation change in radians
  float dTheta = (dR - dL) / WHEEL_TRACK;

  // 4. Integrate coordinate updates using midpoint approximation
  // Adds half of the heading change before calculating coordinate projection
  float midTheta = robotTheta + (dTheta / 2.0f);
  
  robotX += dC * cos(midTheta);
  robotY += dC * sin(midTheta);
  robotTheta += dTheta;

  // Constrain theta to the range [-pi, +pi] (standard robotics convention)
  if (robotTheta > PI) {
    robotTheta -= 2.0f * PI;
  } else if (robotTheta < -PI) {
    robotTheta += 2.0f * PI;
  }

  // Output kinematics telemetry for Serial Monitor / Plotter
  float thetaDeg = robotTheta * 180.0f / PI;
  Serial.print(F("X:"));         Serial.print(robotX, 2);
  Serial.print(F(",Y:"));         Serial.print(robotY, 2);
  Serial.print(F(",Heading:"));   Serial.print(thetaDeg, 1);
  Serial.print(F(",LeftTicks:"));  Serial.print(currentTicksL);
  Serial.print(F(",RightTicks:")); Serial.println(currentTicksR);
}

// =============================================================
//  PATH SIMULATION CONTROLS
// =============================================================
void generateSimulatedTicks() {
  noInterrupts();
  switch (simPattern) {
    case 'f': // Path: Move Straight Forward
      leftEncoderTicks  += 2;
      rightEncoderTicks += 2;
      break;
    case 'c': // Path: Circle (Left wheel turns faster, tracing a curve to the right)
      leftEncoderTicks  += 3;
      rightEncoderTicks += 1;
      break;
    case 't': // Path: Turn on the spot (Spin turn CCW: left backwards, right forwards)
      leftEncoderTicks  -= 1;
      rightEncoderTicks += 1;
      break;
  }
  interrupts();
}

void handleCLICommand(char cmd) {
  if (cmd == '\n' || cmd == '\r') return;

  switch (cmd) {
    // Manual ticks simulation
    case 'l':
      noInterrupts(); leftEncoderTicks += 5; interrupts();
      Serial.println(F("[CLI] Added 5 ticks to Left Wheel"));
      break;
    case 'r':
      noInterrupts(); rightEncoderTicks += 5; interrupts();
      Serial.println(F("[CLI] Added 5 ticks to Right Wheel"));
      break;
      
    // Simulation pattern selections
    case 'f':
    case 'F':
      simPattern = 'f';
      autoSimActive = true;
      Serial.println(F("[SIMULATOR] Pattern set to: STRAIGHT FORWARD"));
      break;
    case 'c':
    case 'C':
      simPattern = 'c';
      autoSimActive = true;
      Serial.println(F("[SIMULATOR] Pattern set to: CIRCULAR CURVE"));
      break;
    case 't':
    case 'T':
      simPattern = 't';
      autoSimActive = true;
      Serial.println(F("[SIMULATOR] Pattern set to: PIVOT SPIN TURN"));
      break;
      
    // Simulation state controls
    case 'p':
    case 'P':
      autoSimActive = false;
      Serial.println(F("[SIMULATOR] Path updates paused."));
      break;
    case 'o':
    case 'O':
      // Reset odometry coordinate system
      noInterrupts();
      leftEncoderTicks = 0;
      rightEncoderTicks = 0;
      interrupts();
      robotX = 0.0f;
      robotY = 0.0f;
      robotTheta = 0.0f;
      Serial.println(F("[SYSTEM] Odometry coordinate system reset to (0, 0, 0)"));
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
  Serial.println(F("\n--- ODOMETRY SIMULATION CLI ---"));
  Serial.println(F(" Manual Tick Simulation:"));
  Serial.println(F(" 'l' : Increment Left wheel ticks (+5)"));
  Serial.println(F(" 'r' : Increment Right wheel ticks (+5)"));
  Serial.println(F(" Auto Path Tracking (Updates at 10 Hz):"));
  Serial.println(F(" 'f' : Start Straight Forward simulation"));
  Serial.println(F(" 'c' : Start Circular Curve simulation"));
  Serial.println(F(" 't' : Start Pivot Spin Turn simulation"));
  Serial.println(F(" Control commands:"));
  Serial.println(F(" 'p' : Pause active simulation updates"));
  Serial.println(F(" 'o' : Reset robot coordinates to (0, 0, 0)"));
  Serial.println(F(" 'h' : Display this help command list"));
  Serial.println(F("-------------------------------\n"));
}

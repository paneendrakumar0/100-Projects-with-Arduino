/*
 * 100 Projects with Arduino - Day 98
 * Project: 1D Kalman Filter for IMU Sensor Fusion & Bias Estimation
 * 
 * DESCRIPTION:
 * This project implements an optimal 1D Kalman Filter for sensor fusion on an IMU (Inertial
 * Measurement Unit). It combines readings from an Accelerometer (noisy, but no drift) and a
 * Gyroscope (low-noise, but drifts due to integrated bias) to estimate the true tilt angle
 * of a robot.
 * 
 * THE MATHEMATICAL MODEL:
 * The state vector is x = [Angle, GyroBias]^T.
 * The system predicts angle by integrating gyroscope rate minus estimated bias:
 *   Angle = Angle + dt * (Rate - GyroBias)
 * 
 * When an accelerometer measurement arrives, the Kalman Filter calculates the Kalman Gain
 * dynamically based on state covariance P and sensor noise parameters:
 *   Q_angle  : Process noise covariance for the accelerometer angle
 *   Q_bias   : Process noise covariance for the gyroscope drift rate
 *   R_measure: Measurement noise covariance for the accelerometer
 * 
 * INTERACTIVE SIMULATOR:
 * To make this testable without a physical MPU6050, the Arduino runs a 100 Hz simulation loop
 * generating:
 * 1. A true sinusoidal oscillation angle.
 * 2. A simulated Gyroscope reading with an adjustable bias drift (e.g., +2.0 deg/sec) and noise.
 * 3. A simulated Accelerometer reading with high-frequency noise and sudden "bumps" (spikes).
 * 
 * Telemetry compares the raw inputs, a Complementary Filter, and our Kalman Filter.
 * 
 * WIRING (For real MPU6050):
 * - SDA -> A4 | SCL -> A5
 * - VCC -> 5V | GND -> GND
 */

// --- KALMAN FILTER PARAMETERS ---
float Q_angle   = 0.001f; // Process noise covariance for the angle
float Q_bias    = 0.003f; // Process noise covariance for the gyro bias
float R_measure = 0.03f;  // Measurement noise covariance for the accelerometer

// --- KALMAN FILTER STATE VARIABLES ---
float kalmanAngle = 0.0f; // Estimated angle (filtered output)
float gyroBias    = 0.0f; // Estimated gyroscope bias drift

// Error covariance matrix (2x2)
float P[2][2] = {
  { 1.0f, 0.0f },
  { 0.0f, 1.0f }
};

// --- COMPLEMENTARY FILTER STATE ---
float compAngle = 0.0f;
const float COMP_ALPHA = 0.98f; // Filtering weight

// --- SIMULATION VARIABLES ---
float trueAngle = 0.0f;
float simTime = 0.0f;
float simGyroBias = 2.0f; // Constant simulated drift bias (deg/sec)
bool injectBump = false;
bool printPlotter = true;

// Sampling rate: 100 Hz (dt = 0.01 seconds)
const float dt = 0.01f;
const unsigned long loopIntervalUs = 10000; // 10ms

void setup() {
  Serial.begin(115200); // High baud rate for 100 Hz telemetry plotter
  randomSeed(analogRead(A0));

  Serial.println(F("=================================================="));
  Serial.println(F("Day 98: 1D Kalman Filter for IMU Sensor Fusion"));
  Serial.println(F("=================================================="));
  
  printMenu();
}

void loop() {
  static unsigned long lastUpdateUs = 0;
  unsigned long currentUs = micros();

  // Run filter loop at a precise 100 Hz timing
  if (currentUs - lastUpdateUs >= loopIntervalUs) {
    lastUpdateUs = currentUs;
    runFilterFusionStep();
  }

  // Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCLICommand(cmd);
  }
}

// =============================================================
//  KALMAN FILTER MATHEMATICS
// =============================================================
/**
 * Updates the Kalman filter estimate using a rate input and a measurement input.
 * @param newAngle Accelerometer measured angle (degrees).
 * @param newRate Gyroscope measured angular velocity (degrees/second).
 */
float updateKalmanFilter(float newAngle, float newRate) {
  // --- STEP 1: PREDICT ---
  // Estimate new angle by integrating gyro rate minus bias
  float rate = newRate - gyroBias;
  kalmanAngle += dt * rate;

  // Update State Error Covariance matrix P
  P[0][0] += dt * (dt * P[1][1] - P[0][1] - P[1][0] + Q_angle);
  P[0][1] -= dt * P[1][1];
  P[1][0] -= dt * P[1][1];
  P[1][1] += Q_bias * dt;

  // --- STEP 2: UPDATE ---
  // Innovation (Measurement error)
  float y = newAngle - kalmanAngle;

  // Innovation Covariance
  float S = P[0][0] + R_measure;

  // Calculate Kalman Gain (2x1 vector)
  float K[2];
  K[0] = P[0][0] / S;
  K[1] = P[1][0] / S;

  // Update State Vector (Angle and Bias)
  kalmanAngle += K[0] * y;
  gyroBias    += K[1] * y;

  // Update State Error Covariance matrix P
  float P00_temp = P[0][0];
  float P01_temp = P[0][1];

  P[0][0] -= K[0] * P00_temp;
  P[0][1] -= K[0] * P01_temp;
  P[1][0] -= K[1] * P00_temp;
  P[1][1] -= K[1] * P01_temp;

  return kalmanAngle;
}

// =============================================================
//  IMU SIMULATOR AND FUSION CONTROLLER
// =============================================================
void runFilterFusionStep() {
  simTime += dt;

  // 1. Calculate TRUE motion: Sine wave oscillation between -15 and +15 degrees
  trueAngle = 15.0f * sin(simTime * 2.0f); 
  
  // True velocity (derivative of true angle: 15 * 2 * cos(2t))
  float trueRate = 30.0f * cos(simTime * 2.0f);

  // 2. Generate simulated Gyroscope (True rate + constant bias + noise)
  float gyroNoise = (random(-100, 100) / 100.0f) * 1.5f; // +/- 1.5 deg/sec noise
  float rawGyro = trueRate + simGyroBias + gyroNoise;

  // 3. Generate simulated Accelerometer (True angle + noise + bumps)
  float accelNoise = (random(-100, 100) / 100.0f) * 3.0f; // +/- 3 deg noise
  float rawAccel = trueAngle + accelNoise;

  // Inject a sudden bump (simulates robot driving over a brick or mechanical shock)
  if (injectBump) {
    rawAccel += 30.0f; // Huge transient lateral spike
    injectBump = false;
  }

  // 4. Run Complementary Filter
  compAngle = COMP_ALPHA * (compAngle + rawGyro * dt) + (1.0f - COMP_ALPHA) * rawAccel;

  // 5. Run Kalman Filter
  float filteredAngle = updateKalmanFilter(rawAccel, rawGyro);

  // 6. Output Telemetry formatted for Serial Plotter
  if (printPlotter) {
    Serial.print(F("TrueAngle:"));   Serial.print(trueAngle, 2);
    Serial.print(F(",RawAccel:"));   Serial.print(rawAccel, 2);
    Serial.print(F(",CompFilter:")); Serial.print(compAngle, 2);
    Serial.print(F(",Kalman:"));     Serial.print(filteredAngle, 2);
    Serial.print(F(",EstBias:"));    Serial.println(gyroBias, 3);
  }
}

// =============================================================
//  CLI COMMAND INTERFACE
// =============================================================
void handleCLICommand(char cmd) {
  if (cmd == '\n' || cmd == '\r') return;

  switch (cmd) {
    case 'b':
    case 'B':
      injectBump = true;
      Serial.println(F("[SIM] Transient bump/shock injected into Accelerometer!"));
      break;

    case 'z':
    case 'Z':
      // Toggle gyro drift bias on/off
      simGyroBias = (simGyroBias == 0.0f) ? 3.0f : 0.0f;
      Serial.print(F("[SIM] Gyro drift bias set to: ")); 
      Serial.print(simGyroBias); Serial.println(F(" deg/sec"));
      break;

    // Adjust process/measurement noise covariances
    case '1':
      R_measure = constrain(R_measure + 0.05f, 0.01f, 2.0f);
      Serial.print(F("[CONFIG] R_measure (Sensor Noise) increased to: ")); Serial.println(R_measure, 2);
      break;
    case '2':
      R_measure = constrain(R_measure - 0.05f, 0.01f, 2.0f);
      Serial.print(F("[CONFIG] R_measure (Sensor Noise) decreased to: ")); Serial.println(R_measure, 2);
      break;

    case '3':
      Q_angle = constrain(Q_angle + 0.001f, 0.0001f, 0.1f);
      Serial.print(F("[CONFIG] Q_angle (Process Noise) increased to: ")); Serial.println(Q_angle, 4);
      break;
    case '4':
      Q_angle = constrain(Q_angle - 0.001f, 0.0001f, 0.1f);
      Serial.print(F("[CONFIG] Q_angle (Process Noise) decreased to: ")); Serial.println(Q_angle, 4);
      break;

    case 'p':
    case 'P':
      printPlotter = !printPlotter;
      Serial.print(F("[SYSTEM] Telemetry printing: "));
      Serial.println(printPlotter ? F("ON") : F("OFF"));
      break;

    case 'r':
    case 'R':
      // Reset filter states
      kalmanAngle = 0.0f;
      gyroBias = 0.0f;
      compAngle = 0.0f;
      P[0][0] = 1.0f; P[0][1] = 0.0f;
      P[1][0] = 0.0f; P[1][1] = 1.0f;
      Serial.println(F("[SYSTEM] Filter states and covariance matrices reset."));
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
  Serial.println(F("\n--- KALMAN FILTER SIMULATOR CLI ---"));
  Serial.println(F(" Simulation Toggles:"));
  Serial.println(F(" 'b' : Inject a shock/bump spike (+30 deg acceleration peak)"));
  Serial.println(F(" 'z' : Toggle Gyro constant drift bias (0.0 vs +3.0 deg/sec)"));
  Serial.println(F(" Covariance Tuning:"));
  Serial.println(F(" '1' / '2' : Increase / Decrease R_measure (Sensor noise weight)"));
  Serial.println(F(" '3' / '4' : Increase / Decrease Q_angle (Process noise weight)"));
  Serial.println(F(" System commands:"));
  Serial.println(F(" 'p' : Toggle Serial Plotter data printing ON/OFF"));
  Serial.println(F(" 'r' : Reset filter accumulations and error covariance"));
  Serial.println(F(" 'h' : Display this help command reference"));
  Serial.println(F("------------------------------------\n"));
}

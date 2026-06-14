/*
 * 100 Projects with Arduino - Day 77
 * Project: Self-Balancing Robot Controller (Stabilization Loop & PID Tuning Shell)
 *
 * DESCRIPTION:
 * This project implements the complete control loop for a two-wheeled Self-Balancing Robot.
 * To align with professional robotic stability systems:
 * 1. Complementary Filter Pitch Estimator: Reads MPU6050 direct I2C registers to calculate
 *    the robot's tilt angle relative to gravity.
 * 2. High-Speed PID Controller: Computes the corrective motor control effort at a precise
 *    100 Hz (10ms) frequency to drive the robot towards its upright setpoint ($0^\circ$).
 * 3. Anti-Windup Integration Clamping: Clamps the accumulator to prevent the integral term
 *    from saturating under steady-state load (which causes overshoot and oscillations).
 * 4. Dual Motor Driver Interface (L298N): Translates control effort magnitudes (0-255) into
 *    PWM speed signals and direction commands for left/right wheel motors.
 * 5. Interactive Tuning Shell: Allows real-time modification of Kp, Ki, and Kd coefficients,
 *    and tilt angle simulation via Serial Commands.
 *
 * PID CONTROL MATHEMATICS:
 * - Error: $e(t) = \theta_{target} - \theta_{current}$
 * - Proportional: $P = K_p \cdot e(t)$ (immediate response to tilt)
 * - Integral: $I = K_i \cdot \int e(t) dt$ (corrects steady-state offset/friction)
 * - Derivative: $D = K_d \cdot \frac{de(t)}{dt}$ (damps oscillations and predicts overshoot)
 * - Control Output: $u(t) = P + I + D$
 *
 * WIRING:
 * - MPU6050 Pin -> Arduino Uno Pin
 *   - VCC -> 5V | GND -> GND | SDA -> A4 | SCL -> A5
 * - L298N Motor Driver -> Arduino Uno Pin
 *   - ENA (Left PWM)      -> Pin 5
 *   - IN1, IN2 (Left Dir) -> Pin 3, Pin 4
 *   - IN3, IN4 (Right Dir)-> Pin 7, Pin 8
 *   - ENB (Right PWM)     -> Pin 6
 *   - GND                 -> GND
 */

#include <Wire.h>

// --- MPU6050 I2C CONFIG ---
const uint8_t MPU6050_ADDR = 0x68;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;
const uint8_t REG_PWR_MGMT_1 = 0x6B;
const float ACCEL_SCALE_FACTOR = 16384.0f;
const float GYRO_SCALE_FACTOR = 131.0f;
// const float RAD_TO_DEG = 180.0f / PI; // Already defined in Arduino.h

// --- MOTOR CONTROLLER PIN DEFINITIONS ---
const int ENA = 5;  // Left Motor Enable (PWM)
const int IN1 = 3;  // Left Motor Direction 1
const int IN2 = 4;  // Left Motor Direction 2
const int IN3 = 7;  // Right Motor Direction 1
const int IN4 = 8;  // Right Motor Direction 2
const int ENB = 6;  // Right Motor Enable (PWM)

// --- CONTROL LOOP CONFIGURATION ---
unsigned long lastLoopTime = 0;
const unsigned long LOOP_PERIOD_US = 10000;  // 10ms = 100 Hz sample rate

// --- SENSOR STATE & FILTER CONFIG ---
float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;
float robotPitch = 0.0f;    // Fused tilt angle (degrees)
const float ALPHA = 0.96f;  // Complementary Filter Weight

// --- PID PARAMETERS ---
float Kp = 22.5f;  // Proportional Gain
float Ki = 1.8f;   // Integral Gain
float Kd = 1.2f;   // Derivative Gain

float targetAngle = 0.0f;           // Upright setpoint (degrees)
float errorIntegral = 0.0f;         // Accumulator for Ki
float lastError = 0.0f;             // Memory for Kd
const float MAX_INTEGRAL = 150.0f;  // Anti-windup limit

// Simulation & Safety parameters
bool simulationMode = false;
float simPitch = 0.0f;
bool motorsEnabled = false;  // Safety lock, turn on via Serial

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Initialize motor controller pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  stopMotors();

  Serial.println(F("=================================================="));
  Serial.println(F("Day 77: Self-Balancing Robot Controller"));
  Serial.println(F("=================================================="));

  if (!initMPU6050()) {
    Serial.println(F("[SYSTEM] MPU6050 not found. Entering Simulation Mode automatically."));
    simulationMode = true;
  } else {
    Serial.println(F("[SYSTEM] MPU6050 detected. Calibrating gyroscope..."));
    calibrateGyro();
  }

  printMenu();
  lastLoopTime = micros();
}

void loop() {
  unsigned long currentMicros = micros();

  // Run stabilization loop at 100 Hz (non-blocking)
  if (currentMicros - lastLoopTime >= LOOP_PERIOD_US) {
    float dt = (currentMicros - lastLoopTime) / 1000000.0f;
    lastLoopTime = currentMicros;

    // 1. Resolve Pitch Tilt Angle
    if (simulationMode) {
      robotPitch = simPitch;
    } else {
      int16_t axRaw, ayRaw, azRaw, tempRaw, gxRaw, gyRaw, gzRaw;
      if (readIMU(axRaw, ayRaw, azRaw, tempRaw, gxRaw, gyRaw, gzRaw)) {
        float ax = axRaw / ACCEL_SCALE_FACTOR;
        float ay = ayRaw / ACCEL_SCALE_FACTOR;
        float az = azRaw / ACCEL_SCALE_FACTOR;
        float gy = (gyRaw - gyroOffsetY) / GYRO_SCALE_FACTOR;  // Y-axis is tilt velocity

        // Trigonometric tilt angle from accelerometer
        float accPitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

        // Complementary Filter
        robotPitch = ALPHA * (robotPitch + gy * dt) + (1.0f - ALPHA) * accPitch;
      }
    }

    // 2. Compute PID Control Effort
    float controlEffort = calculatePID(robotPitch, dt);

    // 3. Drive Motors (if safety lock is released)
    if (motorsEnabled && abs(robotPitch) < 40.0f) {
      // Robot will shut down if tilt angle exceeds 40 degrees (safety drop lock)
      driveMotors(controlEffort);
    } else {
      stopMotors();
      if (motorsEnabled && abs(robotPitch) >= 40.0f) {
        Serial.println(F("[SAFETY] Robot fell over! Motors disabled. Reset state to resume."));
        motorsEnabled = false;
      }
    }

    // 4. Output Telemetry at 10 Hz
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 100) {
      lastPrint = millis();
      printTelemetry(controlEffort);
    }
  }

  // Poll for tuning CLI commands
  pollTuningShell();
}

// =============================================================
//  PID CONTROL CALCULATION
// =============================================================
float calculatePID(float currentAngle, float dt) {
  float error = targetAngle - currentAngle;

  // Proportional Term
  float pTerm = Kp * error;

  // Integral Term with Anti-Windup Clamping
  errorIntegral += error * dt;
  errorIntegral = constrain(errorIntegral, -MAX_INTEGRAL, MAX_INTEGRAL);
  float iTerm = Ki * errorIntegral;

  // Derivative Term (Damping rate of change)
  float derivative = (error - lastError) / dt;
  float dTerm = Kd * derivative;
  lastError = error;

  // Total control effort output
  return pTerm + iTerm + dTerm;
}

// =============================================================
//  MOTOR DRIVER INTERFACE (L298N)
// =============================================================
void driveMotors(float output) {
  // Determine direction based on sign of control effort
  bool forward = (output >= 0.0f);
  int speed = constrain(abs((int)output), 0, 255);

  // Left Motor Direction
  digitalWrite(IN1, forward ? HIGH : LOW);
  digitalWrite(IN2, forward ? LOW : HIGH);

  // Right Motor Direction
  digitalWrite(IN3, forward ? HIGH : LOW);
  digitalWrite(IN4, forward ? LOW : HIGH);

  // Apply PWM speeds
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// =============================================================
//  LOW-LEVEL I2C ACCESS & CALIBRATION
// =============================================================
bool initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75);
  if (Wire.endTransmission() != 0) return false;
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available() && Wire.read() != 0x68) return false;

  // Wake up
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  return (Wire.endTransmission() == 0);
}

void calibrateGyro() {
  long sumY = 0;
  const int samples = 200;
  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, temp, gx, gy, gz;
    if (readIMU(ax, ay, az, temp, gx, gy, gz)) {
      sumY += gy;
    } else {
      i--;
    }
    delay(10);
  }
  gyroOffsetY = (float)sumY / samples;
}

bool readIMU(int16_t& ax, int16_t& ay, int16_t& az, int16_t& temp, int16_t& gx, int16_t& gy,
             int16_t& gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission() != 0) return false;
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)14);
  if (Wire.available() >= 14) {
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();
    az = (Wire.read() << 8) | Wire.read();
    temp = (Wire.read() << 8) | Wire.read();
    gx = (Wire.read() << 8) | Wire.read();
    gy = (Wire.read() << 8) | Wire.read();
    gz = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

// =============================================================
//  TUNING INTERACTIVE SHELL & TELEMETRY
// =============================================================
void pollTuningShell() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    // Read parameter value
    float val = Serial.parseFloat();

    switch (cmd) {
      case 'p':
      case 'P':
        Kp = val;
        Serial.print(F("[PID] Kp set to: "));
        Serial.println(Kp, 3);
        break;
      case 'i':
      case 'I':
        Ki = val;
        errorIntegral = 0;  // Reset integrator on gain change
        Serial.print(F("[PID] Ki set to: "));
        Serial.println(Ki, 3);
        break;
      case 'd':
      case 'D':
        Kd = val;
        Serial.print(F("[PID] Kd set to: "));
        Serial.println(Kd, 3);
        break;
      case 'e':
      case 'E':
        motorsEnabled = (val > 0.0f);
        if (!motorsEnabled) stopMotors();
        Serial.print(F("[MOTOR] Enabled State: "));
        Serial.println(motorsEnabled ? F("ON") : F("OFF"));
        break;
      case 's':
      case 'S':
        simPitch = val;
        if (!simulationMode) {
          simulationMode = true;
          Serial.println(F("[SYSTEM] Simulation Mode turned ON."));
        }
        Serial.print(F("[SIMULATION] Injected Pitch Angle: "));
        Serial.print(simPitch, 2);
        Serial.println(F(" deg"));
        break;
      case 'm':
      case 'M':
        simulationMode = !simulationMode;
        Serial.print(F("[SYSTEM] Simulation Mode: "));
        Serial.println(simulationMode ? F("ON") : F("OFF"));
        break;
      case 'r':
      case 'R':
        errorIntegral = 0;
        lastError = 0;
        robotPitch = 0;
        Serial.println(F("[SYSTEM] Error integrator and angle states reset."));
        break;
      case 'h':
      case 'H':
        printMenu();
        break;
      default:
        break;
    }
  }
}

void printTelemetry(float effort) {
  Serial.print(F("[TELEMETRY] Mode: "));
  Serial.print(simulationMode ? F("SIM") : F("HW "));
  Serial.print(F(" | Active: "));
  Serial.print(motorsEnabled ? F("YES") : F("NO "));
  Serial.print(F(" | Pitch: "));
  Serial.print(robotPitch, 2);
  Serial.print(F(" deg | PID: ["));
  Serial.print(Kp, 1);
  Serial.print(F(", "));
  Serial.print(Ki, 1);
  Serial.print(F(", "));
  Serial.print(Kd, 1);
  Serial.print(F("] | Effort: "));
  Serial.println(effort, 1);
}

void printMenu() {
  Serial.println(F("\n--- SELF-BALANCING PID TUNING SHELL ---"));
  Serial.println(F(" 'p [val]' : Set Proportional Gain Kp (e.g. p 25.5)"));
  Serial.println(F(" 'i [val]' : Set Integral Gain Ki (e.g. i 1.2)"));
  Serial.println(F(" 'd [val]' : Set Derivative Gain Kd (e.g. d 0.8)"));
  Serial.println(F(" 'e [1/0]' : Enable/Disable Motor Outputs (e.g. e 1 to start)"));
  Serial.println(F(" 's [val]' : Inject simulated pitch angle (e.g. s -5.5)"));
  Serial.println(F(" 'm'       : Toggle between Sensor and Simulation Mode"));
  Serial.println(F(" 'r'       : Reset integrated errors & offsets"));
  Serial.println(F(" 'h'       : Print this tuning command menu"));
  Serial.println(F("----------------------------------------"));
}

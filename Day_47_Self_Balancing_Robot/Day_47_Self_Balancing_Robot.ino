/*
 * 100 Projects with Arduino - Day 47
 * Project: Self-Balancing Robot Controller (Inverted Pendulum PID Stabilization)
 *
 * DESCRIPTION:
 * This project implements a closed-loop PID control system to stabilize a two-wheeled
 * self-balancing robot (inverted pendulum) using sensor fusion telemetry from an MPU6050 IMU.
 *
 * CORE SYSTEMS:
 * 1. MPU6050 I2C Register Pipeline: Retrieves raw accelerometer and gyroscope registers.
 * 2. Complementary Filter: Blends high-pass gyro integration and low-pass accelerometer tilt
 *    calculations at a strict 100 Hz sample rate (10ms dt) to calculate the precise pitch angle.
 * 3. Proportional-Integral-Derivative (PID) Controller: Evaluates the angle error, integrates
 *    steady-state errors (with anti-windup clamping), and computes the derivative damping force.
 * 4. Differential Motor Mapping: Translates the PID correction output into dual H-Bridge PWM
 * commands. Incorporates a minimum PWM deadband threshold to overcome static motor friction.
 * 5. Safety Tilt Cut-off: Instantly shuts down the motors if the robot tilts beyond ±40 degrees
 *    to prevent runaway conditions when the robot falls.
 *
 * WIRING:
 * - MPU6050 IMU -> Arduino Uno
 *   - VCC -> 5V (or 3.3V)
 *   - GND -> GND
 *   - SDA -> Pin A4 (I2C Data)
 *   - SCL -> Pin A5 (I2C Clock)
 * - L298N Motor Driver -> Arduino Uno
 *   - ENA (Left PWM)   -> Pin 5
 *   - IN1, IN2         -> Pins 4, 3
 *   - ENB (Right PWM)  -> Pin 6
 *   - IN3, IN4         -> Pins 7, 8
 *   - GND              -> GND (Common Ground)
 */

#include <Wire.h>

// --- MPU6050 DEFINITIONS ---
const int MPU_ADDRESS = 0x68;
const int REG_PWR_MGMT_1 = 0x6B;
const int REG_ACCEL_XOUT_H = 0x3B;
const int REG_GYRO_XOUT_H = 0x43;

// --- SCALE FACTORS (±2g range, ±250°/s range) ---
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE = 131.0;

// --- FILTER COEFFICIENT (Alpha) ---
const float ALPHA = 0.98;

// --- MOTOR CONTROLLER PIN DEFINITIONS ---
const int L_ENA_PIN = 5;
const int L_IN1_PIN = 4;
const int L_IN2_PIN = 3;

const int R_ENB_PIN = 6;
const int R_IN3_PIN = 7;
const int R_IN4_PIN = 8;

const int LED_INDICATOR_PIN = 13;

// --- PID TUNING PARAMETERS ---
// Note: These need to be tuned for your specific chassis, motor voltage, and wheel size.
// Kp: Agility / restoring force. Ki: Steady-state offset. Kd: Damping / oscillation dampener.
double Kp = 32.0;
double Ki = 140.0;
double Kd = 1.8;

// --- CALIBRATION & ANGLE telemetry ---
float gyroBiasY = 0.0;   // Gyroscope offset for Pitch (rotation around Y-axis)
float fusedPitch = 0.0;  // Current estimated tilt angle

// --- PID STATE VARIABLES ---
double targetPitch =
    -1.2;  // Target angle (perfect vertical balance point, adjusted for CoG offset)
double errorSum = 0.0;
double lastError = 0.0;
const double maxErrorSum = 150.0;  // Anti-windup limit for Integral term

// --- TIMING CONFIGURATION ---
unsigned long lastLoopTimeUs = 0;
const unsigned long samplePeriodUs = 10000;  // 10ms sample period = 100 Hz loop rate

// --- MOTOR DEADBAND PARAMETER ---
const int MIN_MOTOR_PWM = 45;  // Minimum PWM required to overcome static motor friction

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(LED_INDICATOR_PIN, OUTPUT);

  // Initialize Motor driver pins
  pinMode(L_ENA_PIN, OUTPUT);
  pinMode(L_IN1_PIN, OUTPUT);
  pinMode(L_IN2_PIN, OUTPUT);
  pinMode(R_ENB_PIN, OUTPUT);
  pinMode(R_IN3_PIN, OUTPUT);
  pinMode(R_IN4_PIN, OUTPUT);

  haltMotors();

  // Wake up MPU6050
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("[ERROR] MPU6050 initialization failed! Halting system.");
    digitalWrite(LED_INDICATOR_PIN, HIGH);
    for (;;);
  }

  // Calibrate Gyroscope bias while stationary
  calibrateGyroscope();

  // Initialize timing
  lastLoopTimeUs = micros();

  // Output column headers for Serial Plotter telemetry
  Serial.println("Target_Angle,Actual_Angle,Motor_Output");
}

void loop() {
  unsigned long currentTimeUs = micros();

  // Strict 100 Hz loop rate (10ms)
  if (currentTimeUs - lastLoopTimeUs >= samplePeriodUs) {
    double dt =
        (double)(currentTimeUs - lastLoopTimeUs) / 1000000.0;  // Loop duration in seconds (~0.01s)
    lastLoopTimeUs = currentTimeUs;

    int16_t rawAccelX, rawAccelY, rawAccelZ;
    int16_t rawGyroX, rawGyroY, rawGyroZ;

    // Read MPU6050 raw registers
    if (readRawAccel(&rawAccelX, &rawAccelY, &rawAccelZ) &&
        readRawGyro(&rawGyroX, &rawGyroY, &rawGyroZ)) {
      // Convert raw accelerometer values to Gs
      float ax = (float)rawAccelX / ACCEL_SCALE;
      float ay = (float)rawAccelY / ACCEL_SCALE;
      float az = (float)rawAccelZ / ACCEL_SCALE;

      // Convert raw gyro Y-axis (Pitch rate) to degrees per second
      float gy = ((float)rawGyroY / GYRO_SCALE) - gyroBiasY;

      // Calculate absolute pitch from accelerometer gravity vector: θ = atan2(-X, Z) or equivalent
      // Pitch rotation revolves about the Y-axis:
      float pitchAccel = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

      // --- COMPLEMENTARY FILTER ---
      // Fuse Accelerometer (stable long term) and Gyroscope (responsive short term)
      fusedPitch = ALPHA * (fusedPitch + gy * dt) + (1.0 - ALPHA) * pitchAccel;

      // --- SAFETY TILT CHECK ---
      // If the robot is tilted more than 40 degrees, it has fallen. Halt motors.
      if (abs(fusedPitch) > 40.0) {
        haltMotors();
        errorSum = 0.0;  // Clear accumulated errors
        lastError = 0.0;

        Serial.print(targetPitch, 1);
        Serial.print(",");
        Serial.print(fusedPitch, 1);
        Serial.println(",0");  // Zero output
        return;
      }

      // --- PID CALCULATIONS ---
      double error = fusedPitch - targetPitch;

      // Proportional term
      double pTerm = Kp * error;

      // Integral term with anti-windup clamping
      errorSum += error * dt;
      errorSum = constrain(errorSum, -maxErrorSum, maxErrorSum);
      double iTerm = Ki * errorSum;

      // Derivative term
      double dTerm = Kd * ((error - lastError) / dt);
      lastError = error;

      // Total correction output
      double pidOutput = pTerm + iTerm + dTerm;

      // --- ACTUATOR CONTROL & DEAD-BAND MAPPING ---
      int motorPWM = (int)pidOutput;

      if (motorPWM > 0) {
        // Falling Forward -> Drive Motors Forward to catch center of gravity
        motorPWM = map(motorPWM, 0, 255, MIN_MOTOR_PWM, 255);
        motorPWM = constrain(motorPWM, MIN_MOTOR_PWM, 255);
        driveMotors(motorPWM, motorPWM);
      } else if (motorPWM < 0) {
        // Falling Backward -> Drive Motors Backward
        motorPWM = -motorPWM;
        motorPWM = map(motorPWM, 0, 255, MIN_MOTOR_PWM, 255);
        motorPWM = constrain(motorPWM, MIN_MOTOR_PWM, 255);
        driveMotors(-motorPWM, -motorPWM);
      } else {
        haltMotors();
      }

      // Output values formatted for the Arduino Serial Plotter
      Serial.print(targetPitch, 1);
      Serial.print(",");
      Serial.print(fusedPitch, 1);
      Serial.print(",");
      Serial.println(pidOutput);
    }
  }
}

// --- I2C REGISTERS PIPELINE ---

bool readRawAccel(int16_t *ax, int16_t *ay, int16_t *az) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(MPU_ADDRESS, 6);
  if (Wire.available() >= 6) {
    *ax = (Wire.read() << 8) | Wire.read();
    *ay = (Wire.read() << 8) | Wire.read();
    *az = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

bool readRawGyro(int16_t *gx, int16_t *gy, int16_t *gz) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_GYRO_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(MPU_ADDRESS, 6);
  if (Wire.available() >= 6) {
    *gx = (Wire.read() << 8) | Wire.read();
    *gy = (Wire.read() << 8) | Wire.read();
    *gz = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

// --- GYROSCOPE CALIBRATION ---

void calibrateGyroscope() {
  Serial.println("[IMU] Calibrating Gyroscope. Keep the robot still on a flat surface...");
  digitalWrite(LED_INDICATOR_PIN, HIGH);

  long sumY = 0;
  int samples = 200;  // Average over 200 samples for high accuracy

  for (int i = 0; i < samples; i++) {
    int16_t gx, gy, gz;
    if (readRawGyro(&gx, &gy, &gz)) {
      sumY += gy;
    }
    delay(5);
  }

  gyroBiasY = (float)(sumY / samples) / GYRO_SCALE;

  digitalWrite(LED_INDICATOR_PIN, LOW);
  Serial.print("[IMU] Calibration complete. Gyro Y bias offset: ");
  Serial.println(gyroBiasY, 4);
  delay(1500);  // Allow user to place the robot in its starting position
}

// --- MOTOR ACTUATION SCHEDULERS ---

void driveMotors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Left Motor control
  if (leftSpeed >= 0) {
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, leftSpeed);
  } else {
    digitalWrite(L_IN1_PIN, LOW);
    digitalWrite(L_IN2_PIN, HIGH);
    analogWrite(L_ENA_PIN, -leftSpeed);
  }

  // Right Motor control
  if (rightSpeed >= 0) {
    digitalWrite(R_IN3_PIN, HIGH);
    digitalWrite(R_IN4_PIN, LOW);
    analogWrite(R_ENB_PIN, rightSpeed);
  } else {
    digitalWrite(R_IN3_PIN, LOW);
    digitalWrite(R_IN4_PIN, HIGH);
    analogWrite(R_ENB_PIN, -rightSpeed);
  }
}

void haltMotors() {
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, 0);

  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, 0);
}

/*
 * 100 Projects with Arduino - Day 42
 * Project: IMU Sensor Fusion via Complementary Filter (Pitch & Roll Estimator)
 * 
 * DESCRIPTION:
 * This project implements a high-performance, real-time sensor fusion algorithm to calculate
 * stable Pitch and Roll angles. To meet advanced mechatronics and control systems standards:
 * 1. Direct I2C Register Pipeline: Interfaces the MPU6050 over I2C at a high sample rate (50 Hz / 20ms loops)
 *    using raw register transactions via Wire.h.
 * 2. Accelerometer Gravity-Vector Trig: Resolves absolute static tilt angles using trigonometric atan2 mapping.
 * 3. Gyroscope Rate Integration: Non-blockingly integrates angular velocity vectors using dynamic time steps (dt).
 * 4. Complementary Sensor Fusion: Blends the high-pass filtered gyroscope data (trusting short-term changes)
 *    and low-pass filtered accelerometer data (trusting long-term gravity orientation) to eliminate both
 *    high-frequency vibration noise and low-frequency gyroscopic drift.
 * 5. Serial Plotter Formatting: Outputs telemetry in a comma-separated format ("Roll_Accel,Roll_Gyro,Roll_Fused")
 *    specifically designed to demonstrate sensor fusion visually in the Arduino Serial Plotter.
 * 
 * SENSOR FUSION PHYSICS & MATH:
 * - Gyroscope Drift: Gyroscopes measure angular rate (ω). To get angle (θ), we integrate over time:
 *     θ_gyro(k) = θ_fused(k-1) + ω * dt
 *   Even after calibration, small sensor biases integrate into a linear drift that wanders off to infinity over time.
 * - Accelerometer Noise: Accelerometers measure absolute tilt using the gravity vector. However, linear acceleration
 *   (e.g., motor vibration or movement) corrupts the gravity vector, making the raw angle extremely noisy:
 *     θ_accel = atan2(Y_accel, Z_accel) * (180 / π)
 * - Complementary Filter Equation:
 *   Combines both signals using a filter coefficient alpha (typically α = 0.98):
 *     θ_fused(k) = α * (θ_fused(k-1) + ω_gyro * dt) + (1 - α) * θ_accel
 *   This is mathematically equivalent to a low-pass filter on the accelerometer and a high-pass filter
 *   on the gyroscope, providing a fast, noise-free, and drift-free angle estimate.
 * 
 * WIRING:
 * - MPU6050 Module -> Arduino Uno
 *   - VCC -> 5V (or 3.3V)
 *   - GND -> GND
 *   - SDA -> Pin A4
 *   - SCL -> Pin A5
 */

#include <Wire.h>

// --- MPU6050 DEFINITIONS ---
const int MPU_ADDRESS = 0x68;
const int REG_PWR_MGMT_1 = 0x6B;
const int REG_ACCEL_XOUT_H = 0x3B;
const int REG_GYRO_XOUT_H = 0x43;

// --- SCALE FACTORS (±2g range, ±250°/s range) ---
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE  = 131.0;

// --- FILTER COEFFICIENT (Alpha) ---
// Trusts Gyro integration for 98% of the step, and corrects 2% of the way toward Accelerometer gravity
const float ALPHA = 0.98;

// --- CALIBRATION OFFSETS ---
float gyroBiasX = 0.0, gyroBiasY = 0.0, gyroBiasZ = 0.0;

// --- SENSOR FUSION ANGLE CONTAINERS ---
float fusedRoll = 0.0;
float fusedPitch = 0.0;
float gyroIntegratedRoll = 0.0; // Unfiltered gyro integration for plotter comparison

// --- TIMING VARIABLES ---
unsigned long lastLoopTimeUs = 0; // Microsecond precision loop timing
const unsigned long samplePeriodUs = 20000; // 20ms sample period = 50 Hz loop rate

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Wake up MPU6050
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("[ERROR] MPU6050 initialization failed! Halt.");
    for (;;);
  }

  // Record gyroscope offset biases while resting flat
  calibrateGyroscope();

  // Initialize timing
  lastLoopTimeUs = micros();
  
  // Output column headers for Serial Plotter labels
  Serial.println("Roll_Accel,Roll_Gyro_Drift,Roll_Fused_Filter");
}

void loop() {
  unsigned long currentTimeUs = micros();
  
  // Non-blocking timer running at exactly 50 Hz
  if (currentTimeUs - lastLoopTimeUs >= samplePeriodUs) {
    // Calculate dynamic time step in seconds
    double dt = (double)(currentTimeUs - lastLoopTimeUs) / 1000000.0;
    lastLoopTimeUs = currentTimeUs;

    // Raw sensor variables
    int16_t rawAccelX, rawAccelY, rawAccelZ;
    int16_t rawGyroX, rawGyroY, rawGyroZ;

    // Read raw data from MPU6050 registers
    if (readRawAccel(&rawAccelX, &rawAccelY, &rawAccelZ) &&
        readRawGyro(&rawGyroX, &rawGyroY, &rawGyroZ)) {

      // Convert raw accelerometer integers to standard G-forces
      float ax = (float)rawAccelX / ACCEL_SCALE;
      float ay = (float)rawAccelY / ACCEL_SCALE;
      float az = (float)rawAccelZ / ACCEL_SCALE;

      // Convert raw gyro rates to degrees/second and subtract stationary offsets
      float gx = ((float)rawGyroX / GYRO_SCALE) - gyroBiasX; // Roll rate
      float gy = ((float)rawGyroY / GYRO_SCALE) - gyroBiasY; // Pitch rate
      float gz = ((float)rawGyroZ / GYRO_SCALE) - gyroBiasZ; // Yaw rate

      // Calculate absolute angles from Accelerometer gravity vectors (trigonometry)
      // Roll (rotation about X-axis) = atan2(Y, Z)
      float rollAccel = atan2(ay, az) * 180.0 / PI;
      
      // Pitch (rotation about Y-axis) = atan2(-X, sqrt(Y^2 + Z^2))
      float pitchAccel = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

      // Calculate un-fused gyroscope integrated roll (for plotter comparison, displays drift)
      gyroIntegratedRoll += gx * dt;

      // --- COMPLEMENTARY FILTER SENSOR FUSION ---
      // Roll Fusion
      fusedRoll = ALPHA * (fusedRoll + gx * dt) + (1.0 - ALPHA) * rollAccel;
      
      // Pitch Fusion
      fusedPitch = ALPHA * (fusedPitch + gy * dt) + (1.0 - ALPHA) * pitchAccel;

      // Output values formatted for the Serial Plotter
      // Comparing raw Accelerometer angle (noisy), pure Gyro angle (drifty), and Fused angle (ideal)
      Serial.print(rollAccel, 1);
      Serial.print(",");
      Serial.print(gyroIntegratedRoll, 1);
      Serial.print(",");
      Serial.println(fusedRoll, 1);
    }
  }
}

// --- I2C CONCURRENT BURST READING FUNCTIONS ---

bool readRawAccel(int16_t* ax, int16_t* ay, int16_t* az) {
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

bool readRawGyro(int16_t* gx, int16_t* gy, int16_t* gz) {
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

// --- GYROSCOPE BIAS CALIBRATION ---

void calibrateGyroscope() {
  Serial.println("[IMU] Calibrating Gyroscope. DO NOT MOVE SENSOR...");
  long sumX = 0, sumY = 0, sumZ = 0;
  int samples = 100; // Average over 100 samples
  
  for (int i = 0; i < samples; i++) {
    int16_t gx, gy, gz;
    if (readRawGyro(&gx, &gy, &gz)) {
      sumX += gx;
      sumY += gy;
      sumZ += gz;
    }
    delay(10); // 10ms delay between calibration steps
  }
  
  gyroBiasX = (float)(sumX / samples) / GYRO_SCALE;
  gyroBiasY = (float)(sumY / samples) / GYRO_SCALE;
  gyroBiasZ = (float)(sumZ / samples) / GYRO_SCALE;
  
  Serial.print("[IMU] Calibration complete. Offsets: ");
  Serial.print(gyroBiasX, 2);
  Serial.print(", ");
  Serial.print(gyroBiasY, 2);
  Serial.print(", ");
  Serial.println(gyroBiasZ, 2);
  delay(1500); // Wait briefly so the user can read the offsets before starting data plots
}

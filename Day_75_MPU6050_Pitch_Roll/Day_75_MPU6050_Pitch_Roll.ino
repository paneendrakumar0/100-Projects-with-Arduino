/*
 * 100 Projects with Arduino - Day 75
 * Project: MPU6050 Pitch & Roll Sensor Fusion (Complementary Filter via I2C Registers)
 *
 * DESCRIPTION:
 * This project implements a high-performance orientation estimator (Pitch and Roll) using the
 * MPU6050 6-Axis IMU. To align with professional aerospace and mechatronic control standards:
 * 1. 14-Byte Burst Read: Read Accelerometer, Temperature, and Gyroscope registers in a single,
 *    atomic I2C burst read transaction to guarantee temporal synchronization across all channels.
 * 2. Onboard Low-Pass Filtering (DLPF): Configures the MPU6050's hardware Digital Low Pass Filter
 *    to ~44 Hz to suppress mechanical and electrical noise before ADC conversion.
 * 3. Trigonometric Euler Angle Solver: Computes Pitch and Roll from Accelerometer gravity vectors
 *    using atan2 transcendental functions.
 * 4. Complementary Filter Sensor Fusion: Combines the high-frequency response of the gyroscope
 *    with the long-term stability of the accelerometer using a weight-matched filter:
 *      Angle = 0.96 * (Angle + Gyro_rate * dt) + 0.04 * Accel_angle
 *    This completely eliminates gyroscope linear drift and accelerometer vibration noise.
 * 5. Side-by-Side Comparison: Outputs Accel-only, Gyro-only, and Fused angles in the console
 *    to visually demonstrate the effect of sensor fusion.
 *
 * SENSOR FUSION THEORY:
 * - Accelerometer: Sensitive to vibrations and external acceleration (noisy in short-term), but
 *   always references the gravity vector (stable in long-term, no drift).
 * - Gyroscope: Immune to linear vibrations (clean in short-term), but integration of bias causes
 *   unbounded angle growth (drifts in long-term).
 * - Complementary Filter: Acts as a High-Pass filter for the gyroscope and a Low-Pass filter
 *   for the accelerometer, giving us the best of both worlds.
 *
 * WIRING:
 * - MPU6050 Pin -> Arduino Uno Pin
 *   - VCC        -> 5V
 *   - GND        -> GND
 *   - SCL        -> Pin A5 (SCL)
 *   - SDA        -> Pin A4 (SDA)
 *   - AD0        -> GND
 */

#include <Wire.h>

// --- MPU6050 I2C ADDRESS & REGISTERS ---
const uint8_t MPU6050_ADDR = 0x68;
const uint8_t REG_CONFIG = 0x1A;  // Configuration register (DLPF)
const uint8_t REG_GYRO_CONFIG = 0x1B;
const uint8_t REG_ACCEL_CONFIG = 0x1C;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;  // Start of data registers (14 bytes block)
const uint8_t REG_PWR_MGMT_1 = 0x6B;

// --- SENSITIVITY SCALE FACTORS ---
const float ACCEL_SCALE_FACTOR = 16384.0f;  // ±2g sensitivity
const float GYRO_SCALE_FACTOR = 131.0f;     // ±250°/s sensitivity
// const float RAD_TO_DEG = 180.0f / PI; // Already defined in Arduino.h

// --- TIMING CONFIGURATION ---
unsigned long lastLoopTime = 0;
unsigned long lastPrintTime = 0;
const unsigned long LOOP_PERIOD_US = 10000;  // 10ms (100 Hz sampling rate)

// --- SYSTEM STATE & CALIBRATION ---
float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;

// Angles tracked via Gyro integration only (will drift)
float gyroAngleX = 0, gyroAngleY = 0;

// Fused angles tracked via Complementary Filter (stable)
float fusedRoll = 0;   // Rotation around X-axis
float fusedPitch = 0;  // Rotation around Y-axis

// Filter Weight constant (alpha)
// Alpha = Tau / (Tau + dt), where Tau is the filter time constant (typically 0.2 - 0.5s)
// At 100 Hz, dt = 0.01. For Tau = 0.24s, Alpha = 0.96
const float ALPHA = 0.96f;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println(F("=================================================="));
  Serial.println(F("Day 75: MPU6050 Pitch & Roll Sensor Fusion"));
  Serial.println(F("=================================================="));

  if (!initMPU6050()) {
    Serial.println(F("[ERROR] Failed to communicate with MPU6050!"));
    while (1)
      ;
  }
  Serial.println(F("[SYSTEM] MPU6050 initialized. Calibrating gyro..."));

  // Calibrate Gyro (keep module flat and still)
  calibrateGyro();

  lastLoopTime = micros();
}

void loop() {
  unsigned long currentMicros = micros();

  // Non-blocking 100 Hz execution loop
  if (currentMicros - lastLoopTime >= LOOP_PERIOD_US) {
    float dt = (currentMicros - lastLoopTime) / 1000000.0f;
    lastLoopTime = currentMicros;

    int16_t rawAx, rawAy, rawAz, rawTemp, rawGx, rawGy, rawGz;
    if (readIMUData(rawAx, rawAy, rawAz, rawTemp, rawGx, rawGy, rawGz)) {
      // 1. Convert accelerometer raw to Gs
      float ax = rawAx / ACCEL_SCALE_FACTOR;
      float ay = rawAy / ACCEL_SCALE_FACTOR;
      float az = rawAz / ACCEL_SCALE_FACTOR;

      // 2. Subtract offsets and convert gyro raw to °/s
      float gx = (rawGx - gyroOffsetX) / GYRO_SCALE_FACTOR;
      float gy = (rawGy - gyroOffsetY) / GYRO_SCALE_FACTOR;
      // Note: gz (yaw rate) is not used for pitch/roll complementary filtering

      // 3. Compute Pitch & Roll directly from Accelerometer gravity vectors
      // Roll (rotation about X-axis): atan2(ay, az)
      float accRoll = atan2(ay, az) * RAD_TO_DEG;
      // Pitch (rotation about Y-axis): atan2(-ax, sqrt(ay^2 + az^2))
      float accPitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

      // 4. Integrate Gyro angular velocities only (for comparison)
      gyroAngleX += gx * dt;
      gyroAngleY += gy * dt;

      // 5. Sensor Fusion: Apply Complementary Filter
      // Fuses gyro integration (high-pass) and accel angle (low-pass)
      fusedRoll = ALPHA * (fusedRoll + gx * dt) + (1.0f - ALPHA) * accRoll;
      fusedPitch = ALPHA * (fusedPitch + gy * dt) + (1.0f - ALPHA) * accPitch;

      // 6. Output Telemetry at 10 Hz
      unsigned long currentMillis = millis();
      if (currentMillis - lastPrintTime >= 100) {
        lastPrintTime = currentMillis;

        Serial.print(F("ACCEL ONLY -> Roll: "));
        Serial.print(accRoll, 1);
        Serial.print(F("\tPitch: "));
        Serial.print(accPitch, 1);
        Serial.print(F(" | GYRO ONLY -> Roll: "));
        Serial.print(gyroAngleX, 1);
        Serial.print(F("\tPitch: "));
        Serial.print(gyroAngleY, 1);
        Serial.print(F(" | FUSED -> Roll: "));
        Serial.print(fusedRoll, 1);
        Serial.print(F("\tPitch: "));
        Serial.print(fusedPitch, 1);
        Serial.println();
      }
    }
  }
}

// =============================================================
//  LOW-LEVEL REGISTER COMMUNICATIONS & UTILITIES
// =============================================================

bool writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

bool initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75);  // WHO_AM_I
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available()) {
    Wire.read();
  }

  // Wake sensor
  if (!writeRegister(REG_PWR_MGMT_1, 0x00)) return false;
  delay(10);

  // Set Digital Low Pass Filter (DLPF) Configuration
  // Register 0x1A: Configures frame synchronization and DLPF.
  // Writing 0x03 sets DLPF to Mode 3:
  // Accelerometer: 44 Hz bandwidth, 4.8ms delay
  // Gyroscope:    42 Hz bandwidth, 4.8ms delay, 1 kHz internal sampling rate
  if (!writeRegister(REG_CONFIG, 0x03)) return false;

  // Set Gyroscope Full Scale Range to ±250°/s
  if (!writeRegister(REG_GYRO_CONFIG, 0x00)) return false;

  // Set Accelerometer Full Scale Range to ±2g
  if (!writeRegister(REG_ACCEL_CONFIG, 0x00)) return false;

  return true;
}

void calibrateGyro() {
  long sumX = 0, sumY = 0, sumZ = 0;
  const int samples = 200;

  Serial.println(F("[CALIBRATION] Standby... keep IMU still."));
  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, temp, gx, gy, gz;
    if (readIMUData(ax, ay, az, temp, gx, gy, gz)) {
      sumX += gx;
      sumY += gy;
      sumZ += gz;
    } else {
      i--;
    }
    delay(10);
  }

  gyroOffsetX = (float)sumX / samples;
  gyroOffsetY = (float)sumY / samples;
  gyroOffsetZ = (float)sumZ / samples;

  Serial.println(F("[CALIBRATION] Calibration complete."));
  Serial.print(F("Offsets -> Gx: "));
  Serial.print(gyroOffsetX, 1);
  Serial.print(F("\tGy: "));
  Serial.print(gyroOffsetY, 1);
  Serial.print(F("\tGz: "));
  Serial.println(gyroOffsetZ, 1);
  Serial.println(F("--------------------------------------------------"));
}

/**
 * Performs a 14-byte block burst read starting at 0x3B.
 * Extracts Accelerometer (6 bytes), Temperature (2 bytes), and Gyroscope (6 bytes).
 */
bool readIMUData(int16_t& ax, int16_t& ay, int16_t& az, int16_t& temp, int16_t& gx, int16_t& gy,
                 int16_t& gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);  // Point to start of burst register block (0x3B)
  if (Wire.endTransmission() != 0) return false;

  // Request 14 bytes from MPU6050
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)14);

  if (Wire.available() >= 14) {
    // Read accelerometer
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();
    az = (Wire.read() << 8) | Wire.read();

    // Read temperature
    temp = (Wire.read() << 8) | Wire.read();

    // Read gyroscope
    gx = (Wire.read() << 8) | Wire.read();
    gy = (Wire.read() << 8) | Wire.read();
    gz = (Wire.read() << 8) | Wire.read();
    return true;
  }

  return false;
}

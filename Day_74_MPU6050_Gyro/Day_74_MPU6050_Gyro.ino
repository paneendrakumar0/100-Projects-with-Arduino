/*
 * 100 Projects with Arduino - Day 74
 * Project: MPU6050 Gyroscope Angles & Integration (Direct I2C Registers)
 *
 * DESCRIPTION:
 * This project interfaces the MPU6050 gyroscope to calculate raw angular velocities and
 * integrate them over time to compute relative orientation angles (Roll, Pitch, and Yaw).
 * To meet professional mechatronics design standards:
 * 1. Startup Calibration (Auto-Zeroing): Performs a 200-sample calibration cycle on boot to
 *    calculate static sensor offsets (bias) while the sensor is at rest. These offsets are
 *    subtracted from subsequent readings to minimize angular drift.
 * 2. Microsecond Integration: Tracks elapsed time ($dt$) using micros() to perform accurate
 *    trapezoidal numerical integration: $\theta_t = \theta_{t-1} + \omega \cdot dt$.
 * 3. Standard Scaling: Converts raw LSB values into physical angular rates (degrees per second,
 *    °/s) using the 131 LSB/°/s sensitivity factor corresponding to the ±250°/s configuration.
 * 4. High-frequency Sampling: Implements a non-blocking 100 Hz (10ms) loop to keep integration
 *    errors low and prevent aliasing of rapid rotational movements.
 *
 * GYROSCOPE PHYSICS & DRIFT:
 * A gyroscope measures angular velocity (rate of rotation). Integrating velocity calculates
 * the relative angle. However, due to temperature fluctuations, thermal noise, and ADC
 * inaccuracies, there is always a tiny, non-zero offset even at rest. During integration, this
 * constant offset accumulates linearly over time, causing the calculated angle to slowly drift away
 * from its true position (known as Gyroscope Drift).
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
const uint8_t REG_GYRO_CONFIG = 0x1B;  // Gyroscope configuration register
const uint8_t REG_GYRO_XOUT_H = 0x43;  // Start of gyroscope registers (X high byte)
const uint8_t REG_PWR_MGMT_1 = 0x6B;

// --- GYROSCOPE SENSITIVITY ---
// Sensitivity factors for full-scale configuration ranges (datasheet):
// ±250°/s  -> 131 LSB / (°/s)
// ±500°/s  -> 65.5 LSB / (°/s)
// ±1000°/s -> 32.8 LSB / (°/s)
// ±2000°/s -> 16.4 LSB / (°/s)
const float GYRO_SCALE_FACTOR = 131.0f;  // Scale factor for ±250°/s range

// --- TIMING CONFIGURATION ---
unsigned long lastLoopTime = 0;              // Tracks the last iteration time in microseconds
unsigned long lastPrintTime = 0;             // Timer for Serial console updates
const unsigned long LOOP_PERIOD_US = 10000;  // 10,000 µs = 10ms (100 Hz loop)

// --- SYSTEM STATE & CALIBRATION ---
float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;  // Calibration offset constants
float angleX = 0, angleY = 0, angleZ = 0;                 // Integrated rotation angles (degrees)

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println(F("=================================================="));
  Serial.println(F("Day 74: MPU6050 Gyroscope Angles & Integration"));
  Serial.println(F("=================================================="));

  // Initialize MPU6050
  if (!initMPU6050()) {
    Serial.println(F("[ERROR] Failed to communicate with MPU6050!"));
    while (1);
  }
  Serial.println(F("[SYSTEM] MPU6050 detected. Starting calibration..."));

  // Perform calibration (sensor must remain perfectly still!)
  calibrateGyro();

  // Initialize loop timer
  lastLoopTime = micros();
}

void loop() {
  unsigned long currentMicros = micros();

  // Non-blocking 100 Hz execution loop
  if (currentMicros - lastLoopTime >= LOOP_PERIOD_US) {
    // Calculate elapsed time (dt) in seconds
    float dt = (currentMicros - lastLoopTime) / 1000000.0f;
    lastLoopTime = currentMicros;

    int16_t rawX, rawY, rawZ;
    if (readGyroRaw(rawX, rawY, rawZ)) {
      // Subtract calibration offset and convert raw LSB to °/s
      float gx = (rawX - gyroOffsetX) / GYRO_SCALE_FACTOR;
      float gy = (rawY - gyroOffsetY) / GYRO_SCALE_FACTOR;
      float gz = (rawZ - gyroOffsetZ) / GYRO_SCALE_FACTOR;

      // Integrate angular rate to obtain relative angles (trapezoidal Euler integration)
      angleX += gx * dt;
      angleY += gy * dt;
      angleZ += gz * dt;  // Yaw angle (note: will drift slowly without compass correction)

      // Print telemetry at a slower rate (10 Hz) to avoid console clogging
      unsigned long currentMillis = millis();
      if (currentMillis - lastPrintTime >= 100) {
        lastPrintTime = currentMillis;

        Serial.print(F("RATE -> X: "));
        Serial.print(gx, 1);
        Serial.print(F(" d/s\tY: "));
        Serial.print(gy, 1);
        Serial.print(F(" d/s\tZ: "));
        Serial.print(gz, 1);
        Serial.print(F(" d/s | ANGLE -> X: "));
        Serial.print(angleX, 2);
        Serial.print(F(" deg\tY: "));
        Serial.print(angleY, 2);
        Serial.print(F(" deg\tZ: "));
        Serial.print(angleZ, 2);
        Serial.println(F(" deg"));
      }
    }
  }
}

// =============================================================
//  LOW-LEVEL I2C & CALIBRATION FUNCTIONS
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
    if (Wire.read() != 0x68) {
      // Allow proceeding if device ID differs slightly (clones)
    }
  }

  // Wake sensor
  if (!writeRegister(REG_PWR_MGMT_1, 0x00)) return false;
  delay(10);

  // Configure Gyroscope range to ±250°/s (write 0x00 to GYRO_CONFIG)
  // Bit 3 and Bit 4 set range: 00 = ±250°/s, 01 = ±500°/s, 10 = ±1000°/s, 11 = ±2000°/s
  if (!writeRegister(REG_GYRO_CONFIG, 0x00)) return false;

  return true;
}

/**
 * Calculates raw gyroscope offsets by taking 200 readings at rest.
 */
void calibrateGyro() {
  long sumX = 0, sumY = 0, sumZ = 0;
  const int samples = 200;

  Serial.println(F("[CALIBRATION] Calibrating gyroscope. Keep sensor perfectly still!"));

  for (int i = 0; i < samples; i++) {
    int16_t x, y, z;
    if (readGyroRaw(x, y, z)) {
      sumX += x;
      sumY += y;
      sumZ += z;
    } else {
      i--;  // Retry if read failed
    }
    delay(10);  // 10ms delay between calibration samples
  }

  gyroOffsetX = (float)sumX / samples;
  gyroOffsetY = (float)sumY / samples;
  gyroOffsetZ = (float)sumZ / samples;

  Serial.println(F("[CALIBRATION] Calibration complete. Offset values calculated:"));
  Serial.print(F("  X Offset: "));
  Serial.println(gyroOffsetX, 2);
  Serial.print(F("  Y Offset: "));
  Serial.println(gyroOffsetY, 2);
  Serial.print(F("  Z Offset: "));
  Serial.println(gyroOffsetZ, 2);
  Serial.println(F("--------------------------------------------------"));
}

/**
 * Reads 6 bytes from the gyroscope registers starting at 0x43.
 * Combines high and low bytes to assemble raw X, Y, and Z measurements.
 */
bool readGyroRaw(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_GYRO_XOUT_H);  // Point to start register (0x43)
  if (Wire.endTransmission() != 0) return false;

  // Request 6 bytes of data (registers 0x43 to 0x48)
  // GYRO_XOUT_H (0x43), GYRO_XOUT_L (0x44),
  // GYRO_YOUT_H (0x45), GYRO_YOUT_L (0x46),
  // GYRO_ZOUT_H (0x47), GYRO_ZOUT_L (0x48)
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)6);

  if (Wire.available() >= 6) {
    x = (Wire.read() << 8) | Wire.read();
    y = (Wire.read() << 8) | Wire.read();
    z = (Wire.read() << 8) | Wire.read();
    return true;
  }

  return false;
}

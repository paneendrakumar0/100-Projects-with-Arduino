/*
 * 100 Projects with Arduino - Day 73
 * Project: MPU6050 3-Axis Accelerometer Raw Data Reader (Direct I2C Registers)
 *
 * DESCRIPTION:
 * This project interfaces the MFRC522/MPU6050 6-DOF Inertial Measurement Unit (IMU).
 * Rather than relying on external libraries, this sketch implements direct register-level I2C
 * communication using the standard Arduino Wire library.
 * 1. Sensor Wake-up & Configuration: Communicates with the MPU6050 (address 0x68), wakes it from
 *    sleep mode by writing to PWR_MGMT_1, and configures the Accelerometer to a full-scale range
 *    of ±2g.
 * 2. Synchronous Burst Read: Reads 6 bytes of accelerometer registers (X, Y, Z high/low bytes)
 *    in a single, atomic I2C transaction to prevent data-skew between axes.
 * 3. Scaling & Conversion: Combines high/low bytes into 16-bit signed integers and converts
 *    raw LSB readings into standard acceleration units of gravity (Gs) using the 16384 LSB/g scale
 * factor.
 * 4. Non-blocking Execution: Outputs data at a precise 10 Hz (100ms) sample rate using millis().
 *
 * MPU6050 & I2C REGISTERS:
 * - Address: 0x68 (Default when AD0 pin is connected to GND).
 * - PWR_MGMT_1 (Register 0x6B): Configures clock source and sleep state. Must write 0x00 to wake.
 * - ACCEL_CONFIG (Register 0x1C): Sets full-scale range (±2g, ±4g, ±8g, ±16g).
 * - ACCEL_XOUT_H (Register 0x3B): Start of the 6-byte block of accelerometer outputs.
 *
 * ACCELEROMETER PHYSICS:
 * An accelerometer measures proper acceleration (acceleration relative to free fall). When the
 * sensor sits flat and stationary on a desk:
 * - X and Y axes experience 0g of proper acceleration.
 * - Z axis experiences +1.0g pointing upwards (resisting gravity).
 *
 * WIRING:
 * - MPU6050 Pin -> Arduino Uno Pin
 *   - VCC        -> 5V (or 3.3V depending on GY-521 regulator version, 5V is standard on GY-521
 * boards)
 *   - GND        -> GND
 *   - SCL        -> Pin A5 (SCL)
 *   - SDA        -> Pin A4 (SDA)
 *   - AD0        -> GND (Sets I2C address to 0x68)
 */

#include <Wire.h>

// --- MPU6050 I2C ADDRESS & REGISTERS ---
const uint8_t MPU6050_ADDR = 0x68;      // Standard I2C address of MPU6050
const uint8_t REG_ACCEL_CONFIG = 0x1C;  // Accelerometer configuration register
const uint8_t REG_ACCEL_XOUT_H = 0x3B;  // Start of accelerometer registers (X high byte)
const uint8_t REG_PWR_MGMT_1 = 0x6B;    // Power management register 1

// --- ACCELEROMETER SENSITIVITY ---
// Sensitivity factors for full-scale configuration ranges (defined in datasheet):
// ±2g  -> 16384 LSB/g
// ±4g  -> 8192 LSB/g
// ±8g  -> 4096 LSB/g
// ±16g -> 2048 LSB/g
const float ACCEL_SCALE_FACTOR = 16384.0f;  // Scale factor for ±2g range

// --- TIMING CONFIGURATION ---
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL_MS = 100;  // Sample at 10 Hz (100ms)

void setup() {
  Serial.begin(9600);
  Wire.begin();  // Initialize I2C Bus as Master

  Serial.println(F("=================================================="));
  Serial.println(F("Day 73: MPU6050 Accelerometer Raw Data Reader"));
  Serial.println(F("=================================================="));

  // Initialize and configure MPU6050
  if (initMPU6050()) {
    Serial.println(F("[SYSTEM] MPU6050 detected and initialized."));
  } else {
    Serial.println(F("[ERROR] Failed to communicate with MPU6050!"));
    Serial.println(F("[SYSTEM] Freezing execution. Verify wiring."));
    while (1);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking 10 Hz sampling loop
  if (currentMillis - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = currentMillis;

    int16_t rawX, rawY, rawZ;
    if (readAccelRaw(rawX, rawY, rawZ)) {
      // Scale raw integers to Gs
      float ax = rawX / ACCEL_SCALE_FACTOR;
      float ay = rawY / ACCEL_SCALE_FACTOR;
      float az = rawZ / ACCEL_SCALE_FACTOR;

      // Print formatted output (Raw LSB values vs physical Gs)
      Serial.print(F("RAW -> X: "));
      Serial.print(rawX);
      Serial.print(F("\tY: "));
      Serial.print(rawY);
      Serial.print(F("\tZ: "));
      Serial.print(rawZ);
      Serial.print(F(" | ACCEL -> X: "));
      Serial.print(ax, 3);
      Serial.print(F("g\tY: "));
      Serial.print(ay, 3);
      Serial.print(F("g\tZ: "));
      Serial.print(az, 3);
      Serial.println(F("g"));
    } else {
      Serial.println(F("[ERROR] Failed to read sensor data!"));
    }
  }
}

// =============================================================
//  LOW-LEVEL I2C REGISTER COMMUNICATIONS
// =============================================================

/**
 * Sends a write instruction to a specific register on the MPU6050.
 */
bool writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

/**
 * Wakes up the MPU6050 and configures the accelerometer range.
 */
bool initMPU6050() {
  // Test connection by reading the WHO_AM_I register (0x75)
  // MPU6050 should respond with 0x68
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75);  // WHO_AM_I register
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available()) {
    uint8_t deviceID = Wire.read();
    if (deviceID != 0x68) {
      Serial.print(F("[WARNING] Device ID mismatch! Read: 0x"));
      Serial.println(deviceID, HEX);
      // Wait a moment and continue anyway, as some clones respond with 0x70 or other IDs
    }
  }

  // 1. Wake MPU6050 (write 0x00 to PWR_MGMT_1 register to disable sleep)
  if (!writeRegister(REG_PWR_MGMT_1, 0x00)) return false;
  delay(10);  // Wait for clock to stabilize

  // 2. Set Accelerometer Full Scale Range to ±2g (write 0x00 to ACCEL_CONFIG)
  // Bit 3 and Bit 4 of ACCEL_CONFIG set the range:
  // 00 = ±2g, 01 = ±4g, 10 = ±8g, 11 = ±16g
  if (!writeRegister(REG_ACCEL_CONFIG, 0x00)) return false;

  return true;
}

/**
 * Reads 6 bytes from the accelerometer registers starting at 0x3B.
 * Combines high and low bytes to assemble raw X, Y, and Z measurements.
 */
bool readAccelRaw(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);  // Point to start register (0x3B)
  if (Wire.endTransmission() != 0) return false;

  // Request 6 bytes of data (registers 0x3B to 0x40)
  // ACCEL_XOUT_H (0x3B), ACCEL_XOUT_L (0x3C),
  // ACCEL_YOUT_H (0x3D), ACCEL_YOUT_L (0x3E),
  // ACCEL_ZOUT_H (0x3F), ACCEL_ZOUT_L (0x40)
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)6);

  if (Wire.available() >= 6) {
    // Read high and low bytes, then combine them using bitwise shifts
    x = (Wire.read() << 8) | Wire.read();
    y = (Wire.read() << 8) | Wire.read();
    z = (Wire.read() << 8) | Wire.read();
    return true;
  }

  return false;
}

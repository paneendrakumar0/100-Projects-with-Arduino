/*
 * 100 Projects with Arduino - Day 41
 * Project: MPU6050 6-Axis IMU (Direct I2C Register Reading)
 * 
 * DESCRIPTION:
 * This project interfaces the MFR6050 / MPU6050 6-DOF (Degrees of Freedom) Inertial Measurement Unit (IMU).
 * To achieve professional-grade, register-level mechatronics engineering insights:
 * 1. Bypasses external libraries: Uses raw I2C transactions (`Wire.h`) to read and write directly
 *    to configuration and sensor data registers.
 * 2. Device Power Management: Writes to the PWR_MGMT_1 register (0x6B) to wake up the IMU from sleep.
 * 3. Multi-byte Burst Reads: Executes high-efficiency 6-byte block reads to retrieve X, Y, and Z axes
 *    simultaneously for the accelerometer and gyroscope, ensuring data synchronicity.
 * 4. Sensitivity Scaling: Converts raw signed 16-bit two's complement values into standardized units
 *    (gravity accelerations 'g' and angular rates '°/s') based on default datasheet scaling factors.
 * 
 * MPU6050 HARDWARE & ACCURACY THEORY:
 * - Accelerometers: Use microscopic MEMS (Micro-Electro-Mechanical Systems) silicon structures suspended
 *   by springs. Acceleration causes the structures to deflect, changing electrical capacitance which is converted
 *   by a 16-bit ADC. Default scale is ±2g (sensitivity 16384 LSB/g).
 * - Gyroscopes: Use vibrating silicon rings. When rotated, the Coriolis effect exerts a force on the ring,
 *   generating capacitance shifts. Default scale is ±250 °/s (sensitivity 131 LSB/(°/s)).
 * - Drift and Bias: Gyroscopes suffer from "zero-rate drift" (outputting non-zero values even at standstill).
 *   We implement a simple calibration routine on setup to record and subtract this stationary offset bias.
 * 
 * WIRING:
 * - MPU6050 Module -> Arduino Uno
 *   - VCC -> 5V (Note: Most MPU6050 breakout boards have an onboard 3.3V regulator; check if yours requires 3.3V)
 *   - GND -> GND
 *   - SDA -> Pin A4 (SDA)
 *   - SCL -> Pin A5 (SCL)
 *   - AD0 -> Not Connected (Selects default I2C address 0x68. Pulling to VCC changes address to 0x69)
 */

#include <Wire.h>

// --- MPU6050 I2C ADDRESS & REGISTER MAP ---
const int MPU_ADDRESS = 0x68;
const int REG_PWR_MGMT_1 = 0x6B;
const int REG_ACCEL_XOUT_H = 0x3B; // Accelerometer registers start here (0x3B to 0x40)
const int REG_TEMP_OUT_H = 0x41;  // Temperature registers start here (0x41 to 0x42)
const int REG_GYRO_XOUT_H = 0x43;  // Gyroscope registers start here (0x43 to 0x48)

// --- SENSITIVITY SCALE FACTORS (From MPU6050 Datasheet) ---
const float ACCEL_SCALE = 16384.0; // 16384 LSB per g (for default ±2g range)
const float GYRO_SCALE  = 131.0;   // 131 LSB per °/s (for default ±250°/s range)

// --- CALIBRATION BIAS ACCUMULATORS ---
float gyroBiasX = 0.0, gyroBiasY = 0.0, gyroBiasZ = 0.0;

// --- TIMING VARIABLES ---
unsigned long lastSensorReadTime = 0;
const unsigned long sensorReadIntervalMs = 200; // Read sensors at 5 Hz (every 200ms)

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize I2C Bus as Master

  Serial.println("==================================================");
  Serial.println("Day 41: MPU6050 6-Axis IMU Direct I2C Register Read");
  Serial.println("==================================================");

  // Wake up the MPU6050.
  // By default, the MPU6050 boots up in sleep mode (bit 6 of PWR_MGMT_1 register is set to 1).
  // Writing 0 to register 0x6B clears the sleep bit and activates the internal oscillators.
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00); // Set power management register to 0
  if (Wire.endTransmission() != 0) {
    Serial.println("[ERROR] Failed to communicate with MPU6050 over I2C! Check wiring.");
    for (;;);
  }
  
  Serial.println("[IMU] Woken up from sleep mode.");
  
  // Calibrate Gyroscope offsets (requires the sensor to remain perfectly stationary)
  calibrateGyroscope();
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking loop for sensor acquisition
  if (currentMillis - lastSensorReadTime >= sensorReadIntervalMs) {
    lastSensorReadTime = currentMillis;

    // Data containers for raw readings
    int16_t rawAccelX, rawAccelY, rawAccelZ;
    int16_t rawGyroX, rawGyroY, rawGyroZ;
    int16_t rawTemp;

    // Read registers
    if (readRawAccel(&rawAccelX, &rawAccelY, &rawAccelZ) &&
        readRawGyro(&rawGyroX, &rawGyroY, &rawGyroZ) &&
        readRawTemp(&rawTemp)) {

      // Convert raw 16-bit integers to physical float units
      float ax = (float)rawAccelX / ACCEL_SCALE;
      float ay = (float)rawAccelY / ACCEL_SCALE;
      float az = (float)rawAccelZ / ACCEL_SCALE;

      // Convert raw gyro and subtract stationary calibration biases
      float gx = ((float)rawGyroX / GYRO_SCALE) - gyroBiasX;
      float gy = ((float)rawGyroY / GYRO_SCALE) - gyroBiasY;
      float gz = ((float)rawGyroZ / GYRO_SCALE) - gyroBiasZ;

      // Convert raw temperature to Celsius using MPU6050 datasheet formula:
      // Temp in °C = (raw_value / 340.0) + 36.53
      float tempCelsius = ((float)rawTemp / 340.0) + 36.53;

      // Print telemetry
      Serial.print("[IMU] Accel (g): X=");
      Serial.print(ax, 2);
      Serial.print(" Y=");
      Serial.print(ay, 2);
      Serial.print(" Z=");
      Serial.print(az, 2);
      
      Serial.print(" | Gyro (deg/s): X=");
      Serial.print(gx, 1);
      Serial.print(" Y=");
      Serial.print(gy, 1);
      Serial.print(" Z=");
      Serial.print(gz, 1);

      Serial.print(" | Temp: ");
      Serial.print(tempCelsius, 1);
      Serial.println(" C");

    } else {
      Serial.println("[WARNING] I2C read error occurred.");
    }
  }
}

// --- DIRECT I2C READING UTILITIES ---

/**
 * Reads 6 bytes starting from register 0x3B (Accel X Out H) to capture all 3 axes.
 */
bool readRawAccel(int16_t* ax, int16_t* ay, int16_t* az) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_ACCEL_XOUT_H); // Point to starting register
  if (Wire.endTransmission(false) != 0) return false; // End transmission with a restart command
  
  Wire.requestFrom(MPU_ADDRESS, 6); // Request 6 bytes (2 bytes per axis)
  if (Wire.available() >= 6) {
    // Join high and low bytes to reconstruct signed 16-bit integer
    *ax = (Wire.read() << 8) | Wire.read();
    *ay = (Wire.read() << 8) | Wire.read();
    *az = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

/**
 * Reads 6 bytes starting from register 0x43 (Gyro X Out H) to capture all 3 axes.
 */
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

/**
 * Reads 2 bytes starting from register 0x41 (Temp Out H).
 */
bool readRawTemp(int16_t* temp) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_TEMP_OUT_H);
  if (Wire.endTransmission(false) != 0) return false;
  
  Wire.requestFrom(MPU_ADDRESS, 2);
  if (Wire.available() >= 2) {
    *temp = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

// --- CALIBRATION ROUTINE ---

/**
 * Sweeps gyroscope outputs over 50 samples while stationary to calculate bias offsets.
 */
void calibrateGyroscope() {
  Serial.println("[IMU] Calibrating Gyroscope. DO NOT MOVE SENSOR...");
  
  long sumX = 0, sumY = 0, sumZ = 0;
  int samples = 50;
  
  for (int i = 0; i < samples; i++) {
    int16_t gx, gy, gz;
    if (readRawGyro(&gx, &gy, &gz)) {
      sumX += gx;
      sumY += gy;
      sumZ += gz;
    }
    delay(20); // 20ms delay between calibration samples
  }
  
  // Calculate average bias offsets in degrees per second
  gyroBiasX = (float)(sumX / samples) / GYRO_SCALE;
  gyroBiasY = (float)(sumY / samples) / GYRO_SCALE;
  gyroBiasZ = (float)(sumZ / samples) / GYRO_SCALE;
  
  Serial.print("[IMU] Calibration complete. Offset Biases -> X: ");
  Serial.print(gyroBiasX, 2);
  Serial.print(" Y: ");
  Serial.print(gyroBiasY, 2);
  Serial.print(" Z: ");
  Serial.println(gyroBiasZ, 2);
}

/*
 * 100 Projects with Arduino - Day 86
 * Project: Persistent Sensor Calibration Manager (MPU6050 Boot-trigger Calibration & EEPROM Storage)
 * 
 * DESCRIPTION:
 * This project implements a production-grade Sensor Calibration Manager for the MPU6050 IMU.
 * To meet industrial robotics and aerospace control standards:
 * 1. Factory/Boot-Trigger Calibration: Instead of forcing a 2-second calibration on every boot
 *    (which requires the robot to be perfectly still every time it powers up), the system loads 
 *    pre-calculated calibration offsets from the EEPROM. If the user holds down a physical button
 *    on Pin 2 during boot, it forces a fresh 500-sample calibration cycle and writes the offsets to EEPROM.
 * 2. 6-Axis Offset Struct: Stores calibration offsets for all 6 degrees of freedom (X, Y, Z accelerometer
 *    offsets and X, Y, Z gyroscope offsets) in a structured EEPROM block.
 * 3. Validation Checksums: Protects calibration data using a magic configuration byte (0xC4) and
 *    a block checksum to prevent reading corrupt or uninitialized memory.
 * 4. Calibration CLI: Allows triggering a calibration routine and listing active parameters via 
 *    Serial commands.
 * 
 * THE PHYSICS of IMU BIAS:
 * Accelerometer and Gyroscope sensors suffer from static bias (offset) caused by silicon manufacturing
 * tolerances, mechanical stress from soldering, and temperature.
 * - Gyroscope Bias: Outputs a non-zero rotation speed at rest (e.g. +0.2 deg/sec).
 * - Accelerometer Bias: Outputs acceleration offsets (e.g. +0.05g) even when sitting flat.
 * Calibration calculates these offsets by averaging readings at rest, and subtracts them from all
 * future measurements:
 * $$\text{Reading}_{\text{calibrated}} = \text{Reading}_{\text{raw}} - \text{Offset}$$
 * 
 * WIRING:
 * - MPU6050 Pin -> Arduino Uno Pin
 *   - VCC -> 5V | GND -> GND | SDA -> A4 | SCL -> A5
 * - Calibration Trigger Button -> Connect Pin 2 to GND (uses internal INPUT_PULLUP)
 * - Calibrating LED Indicator -> Pin 13 (Built-in LED, stays solid during calibration)
 */

#include <Wire.h>
#include <EEPROM.h>

// --- MPU6050 REGISTERS ---
const uint8_t MPU6050_ADDR      = 0x68;
const uint8_t REG_ACCEL_XOUT_H  = 0x3B;
const uint8_t REG_PWR_MGMT_1    = 0x6B;
const float ACCEL_SCALE_FACTOR = 16384.0f; // ±2g sensitivity
const float GYRO_SCALE_FACTOR  = 131.0f;   // ±250°/s sensitivity

// --- PIN CONFIGURATION ---
const int CAL_BUTTON_PIN = 2; // Press during boot to force calibration
const int CAL_LED_PIN    = 13; // Onboard LED, active during calibration

// --- STRUCT FOR PERSISTENT CALIBRATION DATA ---
struct CalibrationData {
  byte magicByte;          // 0xC4 = valid calibration data stored
  float accelOffsetX;
  float accelOffsetY;
  float accelOffsetZ;      // Accel Z should offset to 1.0g at rest
  float gyroOffsetX;
  float gyroOffsetY;
  float gyroOffsetZ;
  uint16_t checksum;       // Data validation checksum
};

const int EEPROM_CAL_ADDR = 0;
CalibrationData calData;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CAL_LED_PIN, OUTPUT);
  digitalWrite(CAL_LED_PIN, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 86: Persistent Sensor Calibration Manager"));
  Serial.println(F("=================================================="));

  if (!initMPU6050()) {
    Serial.println(F("[ERROR] Failed to communicate with MPU6050!"));
    while (1);
  }

  // Check if user is pressing the button during boot to force calibration
  if (digitalRead(CAL_BUTTON_PIN) == LOW) {
    Serial.println(F("[SYSTEM] Forced Calibration Trigger detected on Pin 2!"));
    runSelfCalibration();
  } else {
    // Attempt to load offsets from EEPROM
    if (loadCalibration()) {
      Serial.println(F("[SYSTEM] Pre-stored offsets loaded successfully. Ready."));
    } else {
      Serial.println(F("[WARNING] No valid calibration data found in EEPROM."));
      Serial.println(F("[WARNING] Sensor readings will use 0.0 default offsets."));
      Serial.println(F("[SYSTEM] Press button on Pin 2 during boot, or send 'c' to calibrate."));
    }
  }

  printOffsets(calData);
  printMenu();
}

void loop() {
  // Read and print calibrated sensor values at 10 Hz
  static unsigned long lastSample = 0;
  if (millis() - lastSample >= 100) {
    lastSample = millis();
    readAndPrintCalibratedData();
  }

  // Poll CLI serial commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'c':
      case 'C':
        runSelfCalibration();
        break;
      case 's':
      case 'S':
        printOffsets(calData);
        break;
      case 'd':
      case 'D':
        invalidateEEPROM();
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

// =============================================================
//  CORE CALIBRATION & STORAGE LOGIC
// =============================================================

/**
 * Calculates the checksum of the calibration structure (excluding checksum field).
 */
uint16_t calculateCalChecksum(const CalibrationData& data) {
  uint16_t sum = 0;
  const byte* bytePtr = (const byte*)&data;
  int size = sizeof(CalibrationData) - sizeof(data.checksum);
  
  for (int i = 0; i < size; i++) {
    sum += bytePtr[i];
  }
  return sum;
}

/**
 * Loads calibration struct from EEPROM. Returns true if valid.
 */
bool loadCalibration() {
  EEPROM.get(EEPROM_CAL_ADDR, calData);
  
  uint16_t check = calculateCalChecksum(calData);
  if (calData.magicByte == 0xC4 && calData.checksum == check) {
    return true; // Valid configuration
  }
  
  // Set default zero offsets if invalid
  calData.magicByte = 0x00;
  calData.accelOffsetX = 0;
  calData.accelOffsetY = 0;
  calData.accelOffsetZ = 0;
  calData.gyroOffsetX = 0;
  calData.gyroOffsetY = 0;
  calData.gyroOffsetZ = 0;
  return false;
}

/**
 * Computes offsets by averaging 500 stationary samples and saves to EEPROM.
 */
void runSelfCalibration() {
  Serial.println(F("\n[CALIBRATION] Starting auto-calibration. Keep IMU flat and still!"));
  digitalWrite(CAL_LED_PIN, HIGH); // Turn on indicator LED
  delay(1000); // Allow transient vibrations to settle

  long sumAx = 0, sumAy = 0, sumAz = 0;
  long sumGx = 0, sumGy = 0, sumGz = 0;
  const int samples = 500;

  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, temp, gx, gy, gz;
    if (readIMU(ax, ay, az, temp, gx, gy, gz)) {
      sumAx += ax;
      sumAy += ay;
      sumAz += az;
      sumGx += gx;
      sumGy += gy;
      sumGz += gz;
    } else {
      i--; // Retry read
    }
    delay(4); // 250 Hz sample rate
  }

  // Calculate average offsets
  calData.magicByte = 0xC4;
  calData.accelOffsetX = (float)sumAx / samples;
  calData.accelOffsetY = (float)sumAy / samples;
  // Accel Z should read exactly 1.0g (16384 LSB) at rest.
  // Offset is the difference between average Z and the target 1.0g vector.
  calData.accelOffsetZ = ((float)sumAz / samples) - 16384.0f;

  calData.gyroOffsetX = (float)sumGx / samples;
  calData.gyroOffsetY = (float)sumGy / samples;
  calData.gyroOffsetZ = (float)sumGz / samples;

  // Calculate checksum and save to EEPROM
  calData.checksum = calculateCalChecksum(calData);
  EEPROM.put(EEPROM_CAL_ADDR, calData);

  digitalWrite(CAL_LED_PIN, LOW); // Turn off indicator LED
  Serial.println(F("[CALIBRATION] Auto-calibration completed. Offsets saved to EEPROM."));
}

/**
 * Wipes the calibration signature from EEPROM to simulate uncalibrated board.
 */
void invalidateEEPROM() {
  Serial.println(F("[SYSTEM] Invalidating calibration data in EEPROM..."));
  EEPROM.write(EEPROM_CAL_ADDR, 0x00); // Overwrite magic byte
  loadCalibration(); // Reload default parameters
}

// =============================================================
//  LOW-LEVEL I2C OPERATIONS
// =============================================================
bool initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75); // WHO_AM_I
  if (Wire.endTransmission() != 0) return false;
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available() && Wire.read() != 0x68) return false;

  // Wake
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  return (Wire.endTransmission() == 0);
}

bool readIMU(int16_t& ax, int16_t& ay, int16_t& az, 
             int16_t& temp, 
             int16_t& gx, int16_t& gy, int16_t& gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)14);
  if (Wire.available() >= 14) {
    ax   = (Wire.read() << 8) | Wire.read();
    ay   = (Wire.read() << 8) | Wire.read();
    az   = (Wire.read() << 8) | Wire.read();
    temp = (Wire.read() << 8) | Wire.read();
    gx   = (Wire.read() << 8) | Wire.read();
    gy   = (Wire.read() << 8) | Wire.read();
    gz   = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

void readAndPrintCalibratedData() {
  int16_t ax, ay, az, temp, gx, gy, gz;
  if (readIMU(ax, ay, az, temp, gx, gy, gz)) {
    // Apply calibration offsets (subtract bias) and convert to units
    float axCal = (ax - calData.accelOffsetX) / ACCEL_SCALE_FACTOR;
    float ayCal = (ay - calData.accelOffsetY) / ACCEL_SCALE_FACTOR;
    float azCal = (az - calData.accelOffsetZ) / ACCEL_SCALE_FACTOR;

    float gxCal = (gx - calData.gyroOffsetX) / GYRO_SCALE_FACTOR;
    float gyCal = (gy - calData.gyroOffsetY) / GYRO_SCALE_FACTOR;
    float gzCal = (gz - calData.gyroOffsetZ) / GYRO_SCALE_FACTOR;

    // Print calibrated readings
    Serial.print(F("ACCEL [g] -> X: "));  Serial.print(axCal, 3);
    Serial.print(F("\tY: "));             Serial.print(ayCal, 3);
    Serial.print(F("\tZ: "));             Serial.print(azCal, 3);
    Serial.print(F(" | GYRO [d/s] -> X: ")); Serial.print(gxCal, 1);
    Serial.print(F("\tY: "));             Serial.print(gyCal, 1);
    Serial.print(F("\tZ: "));             Serial.println(gzCal, 1);
  }
}

// =============================================================
//  TELEMETRY & HELP UTILITIES
// =============================================================
void printOffsets(const CalibrationData& data) {
  Serial.println(F("----------------- CALIBRATION OFFSETS -----------------"));
  Serial.print(F(" Status: ")); Serial.println(data.magicByte == 0xC4 ? F("ACTIVE (Offsets Applied)") : F("DEFAULT (No offsets)"));
  Serial.print(F(" Accel Offsets (LSB) -> X: ")); Serial.print(data.accelOffsetX, 1);
  Serial.print(F("\tY: ")); Serial.print(data.accelOffsetY, 1);
  Serial.print(F("\tZ: ")); Serial.println(data.accelOffsetZ, 1);
  Serial.print(F(" Gyro Offsets (LSB)  -> X: ")); Serial.print(data.gyroOffsetX, 1);
  Serial.print(F("\tY: ")); Serial.print(data.gyroOffsetY, 1);
  Serial.print(F("\tZ: ")); Serial.println(data.gyroOffsetZ, 1);
  Serial.print(F(" Config Checksum     -> 0x")); Serial.println(data.checksum, HEX);
  Serial.println(F("-------------------------------------------------------"));
}

void printMenu() {
  Serial.println(F("\n--- PERSISTENT CALIBRATION CLI ---"));
  Serial.println(F(" 'c' : Trigger manual 500-sample calibration loop"));
  Serial.println(F(" 's' : Print calibration parameters in active memory"));
  Serial.println(F(" 'd' : Reset calibration values (invalidate EEPROM magic)"));
  Serial.println(F(" 'h' : Display this help command list"));
  Serial.println(F("-----------------------------------"));
}

/*
 * 100 Projects with Arduino - Day 50
 * Project: Digital Compass Heading Lock (QMC5883L I2C Magnetometer Navigation)
 *
 * DESCRIPTION:
 * This project interfaces a QMC5883L 3-axis digital magnetometer over I2C (using Wire.h
 * direct register addressing) to implement an absolute Heading-Lock navigation controller
 * for a 2WD differential robot chassis.
 *
 * CONTROL SYSTEMS & SIGNAL PROCESSING:
 * 1. Direct I2C Register Pipeline: Configures the QMC5883L at 200 Hz Output Data Rate (ODR),
 *    2 Gauss range, and 512 Over-Sampling Ratio (OSR) by writing to registers 0x09 and 0x0A.
 * 2. Hard-Iron Bias Calibration: Sweeps the sensor in a 360-degree circle during setup to record
 *    the maximum and minimum raw field vectors, calculating offset biases to eliminate magnetic
 * distortions.
 * 3. Heading Angle Formulation: Computes relative magnetic heading via atan2(y, x), adjusts
 *    for local Geographic Declination, and normalizes output bounds to 0 - 360 degrees.
 * 4. Closed-Loop Heading-Lock Steering: Runs a Proportional control loop to calculate differential
 *    motor speed adjustments based on Heading Error, allowing the robot to travel along a precise
 * absolute bearing.
 *
 * PHYSICS & NAVIGATION MATHEMATICS:
 * - Magnetic Declination: Adjusts the heading from Magnetic North to True (Geographic) North:
 *     True Heading = Magnetic Heading + Declination Angle
 *     (Declination varies by location. E.g., London is +0.022 rad / +1.28°, Tokyo is -0.138 rad /
 * -7.9°).
 * - Hard-Iron Correction:
 *     X_offset = (X_max + X_min) / 2;  Y_offset = (Y_max + Y_min) / 2;
 *     X_calibrated = X_raw - X_offset; Y_calibrated = Y_raw - Y_offset;
 * - Angular Error Shortest-Path Normalization:
 *     Error = Target_Heading - Current_Heading
 *     If Error > 180, Error = Error - 360; If Error < -180, Error = Error + 360;
 *
 * WIRING:
 * - QMC5883L Magnetometer (GY-271) -> Arduino Uno
 *   - VCC -> 5V (or 3.3V depending on module)
 *   - GND -> GND
 *   - SDA -> Pin A4 (I2C SDA)
 *   - SCL -> Pin A5 (I2C SCL)
 * - L298N Motor Driver -> Arduino Uno
 *   - ENA (Left PWM)   -> Pin 5
 *   - IN1, IN2         -> Pins 4, 3
 *   - ENB (Right PWM)  -> Pin 6
 *   - IN3, IN4         -> Pins 7, 8
 */

#include <Wire.h>

// --- QMC5883L I2C REGISTER DEFS ---
const int QMC_ADDRESS = 0x0D;
const int REG_DATA_X_LSB = 0x00;
const int REG_CONTROL_1 = 0x09;
const int REG_CONTROL_2 = 0x0A;

// --- MOTOR CONTROLLER PIN DEFINITIONS ---
const int L_ENA_PIN = 5;
const int L_IN1_PIN = 4;
const int L_IN2_PIN = 3;

const int R_ENB_PIN = 6;
const int R_IN3_PIN = 7;
const int R_IN4_PIN = 8;

const int LED_INDICATOR_PIN = 13;

// --- GEOGRAPHIC CALIBRATION CONFIG ---
// Find your local declination at: http://www.magnetic-declination.com/
// Example declination: +2 degrees 30 minutes East -> +2.5 degrees -> (2.5 * PI / 180) = +0.0436
// radians
const float LOCAL_DECLINATION_RAD = 0.0436;

// --- HARD-IRON CALIBRATION BIASES ---
float xOffset = 0.0;
float yOffset = 0.0;

// --- NAVIGATION STATE VARIABLES ---
double targetHeading = 90.0;  // Target heading in degrees (90.0 = True East)
double currentHeading = 0.0;

// --- STEERING CONTROL PARAMETERS ---
const double Kp = 2.2;  // Proportional feedback steering aggressiveness
const int BASE_CRUISE_SPEED = 120;

// --- TIMING CYCLE CONFIG ---
unsigned long lastLoopTimeUs = 0;
const unsigned long samplePeriodUs = 20000;  // 20ms sample period = 50 Hz control loop

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(LED_INDICATOR_PIN, OUTPUT);
  pinMode(L_ENA_PIN, OUTPUT);
  pinMode(L_IN1_PIN, OUTPUT);
  pinMode(L_IN2_PIN, OUTPUT);
  pinMode(R_ENB_PIN, OUTPUT);
  pinMode(R_IN3_PIN, OUTPUT);
  pinMode(R_IN4_PIN, OUTPUT);

  haltRobot();

  // Initialize QMC5883L
  initMagnetometer();

  // Sweep robot in a circle for 5 seconds to calibrate X & Y biases
  runCompassCalibration();

  Serial.print(F("[NAV] Target Heading: "));
  Serial.print(targetHeading);
  Serial.println(F("° (True North reference)"));

  lastLoopTimeUs = micros();
}

void loop() {
  unsigned long currentTimeUs = micros();

  // Strict 50 Hz control loop
  if (currentTimeUs - lastLoopTimeUs >= samplePeriodUs) {
    lastLoopTimeUs = currentTimeUs;

    int16_t rawX, rawY, rawZ;

    if (readRawData(&rawX, &rawY, &rawZ)) {
      // Step 1: Apply Hard-Iron offset adjustments
      float calX = (float)rawX - xOffset;
      float calY = (float)rawY - yOffset;

      // Step 2: Compute heading angle in horizontal plane (radians)
      float heading = atan2(calY, calX);

      // Step 3: Correct for local geographic declination
      heading += LOCAL_DECLINATION_RAD;

      // Step 4: Normalize angle to [0, 2*PI] range
      if (heading < 0) {
        heading += 2 * PI;
      }
      if (heading > 2 * PI) {
        heading -= 2 * PI;
      }

      // Step 5: Convert to degrees
      currentHeading = heading * 180.0 / PI;

      // Step 6: Compute Heading Error (Shortest path rotation)
      double error = targetHeading - currentHeading;

      // Keep error within [-180, 180] degrees
      if (error > 180.0) {
        error -= 360.0;
      } else if (error < -180.0) {
        error += 360.0;
      }

      // Step 7: Closed-Loop Heading Correction (Proportional mixing)
      double correction = Kp * error;

      // Adjust motor PWM to steer toward the lock direction
      int leftPWM = BASE_CRUISE_SPEED - (int)correction;
      int rightPWM = BASE_CRUISE_SPEED + (int)correction;

      driveMotors(leftPWM, rightPWM);

      // Serial diagnostics: Target, Current Heading, Correction PWM
      Serial.print(targetHeading, 1);
      Serial.print(",");
      Serial.print(currentHeading, 1);
      Serial.print(",");
      Serial.println(correction);
    }
  }
}

// --- HARDWARE COMMUNICATIONS PIPELINE ---

void initMagnetometer() {
  // Reset QMC5883L chip
  Wire.beginTransmission(QMC_ADDRESS);
  Wire.write(REG_CONTROL_2);
  Wire.write(0x80);  // Software reset
  Wire.endTransmission();
  delay(100);

  // Set Control Register 1: OSR=512, RNG=2G, ODR=200Hz, MODE=Continuous
  // Bits: OSR(7-6)=00, RNG(5-4)=00, ODR(3-2)=11 (200Hz), MODE(1-0)=01 (Continuous) -> 0x0D or 0x1D
  // Let's use OSR=512 (00), RNG=2G (00), ODR=200Hz (11), MODE=Continuous (01) -> 00001101 = 0x0D
  Wire.beginTransmission(QMC_ADDRESS);
  Wire.write(REG_CONTROL_1);
  Wire.write(0x0D);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("[ERROR] QMC5883L communication failed! Halt."));
    digitalWrite(LED_INDICATOR_PIN, HIGH);
    for (;;);
  }
  Serial.println(F("[COMPASS] QMC5883L configured successfully."));
}

bool readRawData(int16_t *x, int16_t *y, int16_t *z) {
  // Start register pointer burst read
  Wire.beginTransmission(QMC_ADDRESS);
  Wire.write(REG_DATA_X_LSB);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(QMC_ADDRESS, 6);
  if (Wire.available() >= 6) {
    // Read 6 bytes of data: X LSB/MSB, Y LSB/MSB, Z LSB/MSB
    *x = (int16_t)(Wire.read() | (Wire.read() << 8));
    *y = (int16_t)(Wire.read() | (Wire.read() << 8));
    *z = (int16_t)(Wire.read() | (Wire.read() << 8));
    return true;
  }
  return false;
}

// --- ROTATIONAL HARD-IRON BIAS CALIBRATION ---

void runCompassCalibration() {
  Serial.println(F("[CALIBRATION] Starting. Spin the robot in circles on a flat surface!"));
  digitalWrite(LED_INDICATOR_PIN, HIGH);

  long calTimerStart = millis();
  int pivotDir = 1;

  int16_t xMin = 32767, xMax = -32768;
  int16_t yMin = 32767, yMax = -32768;

  // Pivot slowly in place to register minimums/maximums
  drivePivot(pivotDir, 110);

  unsigned long lastToggle = millis();

  while (millis() - calTimerStart < 6000) {
    // Periodically reverse direction to keep layout centered
    if (millis() - lastToggle >= 1500) {
      lastToggle = millis();
      pivotDir = -pivotDir;
      drivePivot(pivotDir, 110);
    }

    int16_t rx, ry, rz;
    if (readRawData(&rx, &ry, &rz)) {
      if (rx < xMin) xMin = rx;
      if (rx > xMax) xMax = rx;
      if (ry < yMin) yMin = ry;
      if (ry > yMax) yMax = ry;
    }
    delay(10);
  }

  haltRobot();
  digitalWrite(LED_INDICATOR_PIN, LOW);

  // Calculate midpoints (Hard-Iron offset biases)
  xOffset = (float)(xMax + xMin) / 2.0;
  yOffset = (float)(yMax + yMin) / 2.0;

  Serial.println(F("[CALIBRATION] Complete. Offsets established:"));
  Serial.print(F("  X Offset: "));
  Serial.println(xOffset);
  Serial.print(F("  Y Offset: "));
  Serial.println(yOffset);
  delay(1500);
}

// --- DIRECT MOTOR DRIVERS ---

void driveMotors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  if (leftSpeed >= 0) {
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, leftSpeed);
  } else {
    digitalWrite(L_IN1_PIN, LOW);
    digitalWrite(L_IN2_PIN, HIGH);
    analogWrite(L_ENA_PIN, -leftSpeed);
  }

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

void drivePivot(int dir, int speed) {
  if (dir == 1) {  // Pivot Right
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, speed);
    digitalWrite(R_IN3_PIN, LOW);
    digitalWrite(R_IN4_PIN, HIGH);
    analogWrite(R_ENB_PIN, speed);
  } else {  // Pivot Left
    digitalWrite(L_IN1_PIN, LOW);
    digitalWrite(L_IN2_PIN, HIGH);
    analogWrite(L_ENA_PIN, speed);
    digitalWrite(R_IN3_PIN, HIGH);
    digitalWrite(R_IN4_PIN, LOW);
    analogWrite(R_ENB_PIN, speed);
  }
}

void haltRobot() {
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, 0);

  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, 0);
}

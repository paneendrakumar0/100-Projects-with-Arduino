/*
 * 100 Projects with Arduino - Day 45
 * Project: Calibrated 5-Sensor IR Array (Centroid Math Navigation)
 * 
 * DESCRIPTION:
 * This project interfaces a 5-channel TCRT5000 infrared reflectance sensor array.
 * To implement professional industrial AGV guidance control:
 * 1. Automatic Sensor Calibration: Runs a 5-second calibration phase on boot, sweeping motors to cross
 *    the line and recording minimum (white floor) and maximum (black line) reflection bounds for each sensor.
 * 2. Min-Max Normalization: Standardizes raw analog readings into a percentage-like range (0 to 1000),
 *    compensating for variance in sensor alignment, height, and ambient room lighting.
 * 3. Weighted Centroid Math: Calculates the horizontal center-of-mass (centroid) of the line relative
 *    to the robot's center axis. This converts discrete sensor gates into a continuous position offset float (-2.0 to +2.0).
 * 4. Proportional Differential Drive: Controls motor speeds proportionally based on the calculated offset error,
 *    steering the chassis smoothly.
 * 
 * CENTROID MATHEMATICS THEORY:
 * - Weighted Average Centroid:
 *   We assign each of the 5 sensors a physical location coordinate along the horizontal axis:
 *     Sensor 0 (Far Left)  : x_0 = -2.0
 *     Sensor 1 (Mid Left)  : x_1 = -1.0
 *     Sensor 2 (Center)    : x_2 =  0.0
 *     Sensor 3 (Mid Right) : x_3 =  1.0
 *     Sensor 4 (Far Right) : x_4 =  2.0
 *   Let W_i be the normalized reflectance value of sensor 'i' (0 for white, 1000 for black).
 *   The line centroid position (x_c) is calculated as:
 *     x_c = [ Sum( W_i * x_i ) ] / [ Sum( W_i ) ]
 *   This outputs a continuous scale:
 *     x_c = 0.0   -> Line is perfectly centered.
 *     x_c = -2.0  -> Line is far left.
 *     x_c = +1.5  -> Line is between mid-right and far-right.
 * 
 * WIRING:
 * - 5-Channel IR Sensor Array -> Arduino Uno
 *   - OUT1 (Far Left)  -> Pin A0
 *   - OUT2 (Mid Left)  -> Pin A1
 *   - OUT3 (Center)    -> Pin A2
 *   - OUT4 (Mid Right) -> Pin A3
 *   - OUT5 (Far Right) -> Pin A4
 *   - VCC              -> 5V
 *   - GND              -> GND
 * - L298N Motor Driver -> Arduino Uno
 *   - ENA (Left PWM)   -> Pin 5
 *   - IN1, IN2         -> Pins 4, 3
 *   - ENB (Right PWM)  -> Pin 6
 *   - IN3, IN4         -> Pins 7, 8
 */

// --- PIN DEFINITIONS ---
const int NUM_SENSORS = 5;
const int SENSOR_PINS[NUM_SENSORS] = {A0, A1, A2, A3, A4};

const int L_ENA_PIN = 5;
const int L_IN1_PIN = 4;
const int L_IN2_PIN = 3;

const int R_ENB_PIN = 6;
const int R_IN3_PIN = 7;
const int R_IN4_PIN = 8;

const int LED_INDICATOR_PIN = 13; // Built-in LED flashes during calibration

// --- PHYSICAL COORDINATE ASSIGNMENTS ---
const float SENSOR_COORDINATES[NUM_SENSORS] = {-2.0, -1.0, 0.0, 1.0, 2.0};

// --- CALIBRATION LIMIT ARRAYS ---
int sensorMinValues[NUM_SENSORS]; // Store minimum values (white floor)
int sensorMaxValues[NUM_SENSORS]; // Store maximum values (black line)

// --- CONTROL SETTINGS ---
const int CRUISE_SPEED = 140; // Base speed PWM (0-255)
const float STEER_GAIN = 45.0; // Proportional correction multiplier

// --- TIMING VARIABLES ---
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryIntervalMs = 200; // Telemetry rate (5 Hz)

void setup() {
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 45: 5-Sensor Calibrated IR Centroid Tracker");
  Serial.println("==================================================");

  pinMode(LED_INDICATOR_PIN, OUTPUT);
  
  // Initialize motor pins
  pinMode(L_ENA_PIN, OUTPUT);
  pinMode(L_IN1_PIN, OUTPUT);
  pinMode(L_IN2_PIN, OUTPUT);
  pinMode(R_ENB_PIN, OUTPUT);
  pinMode(R_IN3_PIN, OUTPUT);
  pinMode(R_IN4_PIN, OUTPUT);
  
  haltRobot();

  // Initialize calibration limits
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensorMinValues[i] = 1023; // High default to find minimums
    sensorMaxValues[i] = 0;    // Low default to find maximums
  }

  // Execute Auto-Calibration Sweep
  runAutoCalibration();
}

void loop() {
  int normalizedWeights[NUM_SENSORS];
  long weightSum = 0;
  float coordinateSum = 0.0;

  // Step 1: Read raw analog signals and apply min-max normalization
  for (int i = 0; i < NUM_SENSORS; i++) {
    int rawVal = analogRead(SENSOR_PINS[i]);
    
    // Normalize reading between 0 (white) and 1000 (black)
    int normVal = map(rawVal, sensorMinValues[i], sensorMaxValues[i], 0, 1000);
    normVal = constrain(normVal, 0, 1000); // Clamp outliers
    
    normalizedWeights[i] = normVal;
    weightSum += normVal;
    coordinateSum += ((float)normVal * SENSOR_COORDINATES[i]);
  }

  // Step 2: Compute Weighted Centroid Offset
  float lineCentroid = 0.0;
  bool lineDetected = (weightSum > 350); // Threshold to verify line is actually under the array

  if (lineDetected) {
    lineCentroid = coordinateSum / weightSum;
    
    // Step 3: Proportional steering adjustments
    // e.g. if centroid = +1.0 (line is right), left motor speeds up, right motor slows down to steer right
    int leftPWM = CRUISE_SPEED + (int)(lineCentroid * STEER_GAIN);
    int rightPWM = CRUISE_SPEED - (int)(lineCentroid * STEER_GAIN);
    
    driveMotors(leftPWM, rightPWM);
  } else {
    // If the line is lost (e.g. robot flew off track), halt motors as safety backup
    haltRobot();
  }

  // Step 4: Telemetry Monitor Output
  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryTime >= telemetryIntervalMs) {
    lastTelemetryTime = currentMillis;

    Serial.print("[WEIGHTS] ");
    for (int i = 0; i < NUM_SENSORS; i++) {
      Serial.print(normalizedWeights[i]);
      Serial.print(" ");
    }
    Serial.print("| Offset: ");
    if (lineDetected) {
      Serial.print(lineCentroid, 3);
    } else {
      Serial.print("LOST");
    }
    Serial.println();
  }
}

// --- CALIBRATION ROUTINE ---

/**
 * Sweeps the robot chassis in place back and forth for 5 seconds to cross the line,
 * recording minimum and maximum values for sensor calibration.
 */
void runAutoCalibration() {
  Serial.println("[CALIBRATION] Starting auto-calibration. Place robot centered on black line!");
  digitalWrite(LED_INDICATOR_PIN, HIGH); // Turn LED on to notify user
  
  unsigned long calTimerStart = millis();
  unsigned long lastToggleTime = millis();
  int pivotDirection = 1; // 1 = right pivot, -1 = left pivot
  
  // Drive slow pivot to cross the line
  drivePivot(pivotDirection, 110);
  
  while (millis() - calTimerStart < 5000) { // Run for 5 seconds
    // Sweep back and forth every 750ms to cross line completely
    if (millis() - lastToggleTime >= 750) {
      lastToggleTime = millis();
      pivotDirection = -pivotDirection;
      drivePivot(pivotDirection, 110);
    }

    // Monitor raw values across all sensors to find min/max
    for (int i = 0; i < NUM_SENSORS; i++) {
      int rawVal = analogRead(SENSOR_PINS[i]);
      if (rawVal < sensorMinValues[i]) {
        sensorMinValues[i] = rawVal;
      }
      if (rawVal > sensorMaxValues[i]) {
        sensorMaxValues[i] = rawVal;
      }
    }
    delay(5); // Small delay to avoid loading CPU
  }

  haltRobot();
  digitalWrite(LED_INDICATOR_PIN, LOW); // Turn LED off when finished
  Serial.println("[CALIBRATION] Auto-calibration finished successfully.");
  
  // Print calibration values to serial monitor for diagnostics
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(" Sensor ");
    Serial.print(i);
    Serial.print(" -> Min: ");
    Serial.print(sensorMinValues[i]);
    Serial.print(" | Max: ");
    Serial.println(sensorMaxValues[i]);
  }
  Serial.println("==================================================");
  delay(1000); // Settle user
}

// --- DIRECT MOTOR DRIVE GATES ---

void driveMotors(int leftSpeed, int rightSpeed) {
  // Constrain PWM values to valid limits
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Left Motor Direction
  if (leftSpeed >= 0) {
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, leftSpeed);
  } else {
    digitalWrite(L_IN1_PIN, LOW);
    digitalWrite(L_IN2_PIN, HIGH);
    analogWrite(L_ENA_PIN, -leftSpeed);
  }

  // Right Motor Direction
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

void drivePivot(int direction, int speed) {
  if (direction == 1) { // Pivot Right
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, speed);
    digitalWrite(R_IN3_PIN, LOW);
    digitalWrite(R_IN4_PIN, HIGH);
    analogWrite(R_ENB_PIN, speed);
  } else { // Pivot Left
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

/*
 * 100 Projects with Arduino - Day 46
 * Project: PD-Controlled Line Follower (Proportional-Derivative Feedback Steering)
 *
 * DESCRIPTION:
 * This project implements a closed-loop Proportional-Derivative (PD) control loop to regulate
 * the steering of a differential-drive robot along a black line.
 * To achieve high-speed robotics control standards:
 * 1. Deterministic Control Loop: Executes sensor readings, centroid math, and PD updates
 *    at a strict, fixed time step (50 Hz / 20ms) using non-blocking micros() timing.
 * 2. Proportional-Derivative (PD) Controller: Uses the normalized line centroid offset (Day 45)
 *    as the input error. The Proportional term corrects for current offset, and the Derivative term
 *    dampens steering overshoots by braking wheels when approaching the line center too fast.
 * 3. Serial Plotter Diagnostic Logs: Outputs telemetry in a comma-separated format
 * ("Line_Offset,Correction_PWM") so developers can tune gains visually using the Arduino Serial
 * Plotter.
 * 4. Automatic Calibration Bootstrap: Retains the Day 45 boot-calibration routine, compiling and
 * normalizing sensor data before launching the active control loop.
 *
 * STEERING PD CONTROL THEORY:
 * - Line Centroid Offset (Error):
 *     Error (e) = Line_Centroid - Target_Position = Line_Centroid  (Scale: -2.0 to +2.0)
 * - Proportional Gain (Kp): Writes a correction output directly proportional to the distance from
 * center: P = Kp * e
 * - Derivative Gain (Kd): Measures the rate of change of error (how fast the robot is crossing the
 * line): D = Kd * ( (e - last_e) / dt )
 * - Total Correction:
 *     Correction = P + D
 * - Differential Steering Actuation:
 *     Left Motor Speed  = Cruise_Speed + Correction
 *     Right Motor Speed = Cruise_Speed - Correction
 *   If the robot drifts left (error is negative), Correction is negative, slowing down the Left
 * wheel and speeding up the Right wheel to steer the robot back right.
 *
 * WIRING:
 * - 5-Channel IR Sensor Array -> Arduino Uno
 *   - OUT1 to OUT5 -> Pins A0 to A4
 *   - VCC, GND     -> 5V, GND
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

const int LED_INDICATOR_PIN = 13;

// --- PHYSICAL COORDINATE ASSIGNMENTS ---
const float SENSOR_COORDINATES[NUM_SENSORS] = {-2.0, -1.0, 0.0, 1.0, 2.0};

// --- CALIBRATION LIMIT ARRAYS ---
int sensorMinValues[NUM_SENSORS];
int sensorMaxValues[NUM_SENSORS];

// --- PD TUNING COEFFICIENTS ---
// Note: Kp handles base steering aggressiveness. Kd dampens wobble/oscillations.
// Adjust these values for your specific chassis gear ratio and motor voltage!
double Kp = 60.0;
double Kd = 4.5;

// --- CONTROL LOOP STATE VARIABLES ---
double lastError = 0.0;
double targetOffset = 0.0;  // Desired line centroid (perfectly centered is 0.0)

// --- TIMING CONFIGURATION ---
unsigned long lastPIDTimeUs = 0;
const unsigned long samplePeriodUs = 20000;  // 20ms sample interval = 50 Hz loop rate

// Cruise speed settings
const int BASE_CRUISE_SPEED = 130;  // Speed on straight segments (0-255)

void setup() {
  Serial.begin(9600);

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
    sensorMinValues[i] = 1023;
    sensorMaxValues[i] = 0;
  }

  // Run the 5-second auto-calibration sweep
  runAutoCalibration();

  // Output column headers for Serial Plotter
  Serial.println("Target_Line,Actual_Offset,PWM_Correction");

  lastPIDTimeUs = micros();
}

void loop() {
  unsigned long currentTimeUs = micros();

  // Strict 50 Hz control loop timer
  if (currentTimeUs - lastPIDTimeUs >= samplePeriodUs) {
    double dt = (double)(currentTimeUs - lastPIDTimeUs) /
                1000000.0;  // Sample interval in seconds (normally ~0.020s)
    lastPIDTimeUs = currentTimeUs;

    int normalizedWeights[NUM_SENSORS];
    long weightSum = 0;
    float coordinateSum = 0.0;

    // Step 1: Read raw analog signals and apply min-max normalization
    for (int i = 0; i < NUM_SENSORS; i++) {
      int rawVal = analogRead(SENSOR_PINS[i]);
      int normVal = map(rawVal, sensorMinValues[i], sensorMaxValues[i], 0, 1000);
      normVal = constrain(normVal, 0, 1000);

      normalizedWeights[i] = normVal;
      weightSum += normVal;
      coordinateSum += ((float)normVal * SENSOR_COORDINATES[i]);
    }

    // Check if line is detected under the sensor array
    bool lineDetected = (weightSum > 350);
    double currentOffset = 0.0;

    if (lineDetected) {
      // Step 2: Compute line centroid
      currentOffset = (double)coordinateSum / weightSum;

      // Calculate Error (e = measurement - target)
      double error = currentOffset - targetOffset;

      // Proportional term
      double pTerm = Kp * error;

      // Derivative term: D = Kd * (de/dt)
      double dTerm = Kd * ((error - lastError) / dt);
      lastError = error;

      // Combined PD output correction
      double correction = pTerm + dTerm;

      // Step 3: Apply differential PWM speed adjustments
      int leftPWM = BASE_CRUISE_SPEED + (int)correction;
      int rightPWM = BASE_CRUISE_SPEED - (int)correction;

      driveMotors(leftPWM, rightPWM);

      // Print telemetry formatted for Arduino Serial Plotter
      // Comparing Target (0.0), Actual Offset (-2.0 to +2.0), and scaled PWM correction (divided by
      // 100 to fit on scale)
      Serial.print(targetOffset, 1);
      Serial.print(",");
      Serial.print(currentOffset, 3);
      Serial.print(",");
      Serial.println(correction / 100.0, 2);

    } else {
      // Safety fail-safe: stop robot if line is lost
      haltRobot();

      // Reset errors to prevent derivative spike on re-acquisition
      lastError = 0.0;

      Serial.println("0.0,0.0,0.0");  // Output flatline to plotter
    }
  }
}

// --- PIVOT AUTO-CALIBRATION ROUTINE ---

void runAutoCalibration() {
  Serial.println("[CALIBRATION] Starting auto-calibration. Place robot centered on black line!");
  digitalWrite(LED_INDICATOR_PIN, HIGH);

  unsigned long calTimerStart = millis();
  unsigned long lastToggleTime = millis();
  int pivotDirection = 1;

  drivePivot(pivotDirection, 110);

  while (millis() - calTimerStart < 5000) {
    if (millis() - lastToggleTime >= 750) {
      lastToggleTime = millis();
      pivotDirection = -pivotDirection;
      drivePivot(pivotDirection, 110);
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
      int rawVal = analogRead(SENSOR_PINS[i]);
      if (rawVal < sensorMinValues[i]) sensorMinValues[i] = rawVal;
      if (rawVal > sensorMaxValues[i]) sensorMaxValues[i] = rawVal;
    }
    delay(5);
  }

  haltRobot();
  digitalWrite(LED_INDICATOR_PIN, LOW);
  Serial.println("[CALIBRATION] Complete. Loaded configurations.");
  delay(1500);  // Settle user
}

// --- DIRECT MOTOR DRIVE CONTROLLERS ---

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

void drivePivot(int direction, int speed) {
  if (direction == 1) {  // Pivot Right
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

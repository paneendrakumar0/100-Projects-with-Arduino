/*
 * 100 Projects with Arduino - Day 56
 * Project: Ultrasonic 2D Spatial Positioning Scanner (Trilateration Math)
 *
 * DESCRIPTION:
 * This project implements a local 2D spatial positioning system that tracks the coordinate position
 * of an object in a 2D plane (X, Y) relative to a sensor bar baseline.
 * It uses two HC-SR04 ultrasonic distance sensors mounted at a fixed distance (d) apart.
 *
 * TRILATERATION MATHEMATICS:
 * Let Sensor 1 (Left) be placed at origin coordinates (0, 0).
 * Let Sensor 2 (Right) be placed at coordinates (d, 0) along the baseline.
 * Let the target object be at position (x, y).
 * The measured distance from Sensor 1 to the object is r1, and from Sensor 2 is r2.
 *
 * The system is modeled by two intersecting circles:
 *   1)  x^2 + y^2 = r1^2
 *   2)  (x - d)^2 + y^2 = r2^2
 *
 * Subtracting equation (1) from (2) yields:
 *   (x - d)^2 - x^2 = r2^2 - r1^2
 *   x^2 - 2xd + d^2 - x^2 = r2^2 - r1^2
 *   -2xd + d^2 = r2^2 - r1^2
 *   2xd = d^2 + r1^2 - r2^2
 *
 *   x = (d^2 + r1^2 - r2^2) / (2 * d)
 *
 * Once x is resolved, y is calculated from circle (1):
 *   y = sqrt(r1^2 - x^2)   (assuming y >= 0, i.e., object is in front of the sensor bar)
 *
 * CRITICAL MECHATRONIC CONTROL details:
 * - Sequential Triggering: Triggering both sensors simultaneously causes cross-talk (Sensor 1
 *   detecting the echo from Sensor 2). We trigger the sensors sequentially with a 30ms guard
 *   interval to allow residual ultrasonic echoes to dissipate.
 * - Out-of-bounds math gate: Noise can cause r1^2 - x^2 < 0 (mathematically impossible on clean
 * circles). We intercept this case to prevent NaN (Not a Number) floating-point errors.
 *
 * WIRING:
 * - Sensor 1 (Left - Position 0,0) -> Arduino Uno
 *   - Trig Pin -> Pin 2
 *   - Echo Pin -> Pin 3
 *   - VCC, GND -> 5V, GND
 * - Sensor 2 (Right - Position d,0) -> Arduino Uno
 *   - Trig Pin -> Pin 4
 *   - Echo Pin -> Pin 5
 *   - VCC, GND -> 5V, GND
 */

// --- PIN DEFINITIONS ---
const int S1_TRIG_PIN = 2;
const int S1_ECHO_PIN = 3;

const int S2_TRIG_PIN = 4;
const int S2_ECHO_PIN = 5;

const int LED_INDICATOR_PIN = 13;

// --- GEOMETRIC BASELINE CONFIG ---
const float BASELINE_D = 25.0;  // Distance between the two sensors in centimeters (d)

// --- TIMING VARIABLES ---
unsigned long lastScanTimeMs = 0;
const unsigned long scanIntervalMs = 60;  // 60ms scan cycle (gives 30ms per sensor sequentially)

void setup() {
  Serial.begin(9600);

  pinMode(S1_TRIG_PIN, OUTPUT);
  pinMode(S1_ECHO_PIN, INPUT);

  pinMode(S2_TRIG_PIN, OUTPUT);
  pinMode(S2_ECHO_PIN, INPUT);

  pinMode(LED_INDICATOR_PIN, OUTPUT);

  digitalWrite(S1_TRIG_PIN, LOW);
  digitalWrite(S2_TRIG_PIN, LOW);

  Serial.println(F("[SPATIAL] 2D Positioning Scanner active."));
  Serial.print(F("[SPATIAL] Baseline Distance (d) = "));
  Serial.print(BASELINE_D);
  Serial.println(F(" cm"));
}

void loop() {
  if (millis() - lastScanTimeMs >= scanIntervalMs) {
    lastScanTimeMs = millis();

    // Step 1: Read Sensor 1 (Left) distance
    float r1 = readDistance(S1_TRIG_PIN, S1_ECHO_PIN);

    // Step 2: Delay briefly to prevent echo cross-talk
    delay(30);

    // Step 3: Read Sensor 2 (Right) distance
    float r2 = readDistance(S2_TRIG_PIN, S2_ECHO_PIN);

    // Filter out invalid/timeout measurements
    // HC-SR04 returns 0 or extremely high values if out of range
    if (r1 > 2.0 && r1 < 150.0 && r2 > 2.0 && r2 < 150.0) {
      // Step 4: Solve for X coordinate
      // x = (d^2 + r1^2 - r2^2) / (2 * d)
      float dSq = BASELINE_D * BASELINE_D;
      float r1Sq = r1 * r1;
      float r2Sq = r2 * r2;

      float x = (dSq + r1Sq - r2Sq) / (2.0 * BASELINE_D);

      // Step 5: Solve for Y coordinate
      // y = sqrt(r1^2 - x^2)
      // Check if radicand is positive to prevent NaN crash
      float yRadicand = r1Sq - (x * x);

      if (yRadicand >= 0.0) {
        float y = sqrt(yRadicand);

        // Flash LED to indicate successful positioning lock
        digitalWrite(LED_INDICATOR_PIN, HIGH);
        delayMicroseconds(100);
        digitalWrite(LED_INDICATOR_PIN, LOW);

        // Output results in structured coordinate format
        // Formatted for easy copy-pasting or custom visualization plotting
        Serial.print(F("R1_Dist:"));
        Serial.print(r1, 1);
        Serial.print(F(",R2_Dist:"));
        Serial.print(r2, 1);
        Serial.print(F(",Coord_X:"));
        Serial.print(x, 2);
        Serial.print(F(",Coord_Y:"));
        Serial.println(y, 2);
      } else {
        Serial.println(F("[SPATIAL] WARNING: Intersection error (geometry out of bounds)."));
      }
    } else {
      Serial.println(F("[SPATIAL] Lock lost (Target out of range or echo timed out)."));
    }
  }
}

// --- SEQUENTIAL ECHO PULSE RECORDER ---

float readDistance(int trigPin, int echoPin) {
  // Send a clean 10us trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure echo pulse duration (microsecond timeout set to 20ms = ~3.4m maximum range)
  long duration = pulseIn(echoPin, HIGH, 20000);

  if (duration == 0) return 999.0;  // Signal timeout indicator

  // Speed of sound = 343 m/s = 0.0343 cm/us
  // Distance = (Time * Speed) / 2 (round-trip)
  float distance = (float)duration * 0.01715;
  return distance;
}

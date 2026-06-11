/*
 * 100 Projects with Arduino - Day 38
 * Project: Ultrasonic Radar Sweep (Servo + Range Finder 2D Mapper)
 *
 * DESCRIPTION:
 * This project combines a servo motor and an HC-SR04 ultrasonic sensor to simulate an active
 * sonar/radar sweep. To achieve professional-level robotic mapping and telemetry standards:
 * 1. Non-Blocking Sweep Scheduler: Moves the servo index incrementally every 40ms using millis(),
 *    avoiding delay() commands so other background processing can occur.
 * 2. Ultrasonic Range-Gated Ranging: Utilizes a trigger-echo pulse routine with a low timeout
 * window (20ms) to prevent the CPU from hanging if no obstacles are in range.
 * 3. Radar-Visualizer Format Telemetry: Outputs formatted coordinates ("Angle,Distance") to the
 * Serial Monitor. This output is directly compatible with standard Processing radar visualizer
 * scripts.
 * 4. Sweep Direction Alternator: Smoothly toggles directions between Clockwise (15° to 165°) and
 *    Counter-Clockwise (165° to 15°) to prevent cable twisting.
 *
 * RADAR & ACOUSTIC SENSING THEORY:
 * - Sonar / Ultrasonic Ranging: The HC-SR04 emits an 8-cycle ultrasonic burst at 40 kHz. The sound
 * wave travels through the air, bounces off an object, and returns. The sensor outputs a digital
 * HIGH signal on the Echo pin equal to the time of flight of the acoustic wave.
 * - Math Calculation (Speed of Sound):
 *   At sea level and 20°C, the speed of sound is roughly 343 m/s (or 0.0343 cm/microsecond).
 *   Since the pulse travels to the target and back, the distance is:
 *     Distance (cm) = (Time_in_microseconds * 0.0343) / 2
 * - Polar Coordinates:
 *   Radar maps obstacles on a 2D plane using Polar coordinates (Distance r, Angle θ). A computer
 * can map this to Cartesian coordinates (X, Y) using: X = r * cos(θ) Y = r * sin(θ)
 *
 * WIRING:
 * - HC-SR04 Sensor -> Arduino Uno
 *   - VCC -> 5V
 *   - GND -> GND
 *   - TRIG -> Pin 11
 *   - ECHO -> Pin 12
 * - SG90 Servo Motor -> Arduino Uno
 *   - Signal (Orange/Yellow) -> Pin 9 (PWM-compatible)
 *   - VCC (Red)              -> 5V
 *   - GND (Brown/Black)      -> GND
 */

#include <Servo.h>

// --- PIN DEFINITIONS ---
const int SERVO_PIN = 9;  // Servo control pin
const int TRIG_PIN = 11;  // Ultrasonic Trigger pin
const int ECHO_PIN = 12;  // Ultrasonic Echo pin

// --- SERVO INSTANTIATION ---
Servo radarServo;

// --- MOTION CONFIGURATION ---
const int MIN_ANGLE = 15;  // Limit sweep to prevent mechanical lockup or cable stretching
const int MAX_ANGLE = 165;
int currentAngle = MIN_ANGLE;
int angleDirection = 1;   // 1 = Clockwise (increasing), -1 = Counter-Clockwise (decreasing)
const int angleStep = 1;  // Step resolution in degrees

// --- TIMING VARIABLES ---
unsigned long lastSweepStepTime = 0;
const unsigned long sweepStepDelayMs =
    40;  // Wait 40ms before moving to the next angle (allows echo settling)

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 38: Ultrasonic Radar Sweep and 2D Mapper");
  Serial.println("==================================================");

  // Configure ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Attach servo motor to pin
  radarServo.attach(SERVO_PIN);
  radarServo.write(currentAngle);  // Move to start angle

  // Wait briefly for the servo to reach the initial position
  delay(500);

  lastSweepStepTime = millis();
  Serial.println("[SYSTEM] Calibration complete. Sonar active.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking loop for the radar step cycle
  if (currentMillis - lastSweepStepTime >= sweepStepDelayMs) {
    lastSweepStepTime = currentMillis;

    // 1. Trigger distance reading at the current angle
    float distance = getUltrasonicDistance();

    // 2. Output formatted telemetry for mapping visualizers
    // Format: "Angle,Distance" (e.g. "90,45.2")
    Serial.print(currentAngle);
    Serial.print(",");
    Serial.println(distance, 1);

    // 3. Increment/Decrement angle for the next cycle
    currentAngle += (angleDirection * angleStep);

    // Check sweep limits and toggle direction
    if (currentAngle >= MAX_ANGLE) {
      currentAngle = MAX_ANGLE;
      angleDirection = -1;  // Reverse to CCW sweep
    } else if (currentAngle <= MIN_ANGLE) {
      currentAngle = MIN_ANGLE;
      angleDirection = 1;  // Reverse to CW sweep
    }

    // Write new position to the servo
    radarServo.write(currentAngle);
  }
}

/**
 * Triggers the ultrasonic sensor and measures the time of flight.
 * Returns the computed distance in centimeters, or -1.0 if out of range.
 */
float getUltrasonicDistance() {
  // Ensure trigger pin is low initially
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Send a 10 microsecond trigger pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echo pulse width.
  // We use a timeout of 20000 microseconds (20ms).
  // Sound travels 343 m/s, so in 20ms it travels 6.86 meters total (3.43 meters max target
  // distance). If no echo returns in 20ms, pulseIn returns 0, indicating out of range.
  unsigned long pulseDuration = pulseIn(ECHO_PIN, HIGH, 20000);

  if (pulseDuration == 0) {
    return -1.0;  // Out of range or no obstacle detected
  }

  // Calculate distance: Distance = (Time * Speed of Sound) / 2
  // Speed of sound = 0.0343 cm/microsecond
  float distCm = (float)pulseDuration * 0.0343 / 2.0;

  return distCm;
}

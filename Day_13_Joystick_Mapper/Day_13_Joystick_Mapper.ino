/*
 * 100 Projects with Arduino - Day 13
 * Project: Dual-Axis Joystick Mapper with Auto-Calibration & Debouncing
 *
 * DESCRIPTION:
 * This project interfaces a 2-axis analog joystick module (equipped with a select switch)
 * with the Arduino. To meet professional mechatronics standards, this code implements:
 * 1. Boot Auto-Calibration: During setup, the Arduino samples the resting (center) position
 *    of the joystick for X and Y axes. This offset is used to mathematically center the readings.
 * 2. Deadband Filter: A center "dead zone" is defined. Jitter around the center rest point is
 * ignored, ensuring stable, zeroed readings when the joystick is untouched.
 * 3. Range Mapping: Translates raw 10-bit ADC signals into a Cartesian coordinate system
 *    spanning from -100 (full left/down) to +100 (full right/up) - ideal for motor drive
 * controllers.
 * 4. Switch Debouncing: Debounces the select switch (button) using non-blocking timing.
 *
 * WIRING:
 * - Joystick VCC   -> Arduino 5V
 * - Joystick GND   -> Arduino GND
 * - Joystick VRx   -> Arduino Analog Pin A0 (X-axis input)
 * - Joystick VRy   -> Arduino Analog Pin A1 (Y-axis input)
 * - Joystick SW    -> Arduino Pin 2 (Select button, configured as INPUT_PULLUP)
 */

// --- PIN DEFINITIONS ---
const int JOY_X_PIN = A0;  // Analog input pin for X-axis (horizontal)
const int JOY_Y_PIN = A1;  // Analog input pin for Y-axis (vertical)
const int JOY_SW_PIN = 2;  // Digital input pin for select switch

// --- CALIBRATION & FILTER PARAMETERS ---
int centerX = 512;        // Calibrated center value for X (measured at boot)
int centerY = 512;        // Calibrated center value for Y (measured at boot)
const int DEADBAND = 15;  // Noise threshold. Readings within ±15 of center are forced to 0.

// --- TIMING CONSTANTS ---
const unsigned long sampleInterval = 40;  // Sample joystick at 25 Hz (every 40ms)
unsigned long lastSampleTime = 0;         // Stores last sample timestamp
const unsigned long debounceDelay = 50;   // Debounce delay for select button
unsigned long lastDebounceTime = 0;       // Stores last button edge change timestamp

// --- STATE VARIABLES ---
int buttonState = HIGH;      // Debounced state of joystick select switch
int lastButtonState = HIGH;  // Previous raw state of joystick select switch
int lastMappedX = 0;         // Stores previous mapped values to prevent serial flooding
int lastMappedY = 0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Configure pins
  pinMode(JOY_SW_PIN, INPUT_PULLUP);  // Use internal pull-up for the switch

  Serial.println("==================================================");
  Serial.println("Day 13: Joystick Value Mapper & Auto-Calibrator");
  Serial.println("==================================================");
  Serial.println("DO NOT TOUCH JOYSTICK. Calibrating center point...");

  // Perform Auto-Calibration (average 10 readings to establish stable resting coordinates)
  long sumX = 0;
  long sumY = 0;
  for (int i = 0; i < 10; i++) {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
    delay(50);
  }
  centerX = sumX / 10;
  centerY = sumY / 10;

  Serial.print("Calibration Complete. Center X: ");
  Serial.print(centerX);
  Serial.print(" | Center Y: ");
  Serial.println(centerY);
  Serial.println("System armed. Move the stick or click the button.");
}

void loop() {
  unsigned long currentTime = millis();

  // --- PART 1: DEBOUNCED SWITCH SCANNING (NON-BLOCKING) ---
  int buttonReading = digitalRead(JOY_SW_PIN);

  if (buttonReading != lastButtonState) {
    lastDebounceTime = currentTime;
  }

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    if (buttonReading != buttonState) {
      buttonState = buttonReading;
      if (buttonState == LOW) {
        Serial.println("[SW] Joystick Button Pressed!");
      } else {
        Serial.println("[SW] Joystick Button Released.");
      }
    }
  }
  lastButtonState = buttonReading;

  // --- PART 2: JOYSTICK ANALOG SAMPLING & COORDINATE MAPPING ---
  if (currentTime - lastSampleTime >= sampleInterval) {
    lastSampleTime = currentTime;

    // Read raw ADC inputs
    int rawX = analogRead(JOY_X_PIN);
    int rawY = analogRead(JOY_Y_PIN);

    // Adjust raw values based on boot calibration offsets
    int calX = rawX - centerX;
    int calY = rawY - centerY;

    // Apply deadband filter to eliminate center drift
    if (abs(calX) < DEADBAND) calX = 0;
    if (abs(calY) < DEADBAND) calY = 0;

    // Map the calibrated values to Cartesian coordinates (-100 to +100)
    // Note: If rawX < centerX, we map from [0, centerX] to [-100, 0]
    // If rawX > centerX, we map from [centerX, 1023] to [0, 100]
    int mappedX = 0;
    if (calX < 0) {
      mappedX = map(rawX, 0, centerX, -100, 0);
    } else if (calX > 0) {
      mappedX = map(rawX, centerX, 1023, 0, 100);
    }

    int mappedY = 0;
    // Note: Often VRy is inverted electrically depending on module mounting.
    // We map so pushing the stick UP yields positive (+) values, and DOWN yields negative (-).
    if (calY < 0) {
      mappedY = map(rawY, 0, centerY, 100, 0);  // Inverted mapping for natural direction
    } else if (calY > 0) {
      mappedY = map(rawY, centerY, 1023, 0, -100);
    }

    // Only print logs to Serial if the position has changed to prevent telemetry bloat
    if (mappedX != lastMappedX || mappedY != lastMappedY) {
      Serial.print("Raw: [");
      Serial.print(rawX);
      Serial.print(", ");
      Serial.print(rawY);
      Serial.print("] | Mapped Coordinates: X = ");
      Serial.print(mappedX);
      Serial.print(" | Y = ");
      Serial.println(mappedY);

      lastMappedX = mappedX;
      lastMappedY = mappedY;
    }
  }
}

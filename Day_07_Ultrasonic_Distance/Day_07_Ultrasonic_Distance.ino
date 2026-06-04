/*
 * 100 Projects with Arduino - Day 7
 * Project: Ultrasonic Sensor Distance Measurer (HC-SR04)
 * 
 * DESCRIPTION:
 * This project demonstrates how to interface the HC-SR04 ultrasonic distance sensor.
 * It measures the distance to an object using sound waves and displays the results in centimeters
 * and inches on the Serial Monitor. To maintain professional coding standards:
 * 1. The sensor trigger timing is controlled using a non-blocking millis() scheduler.
 * 2. The pulseIn() function is configured with a timeout (26,000 microseconds) to prevent 
 *    the Arduino from freezing if the echo pulse is missed or the target is out of range.
 * 
 * THEORY OF OPERATION:
 * 1. The HC-SR04 ultrasonic sensor operates on the principle of echolocation (similar to bats).
 *    It contains two piezoelectric transducers: one transmitter (speaker) and one receiver (microphone).
 * 2. The Measurement Process:
 *    - The Arduino sends a 10-microsecond high pulse to the Trigger pin.
 *    - The sensor responds by transmitting an ultrasonic burst of 8 pulses at 40 kHz.
 *    - The sensor sets its Echo pin to HIGH.
 *    - The sound waves travel through the air, strike an object, and bounce back.
 *    - The receiver detects the returning wave, and the sensor pulls the Echo pin LOW.
 * 3. The time the Echo pin remains HIGH equals the total travel time (round trip) of the sound wave.
 * 4. Physics Math:
 *    The speed of sound in air at 20°C is approximately 343 m/s (or 0.0343 cm/µs).
 *    Since the pulse travels to the object and back, the distance is:
 *    Distance (cm) = (Travel Time in µs * 0.0343) / 2
 *    Which simplifies to:
 *    Distance (cm) = Travel Time / 58.3
 *    Distance (inches) = Travel Time / 148.0
 * 
 * WIRING:
 * - HC-SR04 VCC  -> Arduino 5V
 * - HC-SR04 GND  -> Arduino GND
 * - HC-SR04 Trig -> Arduino Pin 3
 * - HC-SR04 Echo -> Arduino Pin 4
 */

// --- PIN DEFINITIONS ---
const int TRIG_PIN = 3;  // Digital output pin to trigger the ultrasonic pulse
const int ECHO_PIN = 4;  // Digital input pin to read the echo pulse duration

// --- SCHEDULER VARIABLES ---
unsigned long lastMeasureTime = 0;       // Stores the last time a measurement was taken
const unsigned long measureInterval = 200; // Time between measurements in milliseconds (5 Hz)

// --- PHYSICS CONSTANTS ---
// Maximum timeout for pulseIn. If sound travels at 343 m/s, a round trip of 4.5 meters
// (450 cm) takes: (450 * 2) / 0.0343 = 26,239 microseconds.
// Setting the timeout to 26,000 µs prevents the code from blocking if nothing is detected.
const unsigned long ECHO_TIMEOUT = 26000; 

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure pin modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Set trigger pin to idle LOW state
  digitalWrite(TRIG_PIN, LOW);
  
  Serial.println("==================================================");
  Serial.println("Day 7: Ultrasonic Sensor Distance Measurer (HC-SR04)");
  Serial.println("==================================================");
  Serial.println("System Initialized. Point sensor at target...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Run measurement at scheduled non-blocking interval (every 200ms)
  if (currentTime - lastMeasureTime >= measureInterval) {
    lastMeasureTime = currentTime;
    
    // Step 2: Trigger the ultrasonic transducer
    // We send a short 2µs LOW pulse to ensure a clean HIGH pulse, then a 10µs HIGH pulse.
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Step 3: Measure the duration the Echo pin is HIGH.
    // pulseIn(pin, value, timeout_microseconds) returns the duration in microseconds.
    // If the timeout is reached without an echo, it returns 0.
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT);
    
    // Step 4: Calculate distance if a valid echo was received
    if (duration == 0) {
      Serial.println("[ERROR] Out of range or no obstacle detected.");
    } else {
      // Calculate distances using simplified physics constants
      float distanceCm = duration / 58.3;
      float distanceInches = duration / 148.0;
      
      // Print telemetry log
      Serial.print("Echo Duration: ");
      Serial.print(duration);
      Serial.print(" us | Distance: ");
      Serial.print(distanceCm, 1);
      Serial.print(" cm (");
      Serial.print(distanceInches, 1);
      Serial.println(" in)");
    }
  }
  
  // Non-blocking architecture allows the Arduino to handle other tasks concurrently
}

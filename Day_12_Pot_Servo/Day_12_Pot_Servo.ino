/*
 * 100 Projects with Arduino - Day 12
 * Project: Potentiometer-Controlled Servo Motor (Jitter Filtering & Control Loops)
 * 
 * DESCRIPTION:
 * This project interfaces a rotary potentiometer to manually steer a servo motor's output angle.
 * To implement professional mechatronics standards, this sketch features:
 * 1. Scheduled Sampling: The analog input is sampled at a stable 33 Hz rate using a non-blocking
 *    millis() timer.
 * 2. Deadband Noise Filter: A software threshold (deadband) filters out tiny ADC fluctuations
 *    (electrical noise) from the potentiometer. This prevents the servo motor from constantly humming,
 *    grinding, vibrating, and drawing excessive current when the dial is held still.
 * 
 * THEORY OF OPERATION:
 * 1. The potentiometer outputs a variable DC voltage (0V to 5V) to pin A0 based on shaft position.
 * 2. The Arduino ADC converts this to a 10-bit integer (0 to 1023).
 * 3. In standard examples, you see: `servo.write(map(analogRead(0), 0, 1023, 0, 180));`
 *    This has two bugs in real mechatronic systems:
 *    - Jitter: The last bit of the ADC fluctuates naturally due to noise. If 512 oscillates to 513,
 *      the servo receives continuous micro-step adjustments, causing it to buzz, overheat, and fail.
 *    - Blocking: High speed polling wastes CPU.
 * 4. We solve this by introducing a DEADBAND constant. We calculate: `abs(currentRead - lastRead)`.
 *    If the change is less than 4 units (approx 0.4% change), we discard it as noise and make no adjustments.
 * 
 * WIRING:
 * - Potentiometer Pin 1 (Outer left)  -> Arduino 5V
 * - Potentiometer Pin 2 (Middle wiper)-> Arduino Analog A0
 * - Potentiometer Pin 3 (Outer right) -> Arduino GND
 * - Servo Signal (Orange/Yellow)      -> Arduino Pin 9
 * - Servo Power (Red)                 -> Arduino 5V (or external 5V-6V supply)
 * - Servo Ground (Brown/Black)        -> Arduino GND
 */

#include <Servo.h>

// --- PIN DEFINITIONS ---
const int POT_PIN = A0;   // Analog pin connected to potentiometer wiper
const int SERVO_PIN = 9;  // Digital output pin connected to servo signal

// --- CONTROL PARAMETERS ---
const int DEADBAND = 4;           // Noise filter threshold. Adjustments under 4 ADC units are ignored.
const unsigned long sampleDelay = 30; // Time in ms between potentiometer reads (30ms = 33Hz)

// --- SERVO OBJECT ---
Servo myServo;

// --- STATE VARIABLES ---
int lastPotValue = 0;        // Remembers the last stable potentiometer reading
int currentAngle = 0;        // Tracks the current angle of the servo
unsigned long lastSampleTime = 0; // Tracks the timing of our non-blocking scheduler

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Attach servo to control pin
  myServo.attach(SERVO_PIN);
  
  // Read initial potentiometer position to calibrate startup angle
  lastPotValue = analogRead(POT_PIN);
  currentAngle = map(lastPotValue, 0, 1023, 0, 180);
  myServo.write(currentAngle);
  
  Serial.println("==================================================");
  Serial.println("Day 12: Potentiometer-Controlled Servo Motor");
  Serial.println("==================================================");
  Serial.print("System Initialized. Startup Angle: ");
  Serial.print(currentAngle);
  Serial.println("°");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Sample potentiometer at a scheduled rate (every 30ms)
  if (currentTime - lastSampleTime >= sampleDelay) {
    lastSampleTime = currentTime;
    
    // Read raw ADC value (0 - 1023)
    int potValue = analogRead(POT_PIN);
    
    // Step 2: Calculate absolute difference to verify if change exceeds the deadband
    // abs() returns the positive difference (e.g. abs(-5) = 5)
    if (abs(potValue - lastPotValue) > DEADBAND) {
      
      // Step 3: Map the new ADC value to target servo angle (0 - 180)
      int targetAngle = map(potValue, 0, 1023, 0, 180);
      
      // Step 4: If the mapped angle has changed, execute position update
      if (targetAngle != currentAngle) {
        currentAngle = targetAngle;
        myServo.write(currentAngle);
        
        // Log telemetry
        Serial.print("Pot ADC: ");
        Serial.print(potValue);
        Serial.print(" | Target Angle: ");
        Serial.print(currentAngle);
        Serial.println("°");
      }
      
      // Record this reading as our new baseline for comparison
      lastPotValue = potValue;
    }
  }
  
  // Non-blocking loop - you can run PID calculations or read other sensors here!
}

/*
 * 100 Projects with Arduino - Day 11
 * Project: Non-Blocking Servo Motor Sweep
 * 
 * DESCRIPTION:
 * This project demonstrates how to interface a hobby servo motor with the Arduino.
 * Instead of using the traditional, blocking delay() loop (which freezes the Arduino and
 * prevents it from reading other sensors or driving other actuators), this code implements
 * a non-blocking sweep algorithm using millis() to update the servo angle incrementally
 * every 15 milliseconds.
 * 
 * THEORY OF OPERATION:
 * 1. A hobby servo motor is a closed-loop actuator that uses positional feedback to control
 *    its shaft angle (typically 0 to 180 degrees). Internally, it contains:
 *    - A DC motor.
 *    - A high-reduction gearbox to increase torque.
 *    - A feedback potentiometer connected to the output shaft to measure the current angle.
 *    - An internal control circuit (error amplifier) that compares the target angle to the
 *      actual angle and drives the DC motor to correct any error.
 * 2. Pulse Position Modulation (PPM) Control:
 *    We control the servo by sending a repetitive square wave pulse train at 50 Hz (every 20ms).
 *    The width of the HIGH pulse determines the target angle:
 *    - 1.0 millisecond (1000 µs) pulse  -> 0 degrees.
 *    - 1.5 millisecond (1500 µs) pulse  -> 90 degrees (neutral).
 *    - 2.0 millisecond (2000 µs) pulse  -> 180 degrees.
 * 3. The Arduino <Servo.h> library handles generating these exact microsecond pulses in the background
 *    using hardware interrupts.
 * 
 * WIRING:
 * - Servo Red (VCC)     -> Arduino 5V (For small SG90 servos. Large servos need an external power supply!)
 * - Servo Brown/Black (GND) -> Arduino GND
 * - Servo Yellow/Orange (Signal) -> Arduino Pin 9 (PWM/Digital pin)
 * 
 * IMPORTANT: Hobby servos can draw large current spikes under load. While a tiny SG90 servo
 * can run directly off the Arduino 5V pin for a simple desktop sweep, larger servos (like MG995/MG996R)
 * will draw enough current to brown-out (reset) the Arduino or damage its voltage regulator. Always
 * use an external 5V-6V power supply (sharing GND with the Arduino) for heavy-load servos!
 */

#include <Servo.h> // Include the standard Arduino Servo Library

// --- PIN DEFINITIONS ---
const int SERVO_PIN = 9; // Pin connected to the servo signal wire

// --- SERVO OBJECT ---
Servo myServo;

// --- SWEEP STATE VARIABLES ---
int currentAngle = 0;              // Current position of the servo in degrees (0 - 180)
int sweepDirection = 1;            // Direction of sweep: 1 = clockwise, -1 = counterclockwise
unsigned long lastStepTime = 0;    // Time of last angle increment
const unsigned long stepDelay = 15; // Time to wait between 1-degree steps (15ms is standard for speed)

// --- TELEMETRY VARIABLES ---
unsigned long lastLogTime = 0;       // Stores last time we printed to the Serial Monitor
const unsigned long logInterval = 200; // Log status 5 times a second

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Attach the servo object to the pin
  myServo.attach(SERVO_PIN);
  
  // Initialize servo to home position (0 degrees)
  myServo.write(currentAngle);
  
  Serial.println("==================================================");
  Serial.println("Day 11: Non-Blocking Servo Motor Sweep");
  Serial.println("==================================================");
  Serial.println("System Initialized. Commencing sweep...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Incremental, non-blocking angle sweep (runs every 15ms)
  if (currentTime - lastStepTime >= stepDelay) {
    lastStepTime = currentTime;
    
    // Advance the angle based on the sweep direction
    currentAngle += sweepDirection;
    
    // Check boundaries and reverse direction if we hit limits
    if (currentAngle >= 180) {
      currentAngle = 180;
      sweepDirection = -1; // Reverse direction (sweep down)
      Serial.println(">> Reached 180° Limit. Reversing... <<");
    } 
    else if (currentAngle <= 0) {
      currentAngle = 0;
      sweepDirection = 1; // Reverse direction (sweep up)
      Serial.println(">> Reached 0° Limit. Reversing... <<");
    }
    
    // Write the new angle to the servo. 
    // The library converts this 0-180 angle into a 1000µs - 2000µs pulse train.
    myServo.write(currentAngle);
  }
  
  // Step 2: Non-blocking telemetry output
  if (currentTime - lastLogTime >= logInterval) {
    lastLogTime = currentTime;
    
    Serial.print("Target Angle: ");
    Serial.print(currentAngle);
    Serial.print("° | Direction: ");
    Serial.print(sweepDirection == 1 ? "CW (+)" : "CCW (-)");
    
    // Calculate estimated PPM pulse width in microseconds (for educational reference)
    // 0° = 1000µs, 180° = 2000µs
    unsigned int pulseUs = map(currentAngle, 0, 180, 1000, 2000);
    Serial.print(" | Est. PPM Pulse: ");
    Serial.print(pulseUs);
    Serial.println(" us");
  }
  
  // The loop runs thousands of times a second. We can add sensor reads here
  // and the servo will sweep smoothly in the background without hiccuping!
}

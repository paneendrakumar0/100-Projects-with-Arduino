/*
 * 100 Projects with Arduino - Day 36
 * Project: DC Motor Encoder Feedback (Real-Time RPM & Odometry)
 *
 * DESCRIPTION:
 * This project implements a real-time shaft rotation sensor (encoder) feedback loop.
 * To achieve professional robotic sensing and telemetry standards:
 * 1. Interrupt-Driven Pulse Counting: Uses an external hardware interrupt on digital Pin 2
 *    to count rising edges from a single-channel optical or Hall-effect encoder wheel.
 * 2. Fixed-Window RPM Calculation: Implements a non-blocking millis() timer window (e.g., 200ms)
 *    to convert pulse counts into Revolutions Per Minute (RPM) and angular velocity (rad/s).
 * 3. Atomic Register Access: Disables interrupts temporarily during multi-byte variable reads
 *    to prevent "data tearing" corruption from concurrent ISR execution.
 * 4. Active PWM Sweep: Drives a DC motor via a PWM pin, plotting the relationship between control
 * duty cycles and physical output speed (measured RPM).
 *
 * ENCODER & VELOCITY PHYSICS:
 * - Optical Slot Encoders: Contain an infrared LED emitting light toward a photo-transistor
 * receiver, separated by a slotted disk attached to the motor shaft. As the disk spins, slots block
 * and pass the light beam, generating a digital pulse train (HIGH/LOW square wave).
 * - Pulses Per Revolution (PPR): The physical number of slots in the disk (e.g., 20 slots).
 * - RPM Calculation Formula:
 *   If the microchip records 'N' pulses in a time window 'dt' (in milliseconds):
 *     Revolutions = N / PPR
 *     Time in Minutes = dt / 60,000
 *     RPM = (Revolutions) / (Time in Minutes) = (N * 60,000) / (PPR * dt)
 *   For a dt = 200 ms and PPR = 20:
 *     RPM = (N * 60,000) / (20 * 200) = N * 15
 *
 * WIRING:
 * - Encoder Sensor -> Arduino Uno
 *   - VCC -> 5V
 *   - GND -> GND
 *   - OUT -> Pin 2 (Must be Pin 2 or 3 for external interrupts on Uno!)
 * - DC Motor Driver (L298N or single transistor) -> Arduino Uno
 *   - Motor PWM Enable / Transistor Base -> Pin 9 (PWM)
 *   - Motor GND -> Shared Ground
 *   - External Motor Battery -> Driver Power Terminal
 */

// --- PIN DEFINITIONS ---
const int ENCODER_PIN = 2;    // Encoder output signal (Must be Pin 2 or 3)
const int MOTOR_PWM_PIN = 9;  // PWM motor control signal

// --- ENCODER CONFIGURATION ---
const int ENCODER_PPR = 20;  // Pulses Per Revolution of the encoder wheel (e.g., 20 slots)

// --- VOLATILE PULSE COUNT VARIABLES ---
volatile long encoderPulseCount = 0;  // Volatile accumulator incremented inside ISR

// --- TIMING CONFIGURATION ---
unsigned long lastRPMCalculationTime = 0;
const unsigned long calculationWindowMs = 200;  // Recalculate speed every 200ms

// Motor speed sweep timing
unsigned long lastMotorSweepTime = 0;
const unsigned long motorSweepIntervalMs = 4000;  // Advance motor speed every 4 seconds
int motorSpeedIndex = 0;
const int speedSteps[] = {0, 80, 130, 180, 255};  // PWM duty cycles to sweep through
const int numSpeedSteps = sizeof(speedSteps) / sizeof(speedSteps[0]);

// --- TELEMETRY VARIABLES ---
float calculatedRPM = 0.0;
float calculatedRadSec = 0.0;

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 36: DC Motor Encoder Feedback & RPM Monitor");
  Serial.println("==================================================");

  pinMode(ENCODER_PIN, INPUT_PULLUP);  // Use internal pull-up to stabilize pulse transitions
  pinMode(MOTOR_PWM_PIN, OUTPUT);

  // Attach external hardware interrupt 0 (on Pin 2) to trigger on RISING edge
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulseISR, RISING);

  // Initialize motor speed
  analogWrite(MOTOR_PWM_PIN, speedSteps[motorSpeedIndex]);

  lastRPMCalculationTime = millis();
  lastMotorSweepTime = millis();
  Serial.println("[SYSTEM] Interrupts configured. Beginning PWM sweeps...");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. NON-BLOCKING RPM AND VELOCITY CALCULATIONS ---
  if (currentMillis - lastRPMCalculationTime >= calculationWindowMs) {
    unsigned long timeElapsed = currentMillis - lastRPMCalculationTime;
    lastRPMCalculationTime = currentMillis;

    // Read and clear the pulse count atomically (disabling interrupts temporarily)
    noInterrupts();
    long pulsesCaptured = encoderPulseCount;
    encoderPulseCount = 0;  // Reset counter for the next window
    interrupts();

    // Calculate shaft RPM: RPM = (Pulses * 60,000) / (PPR * dt)
    calculatedRPM = (float)(pulsesCaptured * 60000.0) / (ENCODER_PPR * timeElapsed);

    // Calculate angular velocity in Radians Per Second: ω = RPM * 2π / 60
    calculatedRadSec = calculatedRPM * (2.0 * PI / 60.0);

    // Print measurements to Serial Monitor
    Serial.print("[MEASUREMENT] PWM Out: ");
    Serial.print(speedSteps[motorSpeedIndex]);
    Serial.print(" | Pulses: ");
    Serial.print(pulsesCaptured);
    Serial.print(" | Speed: ");
    Serial.print(calculatedRPM, 1);
    Serial.print(" RPM | ");
    Serial.print(calculatedRadSec, 2);
    Serial.println(" rad/s");
  }

  // --- 2. MOTOR PWM SPEED STEP CONTROLLER ---
  if (currentMillis - lastMotorSweepTime >= motorSweepIntervalMs) {
    lastMotorSweepTime = currentMillis;

    // Cycle to the next PWM speed step
    motorSpeedIndex = (motorSpeedIndex + 1) % numSpeedSteps;
    analogWrite(MOTOR_PWM_PIN, speedSteps[motorSpeedIndex]);

    Serial.print("\n[COMMAND] -> Transitioned to Motor PWM Duty Cycle: ");
    Serial.print(speedSteps[motorSpeedIndex]);
    Serial.println(" / 255");
  }
}

/**
 * INTERRUPT SERVICE ROUTINE (ISR)
 * Executed instantly on rising edge of Pin 2 pulse.
 * Must remain extremely fast.
 */
void countPulseISR() {
  encoderPulseCount++;
}

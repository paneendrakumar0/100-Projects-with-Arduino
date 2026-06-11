/*
 * 100 Projects with Arduino - Day 37
 * Project: Closed-Loop PID Motor Speed Control (Feedback Controller)
 *
 * DESCRIPTION:
 * This project implements a closed-loop Proportional-Integral-Derivative (PID) controller
 * to regulate the speed (RPM) of a DC motor against variable mechanical loads.
 * To satisfy professional mechatronic controls engineering standards:
 * 1. Discrete PID Controller: Computes proportional, integral, and derivative terms at a fixed
 * sample rate (10 Hz).
 * 2. Integral Anti-Windup: Clamps the accumulator of the integral term to prevent runaway overshoot
 *    when the actuator saturates (reaches 0 or 255 PWM).
 * 3. Serial Plotter Integration: Outputs telemetry in a comma-separated format
 * ("Setpoint,Actual_RPM,PWM_Output") which can be plotted in real-time using the Arduino IDE Serial
 * Plotter.
 * 4. Automated Setpoint Cycling: Varies the target RPM setpoint every 6 seconds to demonstrate
 *    controller transient response, settling time, and steady-state error correction.
 *
 * PID CONTROL MATHEMATICS & THEORY:
 * - Closed-Loop Control: Unlike open-loop systems, closed-loop control continuously measures the
 * system output (RPM) and adjusts the control output (PWM) to minimize the error: Error e(t) =
 * Setpoint - Measurement
 * - Control Output Equation:
 *     u(t) = Kp * e(t) + Ki * ∫ e(t) dt + Kd * (de(t) / dt)
 *   - Proportional Gain (Kp): Generates a control signal proportional to the current error. If
 * error is large, it pushes hard. High Kp speeds up response but causes oscillation.
 *   - Integral Gain (Ki): Sums past error over time (integration). This eliminates steady-state
 * errors (e.g., if a motor slows down due to friction, the integral sum accumulates, increasing PWM
 * until the target is met).
 *   - Derivative Gain (Kd): Measures the rate of change of error (slope). It acts as a dampener,
 * predicting approaching setpoints and braking the controller to reduce overshoot.
 * - Anti-Windup: If the motor is stalled or blocked, the error remains high, causing the integral
 * term to grow infinitely (windup). When the blockage is removed, this massive sum causes the motor
 * to overshoot wildly. We resolve this by clamping the integral sum.
 *
 * WIRING:
 * - Encoder Sensor -> Arduino Uno
 *   - VCC -> 5V
 *   - GND -> GND
 *   - OUT -> Pin 2 (Interrupt 0)
 * - L298N Motor Driver (or Transistor circuit) -> Arduino Uno
 *   - ENA (Speed PWM) -> Pin 9 (PWM)
 *   - IN1 (Direction) -> Pin 7 (Set HIGH in setup)
 *   - IN2 (Direction) -> Pin 8 (Set LOW in setup)
 *   - GND             -> GND (Shared ground!)
 *   - External Motor Battery -> Driver Power Terminal
 */

// --- PIN DEFINITIONS ---
const int ENCODER_PIN = 2;    // Encoder OUT (Interrupt 0)
const int MOTOR_PWM_PIN = 9;  // PWM Speed pin on L298N (ENA)
const int IN1_PIN = 7;        // L298N Direction pin 1
const int IN2_PIN = 8;        // L298N Direction pin 2

// --- ENCODER CONFIGURATION ---
const int ENCODER_PPR = 20;  // Pulses Per Revolution of the encoder wheel

// --- PID TUNING PARAMETERS (Kp, Ki, Kd) ---
// Note: These values are tuned for a standard hobby yellow gearmotor running at 6-9V.
// If your motor behaves erratically or fluctuates, adjust these coefficients!
double Kp = 0.60;
double Ki = 1.50;
double Kd = 0.03;

// --- CONTROL LOOP VARIABLES ---
double targetSetpointRPM = 0.0;   // Desired motor speed in RPM
double currentMeasuredRPM = 0.0;  // Speed measured from encoder in RPM
double lastError = 0.0;
double integralAccumulator = 0.0;
const double integralMaxLimit = 150.0;  // Anti-windup threshold limit

// --- TIMING CONFIGURATION ---
unsigned long lastPIDTime = 0;
const unsigned long pidSampleTimeMs = 100;  // Run PID controller at 10 Hz (every 100ms)

// Setpoint cycling configuration
unsigned long lastSetpointCycleTime = 0;
const unsigned long setpointCycleIntervalMs = 6000;  // Change target speed every 6 seconds
int setpointIndex = 0;
const double setpointSteps[] = {0.0, 100.0, 220.0, 140.0};  // Target RPM steps to cycle
const int numSetpointSteps = sizeof(setpointSteps) / sizeof(setpointSteps[0]);

// --- VOLATILE ENCODER COUNTER ---
volatile long encoderPulseCount = 0;

void setup() {
  Serial.begin(9600);

  // Setup motor control pins
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Set motor direction forward
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);

  // Initialize encoder pin as pull-up input
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulseISR, RISING);

  // Set initial target speed
  targetSetpointRPM = setpointSteps[setpointIndex];

  // Print header names for Serial Plotter labels
  Serial.println("Setpoint,Actual_RPM,PWM_Output");

  lastPIDTime = millis();
  lastSetpointCycleTime = millis();
}

void loop() {
  unsigned long currentTimeMs = millis();

  // --- 1. SETPOINT SCHEDULER CYCLE (Every 6 seconds) ---
  if (currentTimeMs - lastSetpointCycleTime >= setpointCycleIntervalMs) {
    lastSetpointCycleTime = currentTimeMs;

    // Cycle to the next target RPM in our test array
    setpointIndex = (setpointIndex + 1) % numSetpointSteps;
    targetSetpointRPM = setpointSteps[setpointIndex];

    // If target is 0, clear accumulator to prevent motor spinning during stop
    if (targetSetpointRPM == 0.0) {
      integralAccumulator = 0.0;
    }
  }

  // --- 2. REGULATED PID CONTROL LOOP (Runs at exactly 10 Hz) ---
  if (currentTimeMs - lastPIDTime >= pidSampleTimeMs) {
    double dt =
        (double)(currentTimeMs - lastPIDTime) / 1000.0;  // Time step in seconds (normally ~0.10s)
    lastPIDTime = currentTimeMs;

    // Retrieve and reset pulse counts atomically
    noInterrupts();
    long pulsesCaptured = encoderPulseCount;
    encoderPulseCount = 0;
    interrupts();

    // Calculate actual RPM: RPM = (Pulses * 60,000) / (PPR * dt_ms)
    // dt is sample interval in seconds. dt * 1000 converts to milliseconds.
    currentMeasuredRPM = (double)(pulsesCaptured * 60.0) / (ENCODER_PPR * dt);

    // Calculate Error
    double error = targetSetpointRPM - currentMeasuredRPM;

    // Proportional Term
    double pTerm = Kp * error;

    // Integral Term with Anti-Windup Clamping
    integralAccumulator += error * dt;
    integralAccumulator = constrain(integralAccumulator, -integralMaxLimit, integralMaxLimit);
    double iTerm = Ki * integralAccumulator;

    // Derivative Term (Rate of change of error)
    double dTerm = Kd * ((error - lastError) / dt);
    lastError = error;

    // Combined Control Output
    double controlValue = pTerm + iTerm + dTerm;

    // Clamp output to valid PWM duty cycle limits
    int pwmOutput = constrain((int)controlValue, 0, 255);

    // Write PWM to driver. If target setpoint is 0, force motor stop.
    if (targetSetpointRPM == 0.0) {
      analogWrite(MOTOR_PWM_PIN, 0);
      pwmOutput = 0;
    } else {
      analogWrite(MOTOR_PWM_PIN, pwmOutput);
    }

    // Output telemetry formatted for Arduino Serial Plotter: "Setpoint,Actual_RPM,PWM_Output"
    Serial.print(targetSetpointRPM, 1);
    Serial.print(",");
    Serial.print(currentMeasuredRPM, 1);
    Serial.print(",");
    Serial.println(pwmOutput);
  }
}

/**
 * INTERRUPT SERVICE ROUTINE (ISR)
 * Increments pulse count instantly on encoder rising pulse edge.
 */
void countPulseISR() {
  encoderPulseCount++;
}

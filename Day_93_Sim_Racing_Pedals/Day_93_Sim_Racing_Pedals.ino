/*
 * 100 Projects with Arduino - Day 93
 * Project: Sim Racing Pedals & Shifter Interface (Analog Deadzones & Dynamic Calibration)
 *
 * DESCRIPTION:
 * This project implements the mechatronic interface logic for DIY Sim Racing Pedals
 * (Throttle, Brake, Clutch) and a Sequential Shifter.
 *
 * CORE ENGINEERING CONCEPTS:
 * 1. Inner & Outer Deadzones: Filters out sensor noise when foot rests on a pedal (lower deadzone)
 *    and guarantees 100% input activation before physical travel ends (upper deadzone).
 * 2. Dynamic Auto-Calibration: Automatically tracks and saves the physical minimum and maximum
 *    sensor limits in real-time.
 * 3. Linear Interpolation (Scaling): Maps the calibrated active range between deadzones to a
 *    high-resolution game controller output (10-bit: 0 to 1023).
 * 4. Dual Mode: Emulates a native USB Game Controller (when using Leonardo/Micro and Joystick
 * library) or simulates the output over Serial Plotter/Monitor (Uno fallback).
 *
 * THE PHYSICS OF PEDAL INPUTS:
 * Sim racing pedals use sensors to measure pedal displacement or force:
 * - Throttle/Clutch: Typically use potentiometers or Hall-effect sensors (measure angular
 * position).
 * - Brake: Often uses a Load Cell (measures force using strain gauges and an HX711 or op-amp).
 * Mechanical tolerances, spring wear, and sensor drift mean raw voltage ranges change over time.
 * Without software calibration and deadzones:
 * - Resting your foot on the brake might trigger a 1% brake drag (inner deadzone solves this).
 * - Stomping on the throttle might only reach 98% in-game (outer deadzone solves this).
 *
 * WIRING:
 * - Throttle Potentiometer Wiper -> Analog Pin A0
 * - Brake Pot/Load Cell Amp Output -> Analog Pin A1
 * - Clutch Potentiometer Wiper   -> Analog Pin A2
 * - Shifter Up Switch            -> Digital Pin 3 (Internal Pullup)
 * - Shifter Down Switch          -> Digital Pin 4 (Internal Pullup)
 */

// --- NATIVE USB DETECTION ---
#if defined(USBCON) || defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO) || \
    defined(ARDUINO_AVR_PROMICRO)
// For native game controller emulation, we would normally use MHeironimus's Joystick library.
// We check if it is available, otherwise we use standard Keyboard commands for shift buttons.
#define NATIVE_USB_ACTIVE 1
#if __has_include(<Joystick.h>)
#include <Joystick.h>
#define USE_JOYSTICK_LIB 1
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK, 2,
                   0,                     // Button Count, Hat Switch Count
                   true, true, true,      // X, Y, Z Axis (Throttle, Brake, Clutch)
                   false, false, false,   // No Rx, Ry, Rz
                   false, false,          // No Rudder, Throttle
                   false, false, false);  // No Accelerator, Brake, Steering
#else
#include <Keyboard.h>
#define USE_JOYSTICK_LIB 0
#endif
#else
#define NATIVE_USB_ACTIVE 0
#endif

// --- PIN DEFINITIONS ---
const int PIN_THROTTLE = A0;
const int PIN_BRAKE = A1;
const int PIN_CLUTCH = A2;
const int PIN_SHIFT_UP = 3;
const int PIN_SHIFT_DOWN = 4;
const int LED_INDICATOR = 13;

// --- CALIBRATION STRUCT ---
struct PedalCalibration {
  int minRaw;           // Minimum recorded sensor value
  int maxRaw;           // Maximum recorded sensor value
  float deadzoneLower;  // Lower deadzone percentage (e.g. 0.05 = 5%)
  float deadzoneUpper;  // Upper deadzone percentage (e.g. 0.95 = 95%)
};

// Default calibration settings (Auto-calibration will expand these)
PedalCalibration throttleCal = {200, 800, 0.05f, 0.95f};
PedalCalibration brakeCal = {100, 900, 0.08f,
                             0.92f};  // Brake has larger lower deadzone for foot resting
PedalCalibration clutchCal = {150, 850, 0.06f, 0.94f};

// --- STATE VARIABLES ---
bool calibrationMode = false;
bool printPlotterData = true;

// Simulated variables for CLI testing (if no sensors are connected)
int simThrottle = 512;
int simBrake = 512;
int simClutch = 512;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SHIFT_UP, INPUT_PULLUP);
  pinMode(PIN_SHIFT_DOWN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);

// Initialize native USB game controller
#if NATIVE_USB_ACTIVE
#if USE_JOYSTICK_LIB
  Joystick.begin();
  Joystick.setXAxisRange(0, 1023);
  Joystick.setYAxisRange(0, 1023);
  Joystick.setZAxisRange(0, 1023);
#else
  Keyboard.begin();
#endif
#endif

  Serial.println(F("=================================================="));
  Serial.println(F("Day 93: Sim Racing Pedals & Shifter Controller"));
  Serial.print(F(" Hardware Mode: "));
#if NATIVE_USB_ACTIVE
#if USE_JOYSTICK_LIB
  Serial.println(F("NATIVE USB (Joystick Library Active)"));
#else
  Serial.println(F("NATIVE USB (Keyboard Fallback for Shifter)"));
#endif
#else
  Serial.println(F("SIMULATION (ATmega328P/Uno, Serial Outputs)"));
#endif
  Serial.println(F("=================================================="));

  printMenu();
}

void loop() {
  // 1. Read Raw Sensor Inputs
  int rawT = analogRead(PIN_THROTTLE);
  int rawB = analogRead(PIN_BRAKE);
  int rawC = analogRead(PIN_CLUTCH);

  // If pins are floating/disconnected, fall back to simulated values
  if (rawT < 10 && rawB < 10 && rawC < 10) {
    rawT = simThrottle;
    rawB = simBrake;
    rawC = simClutch;
  }

  // 2. Perform Dynamic Auto-Calibration (if active)
  if (calibrationMode) {
    updateDynamicLimits(rawT, rawB, rawC);
    digitalWrite(LED_INDICATOR, (millis() / 250) % 2);  // Blink LED
  } else {
    digitalWrite(LED_INDICATOR, LOW);
  }

  // 3. Process Deadzones and Scale Inputs (0 to 1023 range)
  int outThrottle = processPedalValue(rawT, throttleCal);
  int outBrake = processPedalValue(rawB, brakeCal);
  int outClutch = processPedalValue(rawC, clutchCal);

// 4. Update Game Controller Outputs
#if NATIVE_USB_ACTIVE && USE_JOYSTICK_LIB
  Joystick.setXAxis(outThrottle);
  Joystick.setYAxis(outBrake);
  Joystick.setZAxis(outClutch);
#endif

  // 5. Read Shifter Switch States
  readShifterSwitches();

  // 6. Output Telemetry for Serial Plotter
  if (printPlotterData) {
    Serial.print(F("RawThrottle:"));
    Serial.print(rawT);
    Serial.print(F(",ThrottleOut:"));
    Serial.print(outThrottle);
    Serial.print(F(",RawBrake:"));
    Serial.print(rawB);
    Serial.print(F(",BrakeOut:"));
    Serial.print(outBrake);
    Serial.print(F(",Calibration:"));
    Serial.println(calibrationMode ? 1000 : 0);
  }

  // 7. Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCLICommand(cmd);
  }

  delay(10);  // 100 Hz update rate
}

// =============================================================
//  CORE SIGNAL PROCESSING ENGINE
// =============================================================

/**
 * Applies lower/upper deadzones and scales value to standard 10-bit range (0-1023).
 */
int processPedalValue(int raw, const PedalCalibration& cal) {
  // Constrain raw input to safety calibration limits
  int val = constrain(raw, cal.minRaw, cal.maxRaw);

  // Calculate active span
  int span = cal.maxRaw - cal.minRaw;

  // Calculate physical boundary thresholds
  int lowerThreshold = cal.minRaw + (span * cal.deadzoneLower);
  int upperThreshold = cal.minRaw + (span * cal.deadzoneUpper);

  // Apply inner deadzone (anything below is 0%)
  if (val <= lowerThreshold) {
    return 0;
  }

  // Apply outer deadzone (anything above is 100%)
  if (val >= upperThreshold) {
    return 1023;
  }

  // Linearly map the active analog span between deadzones to [0, 1023]
  return map(val, lowerThreshold, upperThreshold, 0, 1023);
}

/**
 * Auto-calibrates active range limits dynamically based on maximum travel.
 */
void updateDynamicLimits(int rawT, int rawB, int rawC) {
  // Expand Throttle limits
  if (rawT < throttleCal.minRaw) throttleCal.minRaw = rawT;
  if (rawT > throttleCal.maxRaw) throttleCal.maxRaw = rawT;

  // Expand Brake limits
  if (rawB < brakeCal.minRaw) brakeCal.minRaw = rawB;
  if (rawB > brakeCal.maxRaw) brakeCal.maxRaw = rawB;

  // Expand Clutch limits
  if (rawC < clutchCal.minRaw) clutchCal.minRaw = rawC;
  if (rawC > clutchCal.maxRaw) clutchCal.maxRaw = rawC;
}

// =============================================================
//  SHIFTER SWITCH HANDLERS
// =============================================================
void readShifterSwitches() {
  static bool prevUp = HIGH;
  static bool prevDown = HIGH;

  bool currentUp = digitalRead(PIN_SHIFT_UP);
  bool currentDown = digitalRead(PIN_SHIFT_DOWN);

  // Shift UP button pressed (Active LOW)
  if (currentUp == LOW && prevUp == HIGH) {
    Serial.println(F("[SHIFTER] Shifted UP (Gear +)"));
#if NATIVE_USB_ACTIVE
#if USE_JOYSTICK_LIB
    Joystick.setButton(0, 1);
    delay(50);
    Joystick.setButton(0, 0);
#else
    Keyboard.write('x');  // Map shift-up key to 'x'
#endif
#endif
  }
  prevUp = currentUp;

  // Shift DOWN button pressed
  if (currentDown == LOW && prevDown == HIGH) {
    Serial.println(F("[SHIFTER] Shifted DOWN (Gear -)"));
#if NATIVE_USB_ACTIVE
#if USE_JOYSTICK_LIB
    Joystick.setButton(1, 1);
    delay(50);
    Joystick.setButton(1, 0);
#else
    Keyboard.write('z');  // Map shift-down key to 'z'
#endif
#endif
  }
  prevDown = currentDown;
}

// =============================================================
//  INTERACTIVE CLI INTERFACE
// =============================================================
void handleCLICommand(char cmd) {
  if (cmd == '\n' || cmd == '\r') return;

  switch (cmd) {
    case 'c':
    case 'C':
      calibrationMode = !calibrationMode;
      if (calibrationMode) {
        Serial.println(
            F("\n[CALIBRATION] Auto-calibration STARTED. Press all pedals fully to set limits!"));
        // Reset limits to allow fresh measurement
        throttleCal.minRaw = 1023;
        throttleCal.maxRaw = 0;
        brakeCal.minRaw = 1023;
        brakeCal.maxRaw = 0;
        clutchCal.minRaw = 1023;
        clutchCal.maxRaw = 0;
      } else {
        Serial.println(F("\n[CALIBRATION] Auto-calibration STOPPED. Limits Saved:"));
        printCalStats();
      }
      break;

    case 't':
      simThrottle = constrain(simThrottle + 50, 0, 1023);
      Serial.print(F("[CLI] Sim Throttle raw incremented to: "));
      Serial.println(simThrottle);
      break;
    case 'T':
      simThrottle = constrain(simThrottle - 50, 0, 1023);
      Serial.print(F("[CLI] Sim Throttle raw decremented to: "));
      Serial.println(simThrottle);
      break;

    case 'b':
      simBrake = constrain(simBrake + 50, 0, 1023);
      Serial.print(F("[CLI] Sim Brake raw incremented to: "));
      Serial.println(simBrake);
      break;
    case 'B':
      simBrake = constrain(simBrake - 50, 0, 1023);
      Serial.print(F("[CLI] Sim Brake raw decremented to: "));
      Serial.println(simBrake);
      break;

    case 'u':
    case 'U':
      // Trigger simulated shift up
      simThrottle = 512;
      simBrake = 512;
      Serial.println(F("[CLI] Simulating hardware Shift UP..."));
      pinMode(PIN_SHIFT_UP, OUTPUT);
      digitalWrite(PIN_SHIFT_UP, LOW);
      delay(50);
      digitalWrite(PIN_SHIFT_UP, HIGH);
      pinMode(PIN_SHIFT_UP, INPUT_PULLUP);
      break;

    case 'p':
    case 'P':
      printPlotterData = !printPlotterData;
      break;

    case 'h':
    case 'H':
      printMenu();
      break;

    default:
      break;
  }
}

void printCalStats() {
  Serial.print(F("  Throttle Limits: Min="));
  Serial.print(throttleCal.minRaw);
  Serial.print(F(", Max="));
  Serial.println(throttleCal.maxRaw);
  Serial.print(F("  Brake Limits:    Min="));
  Serial.print(brakeCal.minRaw);
  Serial.print(F(", Max="));
  Serial.println(brakeCal.maxRaw);
  Serial.print(F("  Clutch Limits:   Min="));
  Serial.print(clutchCal.minRaw);
  Serial.print(F(", Max="));
  Serial.println(clutchCal.maxRaw);
}

void printMenu() {
  Serial.println(F("\n--- SIM PEDALS INTERACTIVE CLI ---"));
  Serial.println(
      F(" 'c' : Toggle Auto-Calibration Mode (press pedals fully, then send 'c' to save)"));
  Serial.println(F(" 't' / 'T' : Increase / Decrease simulated raw Throttle value"));
  Serial.println(F(" 'b' / 'B' : Increase / Decrease simulated raw Brake value"));
  Serial.println(F(" 'u' : Simulate physical Shift UP button tap"));
  Serial.println(F(" 'p' : Toggle Serial Plotter data streaming ON/OFF"));
  Serial.println(F(" 'h' : Display this help command menu"));
  Serial.println(F("----------------------------------\n"));
}

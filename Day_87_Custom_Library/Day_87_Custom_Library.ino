/*
 * 100 Projects with Arduino - Day 87
 * Project: Custom C++ Library Design (Exponential Moving Average Filter)
 * 
 * DESCRIPTION:
 * This project demonstrates how to structure, write, and package a custom C++ library
 * for the Arduino ecosystem. To demonstrate the library, we instantiate our custom
 * EMAFilter class. The sketch generates a virtual raw analog signal containing a base
 * low-frequency sine wave overlayed with high-frequency random noise (to simulate 
 * sensor noise).
 * 
 * A command-line interface (CLI) allows the user to interactively modify the filter's
 * alpha coefficient at runtime and toggle noise or test profiles. The raw and filtered
 * output are printed in a format designed for the Arduino Serial Plotter (e.g. "Raw:X,Filtered:Y").
 * 
 * THE MATHEMATICS OF EXPONENTIAL MOVING AVERAGE (EMA):
 * Unlike a Simple Moving Average (SMA), which requires storing a history of N data samples
 * in a ring buffer (heavy on SRAM memory), the EMA requires only a single memory variable:
 * the previous filtered output.
 * 
 * The difference equation is:
 *   y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
 * 
 * Where:
 *   - y[n] is the current filtered output
 *   - x[n] is the current raw sensor input
 *   - y[n-1] is the previous filtered output
 *   - alpha is the smoothing factor (0.0 <= alpha <= 1.0)
 * 
 * Key properties:
 *   - Lower alpha values (e.g., 0.05) block high-frequency noise better but introduce phase lag.
 *   - Higher alpha values (e.g., 0.8) track the signal rapidly with minimal lag, but pass more noise.
 * 
 * WIRING:
 *   - No external components required. Run using the Arduino Serial Plotter or Serial Monitor.
 */

#include "EMAFilter.h"

// Instantiate our custom filter with a default alpha of 0.1 (high smoothing)
EMAFilter myFilter(0.10f);

// --- SIMULATION VARIABLES ---
bool runningState = true;
bool noiseEnabled = true;
float noiseAmplitude = 2.0f; // Scale of random noise added to sine wave
unsigned long sampleInterval = 20; // 50 Hz sampling rate (20ms interval)

void setup() {
  Serial.begin(115200); // Use high baud rate for high-frequency plotting
  
  // Initialize seed for noise generation
  randomSeed(analogRead(A0));
  
  // Seed the filter memory on startup
  myFilter.reset(0.0f);

  // Note: Printing instructions before starting plotter format.
  // The plotter will ignore lines that don't match the variable format.
  Serial.println(F("=================================================="));
  Serial.println(F("Day 87: Custom C++ Library Design & EMA Filtering"));
  Serial.println(F("=================================================="));
  printMenu();
}

void loop() {
  static unsigned long lastSample = 0;
  
  // Run simulation loop at a precise, non-blocking frequency
  if (runningState && (millis() - lastSample >= sampleInterval)) {
    lastSample = millis();
    runFilterSimulation();
  }

  // Poll Serial interface for runtime configuration commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    handleCLICommand(cmd);
  }
}

/**
 * Generates a noisy signal, feeds it into our custom library, and prints the result.
 */
void runFilterSimulation() {
  static float angle = 0.0f;
  
  // 1. Generate clean base signal: Sine wave sweeping from -10 to +10
  float cleanSignal = 10.0f * sin(angle);
  angle += 0.05f;
  if (angle >= 2.0f * PI) {
    angle -= 2.0f * PI;
  }
  
  // 2. Add random high-frequency white noise
  float noise = 0.0f;
  if (noiseEnabled) {
    noise = (random(-100, 100) / 100.0f) * noiseAmplitude;
  }
  float rawSignal = cleanSignal + noise;
  
  // 3. Process signal using our custom library
  float filteredSignal = myFilter.filter(rawSignal);
  
  // 4. Output telemetry formatted for Arduino Serial Plotter
  // Note: Labels MUST match this exact style for Plotter compatibility
  Serial.print(F("Raw:"));
  Serial.print(rawSignal, 3);
  Serial.print(F(",Filtered:"));
  Serial.print(filteredSignal, 3);
  Serial.print(F(",Clean:"));
  Serial.println(cleanSignal, 3);
}

/**
 * Handles keyboard commands to tune the filter parameters live.
 */
void handleCLICommand(char cmd) {
  if (cmd == '\n' || cmd == '\r') return;

  float currentAlpha = myFilter.getAlpha();
  
  switch (cmd) {
    case 'p':
    case 'P':
      runningState = !runningState;
      break;
      
    case 'n':
    case 'N':
      noiseEnabled = !noiseEnabled;
      break;
      
    case '+':
    case '=':
      // Increase alpha (reduce filtering, reduce lag)
      myFilter.setAlpha(currentAlpha + 0.05f);
      break;
      
    case '-':
    case '_':
      // Decrease alpha (increase filtering, increase lag)
      myFilter.setAlpha(currentAlpha - 0.05f);
      break;
      
    case 'r':
    case 'R':
      // Reset filter accumulator
      myFilter.reset(0.0f);
      break;
      
    case 'h':
    case 'H':
      printMenu();
      break;
      
    default:
      break;
  }
}

void printMenu() {
  Serial.println(F("\n--- RUNTIME CONTROL INTERFACE ---"));
  Serial.print(F(" Active Alpha Coefficient: ")); Serial.println(myFilter.getAlpha(), 2);
  Serial.println(F(" '+' / '-' : Adjust Alpha by 0.05 (controls smoothing & lag)"));
  Serial.println(F(" 'n'       : Toggle White Noise ON/OFF"));
  Serial.println(F(" 'p'       : Pause/Resume output stream"));
  Serial.println(F(" 'r'       : Reset filter memory to 0.0"));
  Serial.println(F(" 'h'       : Print this help command reference"));
  Serial.println(F("--------------------------------------------------\n"));
}

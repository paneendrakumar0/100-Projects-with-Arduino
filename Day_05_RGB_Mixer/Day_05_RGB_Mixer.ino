/*
 * 100 Projects with Arduino - Day 5
 * Project: RGB LED Color Mixer (PWM Additive Color Mixing)
 *
 * DESCRIPTION:
 * This project demonstrates how to control a Common Cathode RGB LED to mix and generate
 * any color in the visible spectrum. Instead of hardcoding simple color steps, the sketch
 * uses a non-blocking mathematical algorithm (three phase-shifted sine waves) to cycle
 * smoothly through the entire color spectrum (the color wheel).
 * Telemetry is logged to the Serial Monitor at non-blocking intervals.
 *
 * THEORY OF OPERATION:
 * 1. An RGB LED is actually three separate LEDs (Red, Green, and Blue) packed into a single
 *    4-pin package. In a Common Cathode configuration:
 *    - Pin 1: Red Anode (positive leg)
 *    - Pin 2: Common Cathode (negative leg, connected to GND)
 *    - Pin 3: Green Anode (positive leg)
 *    - Pin 4: Blue Anode (positive leg)
 * 2. Additive Color Mixing: By adjusting the intensity (PWM duty cycle) of Red, Green, and
 *    Blue, we can mix them to create other colors:
 *    - Red (255)   + Green (255)   + Blue (0)     = Yellow
 *    - Red (255)   + Green (0)     + Blue (255)   = Magenta
 *    - Red (0)     + Green (255)   + Blue (255)   = Cyan
 *    - Red (255)   + Green (255)   + Blue (255)   = White
 * 3. Color Cycling Algorithm:
 *    We treat the color wheel as a wave. Red, Green, and Blue PWM values are calculated
 *    using sine functions phase-shifted by 120 degrees (2*pi/3 radians) from each other:
 *    - Red = sin(angle)
 *    - Green = sin(angle + 120°)
 *    - Blue = sin(angle + 240°)
 *    This ensures that as one color fades out, the next fades in, creating a smooth,
 *    continuous rainbow transition.
 *
 * WIRING:
 * - RGB Red Pin      -> 220 Ohm Resistor -> Arduino Pin 9 (PWM)
 * - RGB Cathode Pin  -> Arduino GND
 * - RGB Green Pin    -> 220 Ohm Resistor -> Arduino Pin 10 (PWM)
 * - RGB Blue Pin     -> 220 Ohm Resistor -> Arduino Pin 11 (PWM)
 *
 * IMPORTANT: You MUST use current-limiting resistors on the Red, Green, and Blue anodes.
 * Connecting them directly to 5V will burn out the LED and can damage your Arduino!
 */

// --- PIN DEFINITIONS ---
const int RED_PIN = 9;     // PWM Pin for Red channel
const int GREEN_PIN = 10;  // PWM Pin for Green channel
const int BLUE_PIN = 11;   // PWM Pin for Blue channel

// --- COLOR CYCLE PARAMETERS ---
float angle = 0.0;                  // Current angle for the sine wave cycle (in radians)
const float angleIncrement = 0.02;  // How fast we sweep through the color wheel (smaller = slower)
const unsigned long cycleDelay = 30;  // Time in ms between calculations (30ms = ~33Hz update rate)
unsigned long lastCycleTime = 0;      // Stores last time we updated the colors

// --- TELEMETRY VARIABLES ---
unsigned long lastLogTime = 0;          // Stores last time we logged data
const unsigned long logInterval = 250;  // Log to Serial Monitor 4 times a second

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Configure RGB pins as outputs
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  Serial.println("==================================================");
  Serial.println("Day 5: RGB LED Color Mixer (Sinusoidal Rainbow)");
  Serial.println("==================================================");
  Serial.println("System Initialized. Sweeping color wheel...");
}

void loop() {
  unsigned long currentTime = millis();

  // Step 1: Smoothly update the LED colors at a fixed, non-blocking rate (33Hz)
  if (currentTime - lastCycleTime >= cycleDelay) {
    lastCycleTime = currentTime;

    // Advance the angle (wraps around naturally in trigonometric functions)
    angle += angleIncrement;
    if (angle >= 2.0 * PI) {
      angle -= 2.0 * PI;
    }

    // Step 2: Calculate PWM intensities using phase-shifted sine waves.
    // sin(x) outputs a value from -1.0 to 1.0.
    // We add 1.0 (range 0.0 to 2.0) and multiply by 127.5 to scale to 0.0 to 255.0.

    // Red wave starts at 0 rad
    int rVal = (sin(angle) + 1.0) * 127.5;

    // Green wave is phase-shifted by 120 degrees (2*PI/3)
    int gVal = (sin(angle + (2.0 * PI / 3.0)) + 1.0) * 127.5;

    // Blue wave is phase-shifted by 240 degrees (4*PI/3)
    int bVal = (sin(angle + (4.0 * PI / 3.0)) + 1.0) * 127.5;

    // Step 3: Write the mixed color intensities to the PWM pins
    analogWrite(RED_PIN, rVal);
    analogWrite(GREEN_PIN, gVal);
    analogWrite(BLUE_PIN, bVal);

    // Step 4: Non-blocking telemetry print to show active mixing
    if (currentTime - lastLogTime >= logInterval) {
      lastLogTime = currentTime;

      Serial.print("Sweep Angle: ");
      Serial.print(angle * (180.0 / PI), 0);  // Print angle in degrees for readability
      Serial.print("° | PWM R: ");
      Serial.print(rVal);
      Serial.print(" | G: ");
      Serial.print(gVal);
      Serial.print(" | B: ");
      Serial.println(bVal);
    }
  }

  // Non-blocking architecture allows the microcontroller to execute other code here
}

/*
 * 100 Projects with Arduino - Day 14
 * Project: 4x4 Membrane Keypad Interfacing (Matrix Multiplexing)
 *
 * DESCRIPTION:
 * This project interfaces a 4x4 matrix membrane keypad with the Arduino. It reads keypresses
 * and outputs them to the Serial Monitor. To match professional programming practices:
 * 1. The code utilizes the standard Keypad library, which handles matrix scanning and key
 * debouncing in a clean, non-blocking manner.
 * 2. An onboard LED (Pin 13) is used to provide instant visual tactile feedback for every keypress.
 *
 * THEORY OF OPERATION:
 * 1. Row-Column Multiplexing: A standard 4x4 keypad has 16 buttons. If we connected each button
 *    to an individual Arduino pin, it would consume 16 pins, leaving no space for displays or
 * motors. Instead, we place the buttons in a 4x4 grid (Matrix), tying row contacts together and
 * column contacts together, requiring only 8 pins (4 rows + 4 columns).
 *
 *        Row 1 Pin ----- (1) --- (2) --- (3) --- (A)
 *                         |       |       |       |
 *        Row 2 Pin ----- (4) --- (5) --- (6) --- (B)
 *                         |       |       |       |
 *                         |       |       |       |
 *                      Col 1   Col 2   Col 3   Col 4
 *                       Pin     Pin     Pin     Pin
 *
 * 2. Active Matrix Scanning: The microcontroller constantly scans the matrix:
 *    - It configures all column pins as inputs with internal pull-up resistors (default HIGH).
 *    - It pulls Row 1 pin LOW (0V) while keeping Rows 2, 3, and 4 HIGH (5V).
 *    - It reads the columns. If Column 2 pin is read as LOW, it means the button at Row 1, Column 2
 *      (key '2') has been pressed, completing the electrical circuit to GND.
 *    - It repeats this for every row hundreds of times a second.
 *
 * WIRING:
 * Looking at the 8-pin female connector of the 4x4 membrane keypad from left to right:
 * - Pins 1 to 4: Rows (Row 1, Row 2, Row 3, Row 4)
 * - Pins 5 to 8: Columns (Col 1, Col 2, Col 3, Col 4)
 *
 * Connection Table:
 * - Keypad Pin 1 (Row 1) -> Arduino Pin 9
 * - Keypad Pin 2 (Row 2) -> Arduino Pin 8
 * - Keypad Pin 3 (Row 3) -> Arduino Pin 7
 * - Keypad Pin 4 (Row 4) -> Arduino Pin 6
 * - Keypad Pin 5 (Col 1) -> Arduino Pin 5
 * - Keypad Pin 6 (Col 2) -> Arduino Pin 4
 * - Keypad Pin 7 (Col 3) -> Arduino Pin 3
 * - Keypad Pin 8 (Col 4) -> Arduino Pin 2
 *
 * LIBRARY REQUIREMENT:
 * This code requires the "Keypad" library by Mark Stanley and Alexander Brevig.
 * Install it via the Arduino IDE Library Manager (Manage Libraries -> search "Keypad").
 */

#include <Keypad.h>  // Include the Matrix Keypad scanning library

// --- PIN AND MATRIX DEFINITIONS ---
const byte ROWS = 4;  // Four rows in the matrix
const byte COLS = 4;  // Four columns in the matrix

// Define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}};

// Connect Keypad Row 1, Row 2, Row 3, Row 4 to these Arduino pins
byte rowPins[ROWS] = {9, 8, 7, 6};

// Connect Keypad Col 1, Col 2, Col 3, Col 4 to these Arduino pins
byte colPins[COLS] = {5, 4, 3, 2};

// Initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// --- PIN DEFINITIONS ---
const int FEEDBACK_LED = 13;  // Onboard LED for keypress confirmation feedback

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Configure feedback LED
  pinMode(FEEDBACK_LED, OUTPUT);
  digitalWrite(FEEDBACK_LED, LOW);

  Serial.println("==================================================");
  Serial.println("Day 14: 4x4 Membrane Keypad Matrix Scanner");
  Serial.println("==================================================");
  Serial.println("System armed. Press any key on the keypad...");
}

void loop() {
  // Step 1: Query the keypad for any key activity (non-blocking scan)
  // getKey() returns 0 (NO_KEY) if no buttons are actively pressed,
  // or the character of the pressed key (e.g. '5', 'A', etc.)
  char customKey = customKeypad.getKey();

  // Step 2: Handle keypress events
  if (customKey) {
    // Visual indicator: Flash the feedback LED for 100ms (blocking flash is acceptable for simple
    // feedback, but in large loops we use a millis LED timer. Here we keep it simple for immediate
    // response)
    digitalWrite(FEEDBACK_LED, HIGH);
    delay(100);
    digitalWrite(FEEDBACK_LED, LOW);

    // Log keypress to Serial Monitor
    Serial.print("[KEYPRESS] Detected: '");
    Serial.print(customKey);
    Serial.println("'");
  }

  // Non-blocking architecture: no delay() in the main loop means this runs at maximum CPU speed.
}

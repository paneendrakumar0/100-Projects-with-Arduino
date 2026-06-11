/*
 * 100 Projects with Arduino - Day 24
 * Project: Single-Digit 7-Segment Display Counter (Binary Bit-Mask Arrays)
 *
 * DESCRIPTION:
 * This project interfaces a single-digit 7-segment LED display with the Arduino to count from
 * 0 to 9. To meet professional mechatronics standards, this sketch implements:
 * 1. Binary Bit-Mask Array: Instead of writing separate digitalWrite commands for each segment
 *    manually, the segment patterns are encoded as bits inside a byte array (0bGPFEDCBA).
 *    A custom parser unpacks this byte to drive the pins.
 * 2. Non-blocking timing: The counter increments once per second using millis().
 * 3. Modular configuration: Support for both Common Cathode and Common Anode displays via a
 *    software configuration flag.
 *
 * THEORY OF OPERATION:
 * 1. 7-Segment Construction: The display consists of 8 separate LEDs (segments A, B, C, D, E, F, G,
 *    and a decimal point DP) arranged in a figure-8 pattern.
 * 2. Common Cathode (Default): The negative terminals (cathodes) of all 8 LEDs are tied together
 *    and connected to GND. Individual segments are turned ON by writing HIGH (5V) to their anode
 * pins.
 * 3. Binary Bit-Mapping:
 *    We map each segment to a specific bit in a single byte:
 *    - Bit 0 -> Segment A (Top)
 *    - Bit 1 -> Segment B (Top-Right)
 *    - Bit 2 -> Segment C (Bottom-Right)
 *    - Bit 3 -> Segment D (Bottom)
 *    - Bit 4 -> Segment E (Bottom-Left)
 *    - Bit 5 -> Segment F (Top-Left)
 *    - Bit 6 -> Segment G (Middle)
 *    - Bit 7 -> Decimal Point DP (Not used here, set to 0)
 *    Pattern for '0' (A, B, C, D, E, F ON; G OFF): `0b00111111` (0x3F)
 *    Pattern for '1' (B, C ON; others OFF):         `0b00000110` (0x06)
 *
 * WIRING (Common Cathode Display):
 * Connect pins 2-8 to their respective segments through 220 Ohm current-limiting resistors.
 * - Segment A  -> 220 Ohm Resistor -> Arduino Pin 2
 * - Segment B  -> 220 Ohm Resistor -> Arduino Pin 3
 * - Segment C  -> 220 Ohm Resistor -> Arduino Pin 4
 * - Segment D  -> 220 Ohm Resistor -> Arduino Pin 5
 * - Segment E  -> 220 Ohm Resistor -> Arduino Pin 6
 * - Segment F  -> 220 Ohm Resistor -> Arduino Pin 7
 * - Segment G  -> 220 Ohm Resistor -> Arduino Pin 8
 * - Common pin -> Arduino GND
 */

// --- CONFIGURATION: DISPLAY TYPE ---
// Set to 'true' for Common Cathode displays (default),
// or 'false' for Common Anode displays.
#define IS_COMMON_CATHODE true

// --- PIN DEFINITIONS ---
// Map segment A, B, C, D, E, F, G in order to Arduino pin numbers
const int segmentPins[] = {2, 3, 4, 5, 6, 7, 8};
const int TOTAL_SEGMENTS = 7;

// --- BINARY BIT-MASK VALUES ---
// Bit arrangement: 0b0GFEDCBA (Bit 7 is empty, Bit 6 is G, Bit 0 is A)
const byte numPatterns[] = {
    0b00111111,  // 0: A, B, C, D, E, F ON
    0b00000110,  // 1: B, C ON
    0b01011011,  // 2: A, B, D, E, G ON
    0b01001111,  // 3: A, B, C, D, G ON
    0b01100110,  // 4: B, C, F, G ON
    0b01101101,  // 5: A, C, D, F, G ON
    0b01111101,  // 6: A, C, D, E, F, G ON
    0b00000111,  // 7: A, B, C ON
    0b01111111,  // 8: All segments ON
    0b01101111   // 9: A, B, C, D, F, G ON
};

// --- SCHEDULER VARIABLES ---
unsigned long lastIncrementTime = 0;    // Stores last counter update timestamp
const unsigned long countDelay = 1000;  // Count speed in ms (1 second)

// --- STATE VARIABLES ---
int currentNumber = 0;  // The active number (0 - 9) to display

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Configure all segment pins as output pins using a loop
  for (int i = 0; i < TOTAL_SEGMENTS; i++) {
    pinMode(segmentPins[i], OUTPUT);

    // Set segments to default OFF state immediately
    digitalWrite(segmentPins[i], IS_COMMON_CATHODE ? LOW : HIGH);
  }

  Serial.println("==================================================");
  Serial.println("Day 24: Single-Digit 7-Segment Display Counter");
  Serial.println("==================================================");
  Serial.println("System armed. Commencing count 0-9...");

  // Show initial number (0)
  displayDigit(currentNumber);
}

void loop() {
  unsigned long currentTime = millis();

  // Step 1: Increment number at scheduled non-blocking interval (every 1 second)
  if (currentTime - lastIncrementTime >= countDelay) {
    lastIncrementTime = currentTime;

    currentNumber++;
    if (currentNumber > 9) {
      currentNumber = 0;  // Reset count
      Serial.println("--- Loop reset ---");
    }

    // Step 2: Write pattern to display
    displayDigit(currentNumber);
  }
}

/**
 * Parses the binary bitmask for a digit and applies it to the digital output pins.
 */
void displayDigit(int number) {
  // Retrieve the byte pattern for the target number
  byte pattern = numPatterns[number];

  // Iterate through each of the 7 segment pins
  for (int i = 0; i < TOTAL_SEGMENTS; i++) {
    // Read the specific bit corresponding to segment 'i' (Bit 0 for A, Bit 1 for B, etc.)
    // bitRead(x, n) returns 0 or 1.
    int segmentState = bitRead(pattern, i);

// If Common Anode, invert the bit (LOW turns LED ON, HIGH turns LED OFF)
#if !IS_COMMON_CATHODE
    segmentState = !segmentState;
#endif

    // Write state to the physical pin
    digitalWrite(segmentPins[i], segmentState);
  }

  // Log telemetry
  Serial.print("Displaying: ");
  Serial.print(number);
  Serial.print(" | Bit-Mask: 0b");
  // Print byte in binary formatting for student clarity
  for (int b = 6; b >= 0; b--) {
    Serial.print(bitRead(pattern, b));
  }
  Serial.println();
}

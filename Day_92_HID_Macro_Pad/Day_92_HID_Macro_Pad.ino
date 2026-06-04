/*
 * 100 Projects with Arduino - Day 92
 * Project: USB HID Macro Pad with Diode Matrix (Cherry MX Switches)
 * 
 * DESCRIPTION:
 * This project implements a 6-key custom Macro Pad arranged in a 2x3 Matrix. It demonstrates 
 * how professional keyboards scan mechanical switches in columns and rows to save I/O pins,
 * and how diodes prevent "ghosting" (false keypress detection) when multiple keys are pressed.
 * 
 * CORE FEATURES:
 * 1. 2x3 Keyboard Matrix Scanning: Scans 2 rows and 3 columns using only 5 microcontroller pins.
 * 2. Anti-Ghosting Diode Logic: Analyzes column read states while pulling rows LOW.
 * 3. Multi-Key Macros: Sends system hotkeys (Ctrl+C, Ctrl+V, Ctrl+Z, etc.) over USB HID.
 * 4. Dual Mode: Works as a native USB macro pad on Leonardo/Micro, or as a fully simulated
 *    serial matrix parser on Uno/Mega.
 * 
 * THE PHYSICS OF KEYBOARD MATRIX SCANNING & GHOSTING:
 * If we wired 100 keys directly to pins, we would need 100 pins. Instead, a matrix wires keys at 
 * the intersections of Row and Column lines.
 * 
 * Matrix Scanning Sequence:
 * 1. Column pins are configured as INPUT_PULLUP (HIGH by default).
 * 2. Row pins are configured as INPUT (High impedance) to avoid conflicts.
 * 3. To scan Row 0, we set Row 0 pin to OUTPUT and write it LOW.
 * 4. We read Column pins 0, 1, and 2. If a column is LOW, the key at (Row 0, Col X) is pressed.
 * 5. Set Row 0 back to INPUT (or HIGH).
 * 6. Repeat for Row 1.
 * 
 * Ghosting occurs when 3 keys sharing rows and columns are pressed, forming a closed loop that
 * tricks the microcontroller into thinking a 4th key is pressed. Placing a signal diode (e.g., 1N4148)
 * in series with every switch allows current to flow in only one direction, breaking the sneak path.
 * 
 * WIRING:
 * - Rows: Row 0 -> Pin 5, Row 1 -> Pin 6
 * - Columns: Col 0 -> Pin 7, Col 1 -> Pin 8, Col 2 -> Pin 9
 * - Diode Orientation: Anode to key switch, Cathode to Row pin.
 */

// --- NATIVE USB DETECTION ---
#if defined(USBCON) || defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO) || defined(ARDUINO_AVR_PROMICRO)
  #include <Keyboard.h>
  #define NATIVE_USB_ACTIVE 1
#else
  #define NATIVE_USB_ACTIVE 0
#endif

// --- MATRIX DEFINITIONS ---
const int NUM_ROWS = 2;
const int NUM_COLS = 3;

// Pin mapping
const int rowPins[NUM_ROWS] = {5, 6};    // Row 0, Row 1
const int colPins[NUM_COLS] = {7, 8, 9}; // Col 0, Col 1, Col 2

// Tracking active switch states (false = released, true = pressed)
bool keyStates[NUM_ROWS][NUM_COLS] = {
  {false, false, false},
  {false, false, false}
};

// Macro profiles assigned to each matrix key
struct MacroKey {
  const char* label;
  void (*triggerAction)();
};

void actionCopy();
void actionPaste();
void actionUndo();
void actionSave();
void actionBrowserHome();
void actionLockScreen();

const MacroKey macroMap[NUM_ROWS][NUM_COLS] = {
  { {"Ctrl+C (Copy)",  actionCopy},  {"Ctrl+V (Paste)", actionPaste}, {"Ctrl+Z (Undo)",  actionUndo} },
  { {"Ctrl+S (Save)",  actionSave},  {"Browser Home",   actionBrowserHome}, {"Lock PC (GUI+L)", actionLockScreen} }
};

void setup() {
  Serial.begin(9600);

  // Initialize Matrix Pins
  for (int r = 0; r < NUM_ROWS; r++) {
    pinMode(rowPins[r], INPUT); // Set as high-impedance input when idle
  }
  for (int c = 0; c < NUM_COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP); // Active low pullups
  }

  #if NATIVE_USB_ACTIVE
    Keyboard.begin();
  #endif

  Serial.println(F("=================================================="));
  Serial.println(F("Day 92: USB HID Diode-Matrix Cherry MX Macro Pad"));
  Serial.print(F(" Hardware Mode: "));
  #if NATIVE_USB_ACTIVE
    Serial.println(F("NATIVE USB (ATmega32U4/Leonardo active)"));
  #else
    Serial.println(F("SIMULATION (ATmega328P/Uno, Serial Outputs)"));
  #endif
  Serial.println(F("=================================================="));
  
  printMenu();
}

void loop() {
  // 1. Scan Keyboard Matrix
  scanKeyboardMatrix();

  // 2. Poll Serial Commands for Interactive Simulation
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;
    
    // Commands 1-6 simulate pressing key switches (0,0) through (1,2)
    if (cmd >= '1' && cmd <= '6') {
      int idx = cmd - '1';
      int r = idx / NUM_COLS;
      int c = idx % NUM_COLS;
      
      Serial.print(F("[SIMULATED PRESS] Key "));
      Serial.print(cmd);
      Serial.print(F(" at Matrix ("));
      Serial.print(r);
      Serial.print(F(","));
      Serial.print(c);
      Serial.print(F(") -> "));
      Serial.println(macroMap[r][c].label);
      
      macroMap[r][c].triggerAction();
    } else if (cmd == 'h' || cmd == 'H') {
      printMenu();
    }
  }
  
  delay(10); // Throttle loop slightly for bounce stability
}

// =============================================================
//  MATRIX SCANNING ENGINE
// =============================================================
void scanKeyboardMatrix() {
  for (int r = 0; r < NUM_ROWS; r++) {
    // 1. Activate Row: set as output and pull LOW
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], LOW);
    
    // Short delay to let electrical logic state stabilize (parasitic capacitance)
    delayMicroseconds(10);
    
    // 2. Read Columns
    for (int c = 0; c < NUM_COLS; c++) {
      bool isPressed = (digitalRead(colPins[c]) == LOW);
      
      // State change detection (rising/falling edge)
      if (isPressed != keyStates[r][c]) {
        delay(10); // Debounce delay
        
        // Re-read after debounce window to confirm
        isPressed = (digitalRead(colPins[c]) == LOW);
        if (isPressed != keyStates[r][c]) {
          keyStates[r][c] = isPressed;
          
          if (isPressed) {
            handleKeyPress(r, c);
          } else {
            handleKeyRelease(r, c);
          }
        }
      }
    }
    
    // 3. Deactivate Row: set back to high-impedance INPUT
    digitalWrite(rowPins[r], HIGH);
    pinMode(rowPins[r], INPUT);
  }
}

void handleKeyPress(int r, int c) {
  Serial.print(F("[MATRIX] Physical Press detected at Row "));
  Serial.print(r);
  Serial.print(F(", Col "));
  Serial.print(c);
  Serial.print(F(" -> Triggering Macro: "));
  Serial.println(macroMap[r][c].label);
  
  macroMap[r][c].triggerAction();
}

void handleKeyRelease(int r, int c) {
  Serial.print(F("[MATRIX] Physical Release detected at Row "));
  Serial.print(r);
  Serial.print(F(", Col "));
  Serial.println(c);
}

// =============================================================
//  MACRO PAYLOAD ACTIONS
// =============================================================

void actionCopy() {
  #if NATIVE_USB_ACTIVE
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('c');
    delay(20);
    Keyboard.releaseAll();
  #else
    Serial.println(F("[SIMULATED KEYBOARD] Sending: CTRL + C (Copy)"));
  #endif
}

void actionPaste() {
  #if NATIVE_USB_ACTIVE
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('v');
    delay(20);
    Keyboard.releaseAll();
  #else
    Serial.println(F("[SIMULATED KEYBOARD] Sending: CTRL + V (Paste)"));
  #endif
}

void actionUndo() {
  #if NATIVE_USB_ACTIVE
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('z');
    delay(20);
    Keyboard.releaseAll();
  #else
    Serial.println(F("[SIMULATED KEYBOARD] Sending: CTRL + Z (Undo)"));
  #endif
}

void actionSave() {
  #if NATIVE_USB_ACTIVE
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('s');
    delay(20);
    Keyboard.releaseAll();
  #else
    Serial.println(F("[SIMULATED KEYBOARD] Sending: CTRL + S (Save File)"));
  #endif
}

void actionBrowserHome() {
  #if NATIVE_USB_ACTIVE
    // Standard keyboard shortcut to focus browser URL bar is Alt+D or Ctrl+L
    // We send Ctrl+T to open a new tab
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('t');
    delay(20);
    Keyboard.releaseAll();
  #else
    Serial.println(F("[SIMULATED KEYBOARD] Sending: CTRL + T (Open Browser Tab)"));
  #endif
}

void actionLockScreen() {
  #if NATIVE_USB_ACTIVE
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press('l');
    delay(20);
    Keyboard.releaseAll();
  #else
    Serial.println(F("[SIMULATED KEYBOARD] Sending: WIN_GUI + L (Lock Computer)"));
  #endif
}

void printMenu() {
  Serial.println(F("\n--- MACRO PAD SIMULATION CLI ---"));
  Serial.println(F(" Send number keys '1' to '6' to simulate pressing the matrix switches:"));
  Serial.println(F(" '1' : Key (0,0) -> Ctrl+C (Copy)"));
  Serial.println(F(" '2' : Key (0,1) -> Ctrl+V (Paste)"));
  Serial.println(F(" '3' : Key (0,2) -> Ctrl+Z (Undo)"));
  Serial.println(F(" '4' : Key (1,0) -> Ctrl+S (Save)"));
  Serial.println(F(" '5' : Key (1,1) -> Browser Open Tab"));
  Serial.println(F(" '6' : Key (1,2) -> Lock System"));
  Serial.println(F(" 'h' : Display this help command list"));
  Serial.println(F("---------------------------------\n"));
}

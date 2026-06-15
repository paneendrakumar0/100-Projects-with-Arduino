/*
 * 100 Projects with Arduino - Day 32
 * Project: Quadrature Rotary Encoder (Interrupt-Driven Menu Selector)
 *
 * DESCRIPTION:
 * This project interfaces a KY-040 rotary encoder using hardware interrupts.
 * To implement a robust HMI (Human-Machine Interface) system, this sketch:
 * 1. Implements hardware interrupt-driven quadrature decoding on digital pin 2 (external interrupt
 * 0).
 * 2. Employs microsecond-based software lockout debouncing in the Interrupt Service Routine (ISR)
 *    to filter out mechanical contact bounce.
 * 3. Incorporates software debouncing on the integrated push-button switch (SW) in the main loop.
 * 4. Implements an interactive, state-driven 4-option menu system printed to the Serial Monitor.
 *    Rotating the encoder moves the menu selector, and pressing the shaft button selects the menu
 * item.
 *
 * ENCODER PHYSICS & THEORY:
 * - Quadrature Decoding: A rotary encoder is a sensor that detects rotational position and
 * direction. Inside, there is a slotted disk connected to common ground, and two contact brushes
 * (Channel A/CLK and Channel B/DT). When rotated, they generate two square wave signals out of
 * phase by 90 degrees (Quadrature Encoding).
 *   - Clockwise (CW): Channel A leads Channel B. When Channel A goes from HIGH to LOW, Channel B is
 * still HIGH.
 *   - Counter-Clockwise (CCW): Channel B leads Channel A. When Channel A goes from HIGH to LOW,
 * Channel B is LOW.
 * - Hardware Interrupts: Polling encoder pins inside loop() will miss pulses if the processor is
 * busy. By attaching an interrupt to Pin 2, the processor halts execution of loop() instantly when
 * a state change occurs, running the ISR (Interrupt Service Routine) to update the position vector
 * without lag.
 * - Debouncing: Mechanical switches bounce (vibrate) for several milliseconds upon transition,
 * causing false triggers. We implement a software lockout window (e.g., 2000 microseconds) in the
 * ISR to discard false bounce transitions.
 *
 * HARDWARE CONNECTIONS:
 * - KY-040 Encoder Pin -> Arduino Uno Pin
 *   - CLK (Output A)   -> Pin 2 (Must be Pin 2 or 3 for hardware interrupts on Uno)
 *   - DT (Output B)    -> Pin 4
 *   - SW (Push Button) -> Pin 3 (Pull-up input)
 *   - VCC              -> 5V
 *   - GND              -> GND
 */

// --- PIN DEFINITIONS ---
const int CLK_PIN = 2;  // Encoder Output A (Interrupt Pin)
const int DT_PIN = 4;   // Encoder Output B
const int SW_PIN = 3;   // Integrated Push Button

// --- VOLATILE ENCODER STATE VARIABLES ---
// Volatile variables are modified in the ISR and must be declared as such so the compiler
// doesn't optimize them out or store them in registers.
volatile int encoderPosition = 0;
volatile bool encoderChanged = false;
volatile unsigned long lastISRTime = 0;  // Timestamp for ISR debouncing (microseconds)

// --- MENU STATE MACHINE ---
const int MENU_SIZE = 4;
const char *menuItems[MENU_SIZE] = {"System Status Monitor", "Temperature Diagnostic",
                                    "Wipe Non-Volatile Memory", "Hardware System Information"};
int currentSelection = 0;

// --- BUTTON DEBOUNCE STATE ---
int lastButtonState = HIGH;
unsigned long lastButtonDebounce = 0;
const unsigned long debounceDelay = 50;  // ms

void setup() {
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 32: Quadrature Rotary Encoder HMI Controller");
  Serial.println("==================================================");

  // Setup input pins. Note: KY-040 board usually has pullup resistors,
  // but using INPUT_PULLUP ensures reliability.
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);

  // Attach external interrupt on Pin 2 (CLK) triggering on a FALLING edge
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), handleEncoderISR, FALLING);

  Serial.println("[SYSTEM] Interrupt attached. Rotary Encoder active.");
  renderMenu();
}

void loop() {
  // Step 1: Check if encoder has moved (updated in ISR)
  if (encoderChanged) {
    // Disable interrupts briefly while reading volatile multibyte variables to prevent data tearing
    noInterrupts();
    int positionDelta = encoderPosition;
    encoderPosition = 0;  // Clear delta accumulator
    encoderChanged = false;
    interrupts();

    // Map encoder rotation to menu selection index
    currentSelection += positionDelta;

    // Constrain menu selection index within range (circular wrap-around)
    if (currentSelection < 0) {
      currentSelection = MENU_SIZE - 1;
    } else if (currentSelection >= MENU_SIZE) {
      currentSelection = 0;
    }

    // Redraw menu with new selector position
    renderMenu();
  }

  // Step 2: Read and debounce integrated push button
  int reading = digitalRead(SW_PIN);
  if (reading != lastButtonState) {
    lastButtonDebounce = millis();
  }

  if ((millis() - lastButtonDebounce) > debounceDelay) {
    // If the button state has changed stably
    if (reading == LOW && lastButtonState == HIGH) {
      // Button was pressed (LOW due to pullup resistor configuration)
      executeMenuAction(currentSelection);
    }
  }
  lastButtonState = reading;
}

/**
 * INTERRUPT SERVICE ROUTINE (ISR)
 * Executed instantly on Pin 2 falling edge. Keeps operations minimal and fast.
 */
void handleEncoderISR() {
  unsigned long currentTime = micros();

  // Software lockout debouncing (ignore pulses occurring within 2000 microseconds)
  if (currentTime - lastISRTime > 2000) {
    // Read Channel B (DT_PIN) to determine rotation direction
    int dtVal = digitalRead(DT_PIN);

    if (dtVal == HIGH) {
      encoderPosition++;  // Clockwise rotation
    } else {
      encoderPosition--;  // Counter-Clockwise rotation
    }

    lastISRTime = currentTime;
    encoderChanged = true;
  }
}

/**
 * Renders the selection menu in the Serial Monitor.
 */
void renderMenu() {
  // Print clear line feeds to "scroll" the menu down in the terminal
  Serial.println("\n\n\n\n\n");
  Serial.println("====== MAIN SYSTEM MENU ======");
  Serial.println("Rotate encoder to scroll. Press button to select.");
  Serial.println("--------------------------------");

  for (int i = 0; i < MENU_SIZE; i++) {
    if (i == currentSelection) {
      Serial.print(" -> [*] ");  // Active item indicator
    } else {
      Serial.print("    [ ] ");
    }
    Serial.println(menuItems[i]);
  }
  Serial.println("==============================");
}

/**
 * Simulates system responses based on selected menu choices.
 */
void executeMenuAction(int selection) {
  Serial.println("\n>>> EVENT: Select Button Pressed!");
  Serial.print(">>> RUNNING: ");
  Serial.println(menuItems[selection]);
  Serial.println("--------------------------------");

  switch (selection) {
    case 0:
      Serial.println("[STATUS] CPU Temp: Normal | Voltage: 5.01V | Duty Cycle: 100%");
      break;
    case 1:
      Serial.println("[DIAGNOSTIC] Calibrating temp offsets... Sensor read: 24.50 C");
      break;
    case 2:
      Serial.println("[MEMORY] ERASING NV RAM... [OK] System formatted successfully.");
      break;
    case 3:
      Serial.println("[INFO] Model: ATmega328P-PU | Clock: 16MHz | Firmware: v1.3.2");
      break;
  }
  Serial.println("--------------------------------");

  // Pause brief moment to allow user to read selection confirmation, then redraw menu
  delay(2500);
  renderMenu();
}

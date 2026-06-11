/*
 * 100 Projects with Arduino - Day 90
 * Project: Arduino as a USB HID (Human Interface Device) Keyboard & Mouse
 *
 * DESCRIPTION:
 * This project demonstrates how to turn an Arduino into a native USB Keyboard and Mouse
 * using USB Human Interface Device (HID) emulation.
 *
 * DESIGN FOR COMPATIBILITY (NATIVE vs SIMULATED):
 * 1. Native Mode: If compiled for a board with native USB support (e.g., ATmega32U4 like
 *    Leonardo, Micro, or Pro Micro), it acts as a real USB keyboard and mouse.
 * 2. Simulation Mode: If compiled for standard Arduino Uno (ATmega328P) or Mega, it
 *    detects the lack of hardware USB and falls back to a Serial Simulation CLI, printing
 *    simulated keystrokes and mouse movements to the Serial Monitor.
 *
 * DEMONSTRATED FEATURES:
 * - PC Keep-Alive "Mouse Jiggler": Moves the mouse cursor back and forth periodically.
 * - Text Typer: Types out a predefined string as if entered from a keyboard.
 * - Shortcut Hotkeys: Toggles Alt+Tab (App Switcher) or GUI+L (Lock Screen).
 * - Pins 2 & 3: Attached to physical pushbuttons (uses internal pull-ups) to trigger actions.
 * - Serial CLI: Allows triggering all actions via keyboard commands over the Serial Monitor.
 *
 * SAFETY WARNING:
 * When programming USB HID devices, always add a "Safety Disarm Switch" (e.g., holding a pin LOW
 * during boot) or a startup delay (e.g., 5 seconds) before sending keystrokes. Otherwise, if you
 * write an infinite typing loop, you can lock yourself out of your PC or write dangerous commands!
 *
 * WIRING:
 * - Pin 2: Button A (Trigger Text Type / Alt-Tab) -> Connect to GND
 * - Pin 3: Button B (Toggle Mouse Jiggler)        -> Connect to GND
 */

// --- NATIVE USB DETECTION ---
#if defined(USBCON) || defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO) || \
    defined(ARDUINO_AVR_PROMICRO)
#include <Keyboard.h>
#include <Mouse.h>
#define NATIVE_USB_ACTIVE 1
#else
#define NATIVE_USB_ACTIVE 0
#endif

// --- PIN DEFINITIONS ---
const int BUTTON_A_PIN = 2;    // Keystroke trigger
const int BUTTON_B_PIN = 3;    // Jiggler toggle
const int LED_INDICATOR = 13;  // Status LED

// --- STATE VARIABLES ---
bool jigglerEnabled = false;
unsigned long lastJiggleTime = 0;
const unsigned long JIGGLE_INTERVAL_MS = 5000;  // Jiggle mouse every 5 seconds

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);

// Initialize USB HID interface if native hardware exists
#if NATIVE_USB_ACTIVE
  Keyboard.begin();
  Mouse.begin();
#endif

  // --- SAFETY DELAY ---
  // Gives the user 5 seconds to upload new code or disconnect the USB cable
  // before the Arduino starts emulation.
  for (int i = 5; i > 0; i--) {
    digitalWrite(LED_INDICATOR, HIGH);
    delay(100);
    digitalWrite(LED_INDICATOR, LOW);
    delay(900);
  }

  Serial.println(F("=================================================="));
  Serial.println(F("Day 90: USB HID Keyboard & Mouse Emulation"));
  Serial.print(F(" Hardware Mode: "));
#if NATIVE_USB_ACTIVE
  Serial.println(F("NATIVE USB (ATmega32U4 detected)"));
#else
  Serial.println(F("SIMULATION (ATmega328P/Uno, outputting to Serial)"));
#endif
  Serial.println(F("=================================================="));

  printMenu();
}

void loop() {
  // 1. Process Periodical Mouse Jiggler
  if (jigglerEnabled && (millis() - lastJiggleTime >= JIGGLE_INTERVAL_MS)) {
    lastJiggleTime = millis();
    triggerMouseJiggle();
  }

  // 2. Poll Physical Buttons (Active LOW)
  static bool prevBtnA = HIGH;
  static bool prevBtnB = HIGH;

  bool currentBtnA = digitalRead(BUTTON_A_PIN);
  bool currentBtnB = digitalRead(BUTTON_B_PIN);

  // Button A: Types text
  if (currentBtnA == LOW && prevBtnA == HIGH) {
    delay(50);  // Simple debounce
    if (digitalRead(BUTTON_A_PIN) == LOW) {
      typeTextSequence();
    }
  }
  prevBtnA = currentBtnA;

  // Button B: Toggles Jiggler
  if (currentBtnB == LOW && prevBtnB == HIGH) {
    delay(50);  // Simple debounce
    if (digitalRead(BUTTON_B_PIN) == LOW) {
      toggleJiggler();
    }
  }
  prevBtnB = currentBtnB;

  // 3. Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 't':
      case 'T':
        typeTextSequence();
        break;
      case 'j':
      case 'J':
        toggleJiggler();
        break;
      case 'l':
      case 'L':
        triggerLockScreen();
        break;
      case 'a':
      case 'A':
        triggerAltTab();
        break;
      case 'h':
      case 'H':
        printMenu();
        break;
      default:
        break;
    }
  }
}

// =============================================================
//  USB HID EMULATION LAYER
// =============================================================

/**
 * Types out a message.
 */
void typeTextSequence() {
  Serial.println(F("[HID] Typing text sequence..."));

#if NATIVE_USB_ACTIVE
  Keyboard.println(F("Hello from Day 90 of Arduino 100-Day Masterclass!"));
#else
  Serial.println(
      F("[SIMULATED KEYBOARD WRITE] \"Hello from Day 90 of Arduino 100-Day Masterclass!\\n\""));
#endif
}

/**
 * Toggles the state of the mouse jiggler.
 */
void toggleJiggler() {
  jigglerEnabled = !jigglerEnabled;
  digitalWrite(LED_INDICATOR, jigglerEnabled ? HIGH : LOW);

  Serial.print(F("[HID] Mouse Jiggler: "));
  Serial.println(jigglerEnabled ? F("ENABLED") : F("DISABLED"));
}

/**
 * Periodically moves the mouse 5 pixels to the right and back to the left
 * to prevent the operating system from falling asleep or locking.
 */
void triggerMouseJiggle() {
  Serial.println(F("[HID] Jiggling Mouse cursor."));

#if NATIVE_USB_ACTIVE
  Mouse.move(5, 0, 0);
  delay(100);
  Mouse.move(-5, 0, 0);
#else
  Serial.println(F("[SIMULATED MOUSE MOVE] X: +5, Y: 0"));
  delay(100);
  Serial.println(F("[SIMULATED MOUSE MOVE] X: -5, Y: 0"));
#endif
}

/**
 * Sends a system Lock shortcut.
 * - Windows/Linux: Win + L
 * - macOS: Cmd + Ctrl + Q
 */
void triggerLockScreen() {
  Serial.println(F("[HID] Sending Lock Screen shortcut (Win+L)..."));

#if NATIVE_USB_ACTIVE
  // KEY_LEFT_GUI is the Windows/Command key
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('l');
  delay(50);
  Keyboard.releaseAll();
#else
  Serial.println(F("[SIMULATED KEYBOARD HOTKEY] WIN_KEY + 'l'"));
#endif
}

/**
 * Sends an Alt+Tab shortcut (Application Switcher).
 */
void triggerAltTab() {
  Serial.println(F("[HID] Sending App Switcher shortcut (Alt+Tab)..."));

#if NATIVE_USB_ACTIVE
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(KEY_TAB);
  delay(50);
  Keyboard.releaseAll();
#else
  Serial.println(F("[SIMULATED KEYBOARD HOTKEY] ALT_KEY + TAB_KEY"));
#endif
}

void printMenu() {
  Serial.println(F("\n--- USB HID SIMULATOR CLI ---"));
  Serial.println(F(" 't' : Type a test string (Keyboard typing)"));
  Serial.println(F(" 'j' : Toggle Mouse Jiggler (Periodically moves cursor)"));
  Serial.println(F(" 'l' : Trigger PC Lock Screen (Win+L hotkey)"));
  Serial.println(F(" 'a' : Trigger App Switcher (Alt+Tab hotkey)"));
  Serial.println(F(" 'h' : Display this help menu"));
  Serial.println(F("------------------------------\n"));
}

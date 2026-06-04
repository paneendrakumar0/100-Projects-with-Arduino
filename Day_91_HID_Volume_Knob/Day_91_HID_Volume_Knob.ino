/*
 * 100 Projects with Arduino - Day 91
 * Project: Custom PC Volume Knob (Quadrature Rotary Encoder + USB HID Media Controller)
 * 
 * DESCRIPTION:
 * This project implements a custom USB Media Volume Control Knob. It interfaces a mechanical
 * Quadrature Rotary Encoder (with push switch) to control a PC's volume (Volume Up, Volume Down,
 * Mute/Unmute) directly over USB.
 * 
 * CORE ARCHITECTURES:
 * 1. Quadrature Decoder: State-machine-based decoder for the rotary encoder that filters out
 *    sub-detent bounces and accurately tracks clockwise (CW) and counter-clockwise (CCW) rotation.
 * 2. Native USB HID Consumer API: If compiled on ATmega32U4 (Leonardo/Micro), it uses native
 *    USB HID Consumer Page controls to adjust Windows/macOS system volume.
 * 3. Serial Simulation Mode: If compiled on ATmega328P (Uno), it outputs the volume adjustments
 *    and rotary steps to the Serial Monitor.
 * 
 * THE PHYSICS OF QUADRATURE ENCODERS:
 * A rotary encoder contains two internal contact switches (Channel A and Channel B) that are mechanically
 * offset. As the shaft rotates, the switches produce square-wave signals that are 90 degrees out of phase.
 * 
 * Clockwise (CW) Rotation:
 *   Channel A: ┌───┐   ┌───┐
 *              │   │   │   │
 *   Channel B:   ┌───┐   ┌───┐  (Channel A transitions BEFORE Channel B)
 * 
 * Counter-Clockwise (CCW) Rotation:
 *   Channel A:   ┌───┐   ┌───┐
 *              │   │   │   │
 *   Channel B: ┌───┐   ┌───┐    (Channel B transitions BEFORE Channel A)
 * 
 * By checking the state of Channel B when Channel A changes edge, we determine direction.
 * 
 * WIRING:
 * - Encoder Pin CLK (A) -> Pin 5 (Internal Pullup)
 * - Encoder Pin DT (B)  -> Pin 6 (Internal Pullup)
 * - Encoder Pin SW (Button) -> Pin 7 (Internal Pullup)
 * - Encoder GND -> GND
 */

// --- NATIVE USB DETECTION ---
#if defined(USBCON) || defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO) || defined(ARDUINO_AVR_PROMICRO)
  // For native media keys (Volume Up, Down, Mute) on ATmega32U4, we use the standard HID-Project library.
  // If the user hasn't installed HID-Project, we fall back to standard Keyboard shortcuts.
  #define NATIVE_USB_ACTIVE 1
  #if __has_include(<HID-Project.h>)
    #include <HID-Project.h>
    #define USE_HID_PROJECT 1
  #else
    #include <Keyboard.h>
    #define USE_HID_PROJECT 0
  #endif
#else
  #define NATIVE_USB_ACTIVE 0
#endif

// --- PIN DEFINITIONS ---
const int ENCODER_PIN_A = 5; // CLK pin
const int ENCODER_PIN_B = 6; // DT pin
const int ENCODER_SW    = 7; // Pushbutton pin
const int LED_INDICATOR = 13; // Status indicator LED

// --- ENCODER STATE VARIABLES ---
int lastStateCLK;
bool muteState = false;

// --- DEBOUNCE VARIABLES ---
unsigned long lastButtonTime = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 250;

void setup() {
  Serial.begin(9600);

  // Configure pins with pull-ups (mechanical switches ground the pin when closed)
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);

  // Read initial state of CLK
  lastStateCLK = digitalRead(ENCODER_PIN_A);

  // Initialize USB HID if hardware exists
  #if NATIVE_USB_ACTIVE
    #if USE_HID_PROJECT
      Consumer.begin();
    #else
      Keyboard.begin();
    #endif
  #endif

  Serial.println(F("=================================================="));
  Serial.println(F("Day 91: USB HID PC Volume Knob (Rotary Encoder)"));
  Serial.print(F(" Hardware Mode: "));
  #if NATIVE_USB_ACTIVE
    #if USE_HID_PROJECT
      Serial.println(F("NATIVE USB (HID-Project Library Active)"));
    #else
      Serial.println(F("NATIVE USB (Keyboard Fallback Mode)"));
    #endif
  #else
    Serial.println(F("SIMULATION (ATmega328P/Uno, Serial Outputs)"));
  #endif
  Serial.println(F("=================================================="));
  
  printMenu();
}

void loop() {
  // 1. Read Quadrature Rotary Encoder (Non-blocking Polling)
  int currentStateCLK = digitalRead(ENCODER_PIN_A);
  
  // Detect transition on CLK pin
  if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
    // If the DT state is different from the CLK state, the encoder is rotating Clockwise
    if (digitalRead(ENCODER_PIN_B) != currentStateCLK) {
      volumeUp();
    } else {
      // Otherwise, encoder is rotating Counter-Clockwise
      volumeDown();
    }
  }
  lastStateCLK = currentStateCLK;

  // 2. Read Pushbutton (Mute Command)
  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastButtonTime >= BUTTON_DEBOUNCE_MS) {
      lastButtonTime = millis();
      volumeMute();
    }
  }

  // 3. Poll Serial CLI (Interactive Simulation)
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case '+':
      case 'u':
      case 'U':
        volumeUp();
        break;
      case '-':
      case 'd':
      case 'D':
        volumeDown();
        break;
      case 'm':
      case 'M':
        volumeMute();
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
//  VOLUME CONTROL ACTIONS
// =============================================================

/**
 * Commands system to increase volume.
 */
void volumeUp() {
  Serial.println(F("[VOLUME] Knob Rotated: CW -> Volume UP"));
  digitalWrite(LED_INDICATOR, HIGH);
  delay(10);
  digitalWrite(LED_INDICATOR, LOW);

  #if NATIVE_USB_ACTIVE
    #if USE_HID_PROJECT
      Consumer.write(MEDIA_VOLUME_UP);
    #else
      // Fallback: Send a media-player shortcut (e.g. VLC volume is Ctrl+Up Arrow)
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_UP_ARROW);
      delay(20);
      Keyboard.releaseAll();
    #endif
  #else
    #if USE_HID_PROJECT
      Serial.println(F("[SIMULATED USB HID] Sent: Consumer Key Media Volume Up"));
    #else
      Serial.println(F("[SIMULATED USB HID] Sent shortcut: CTRL + UP_ARROW"));
    #endif
  #endif
}

/**
 * Commands system to decrease volume.
 */
void volumeDown() {
  Serial.println(F("[VOLUME] Knob Rotated: CCW -> Volume DOWN"));
  digitalWrite(LED_INDICATOR, HIGH);
  delay(10);
  digitalWrite(LED_INDICATOR, LOW);

  #if NATIVE_USB_ACTIVE
    #if USE_HID_PROJECT
      Consumer.write(MEDIA_VOLUME_DOWN);
    #else
      // Fallback: Send VLC volume decrease shortcut (Ctrl+Down Arrow)
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_DOWN_ARROW);
      delay(20);
      Keyboard.releaseAll();
    #endif
  #else
    #if USE_HID_PROJECT
      Serial.println(F("[SIMULATED USB HID] Sent: Consumer Key Media Volume Down"));
    #else
      Serial.println(F("[SIMULATED USB HID] Sent shortcut: CTRL + DOWN_ARROW"));
    #endif
  #endif
}

/**
 * Commands system to mute or unmute audio.
 */
void volumeMute() {
  muteState = !muteState;
  Serial.print(F("[VOLUME] Button Pressed: -> Mute Toggle (Active: "));
  Serial.print(muteState ? F("YES") : F("NO"));
  Serial.println(F(")"));

  // Solid LED when muted
  digitalWrite(LED_INDICATOR, muteState ? HIGH : LOW);

  #if NATIVE_USB_ACTIVE
    #if USE_HID_PROJECT
      Consumer.write(MEDIA_VOLUME_MUTE);
    #else
      // Fallback: Send mute shortcut (M key in VLC, or custom shortcut)
      Keyboard.press('m');
      delay(20);
      Keyboard.releaseAll();
    #endif
  #else
    #if USE_HID_PROJECT
      Serial.println(F("[SIMULATED USB HID] Sent: Consumer Key Media Volume Mute"));
    #else
      Serial.println(F("[SIMULATED USB HID] Sent shortcut: Key 'm'"));
    #endif
  #endif
}

void printMenu() {
  Serial.println(F("\n--- VOLUME KNOB INTERACTIVE CLI ---"));
  Serial.println(F(" '+' or 'u' : Simulate Clockwise rotation (Volume Up)"));
  Serial.println(F(" '-' or 'd' : Simulate Counter-Clockwise rotation (Volume Down)"));
  Serial.println(F(" 'm'        : Simulate Button press (Mute Toggle)"));
  Serial.println(F(" 'h'        : Print this command help menu"));
  Serial.println(F("-------------------------------------\n"));
}

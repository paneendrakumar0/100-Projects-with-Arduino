/*
 * 100 Projects with Arduino - Day 3
 * Project: Pushbutton State Toggle with Software Debouncing
 * 
 * DESCRIPTION:
 * This project demonstrates how to interface a tactile pushbutton with the Arduino
 * to toggle an LED on and off. Instead of using the blocking delay() function or
 * basic digitalRead() which suffers from mechanical button bounce, this sketch
 * implements a robust, non-blocking software debounce algorithm using millis().
 * 
 * THEORY OF OPERATION:
 * 1. Mechanical switches consist of metal contacts. When pressed, these contacts
 *    physically bounce against each other for a few milliseconds before settling.
 *    To the microcontroller, this looks like the button is pressed and released 
 *    dozens of times in a fraction of a millisecond.
 * 2. We use a software timer (millis()) to ignore any state changes that happen
 *    faster than our debounce threshold (50 milliseconds).
 * 3. We use the internal pull-up resistor (INPUT_PULLUP), meaning:
 *    - Button NOT pressed = HIGH (5V)
 *    - Button pressed = LOW (GND)
 * 
 * WIRING:
 * - Pushbutton Pin A -> Arduino Pin 2
 * - Pushbutton Pin B -> Arduino GND
 * - Onboard LED is connected to Pin 13 (no external wiring needed for the LED)
 * 
 * INSTRUCTIONS:
 * 1. Connect the pushbutton to Pin 2 and GND.
 * 2. Upload this code to your Arduino.
 * 3. Open the Serial Monitor at 9600 Baud.
 * 4. Press the button. The onboard LED will toggle state with every clean press,
 *    and status messages will be printed to the Serial Monitor.
 */

// --- PIN DEFINITIONS ---
const int BUTTON_PIN = 2; // Pin connected to the pushbutton
const int LED_PIN = 13;   // Pin connected to the onboard LED

// --- STATE VARIABLES ---
bool ledState = LOW;         // Tracks the current state of the LED (ON or OFF)
int buttonState = HIGH;      // Tracks the current stable debounced button state
int lastButtonState = HIGH;  // Tracks the previous raw reading from the button pin

// --- TIMING VARIABLES ---
unsigned long lastDebounceTime = 0;  // Store the last time the output pin was toggled
const unsigned long debounceDelay = 50; // The debounce time threshold in milliseconds (50ms is standard)

void setup() {
  // Initialize Serial communication at 9600 bps for debugging
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect (needed for native USB boards like Leonardo/Micro)
  }
  
  Serial.println("==================================================");
  Serial.println("Day 3: Pushbutton State Toggle with Debouncing");
  Serial.println("==================================================");

  // Configure the LED pin as an output
  pinMode(LED_PIN, OUTPUT);
  
  // Configure the button pin with the internal pull-up resistor.
  // This pulls the pin to HIGH when the button is open (not pressed).
  // When the button is closed (pressed), it connects to GND, reading LOW.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Set the initial LED state
  digitalWrite(LED_PIN, ledState);
  Serial.println("System Initialized. Press the button to toggle the LED.");
}

void loop() {
  // Step 1: Read the current raw state of the pushbutton
  int reading = digitalRead(BUTTON_PIN);

  // Step 2: Check if the button state has changed (due to noise or press)
  // If the raw reading is different from the last raw reading, we reset the timer.
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Step 3: Check if the reading has been stable for longer than the debounce delay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Whatever the reading is, it has been stable for longer than our threshold,
    // so we can accept it as the actual, stable state of the button.
    
    // Check if the stable state has changed from our previously saved button state
    if (reading != buttonState) {
      buttonState = reading;

      // Step 4: If the new stable state is LOW, the button was transitioned to PRESSED.
      // Since we use INPUT_PULLUP, a transition to LOW represents a button press.
      if (buttonState == LOW) {
        // Toggle the LED state
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        
        // Print the status to the Serial Monitor for verification
        Serial.print("[DEBOUNCED] Button Pressed! LED is now: ");
        if (ledState == HIGH) {
          Serial.println("ON");
        } else {
          Serial.println("OFF");
        }
      }
    }
  }

  // Step 5: Save the raw reading so we can compare it next time through the loop
  lastButtonState = reading;
  
  // Note: Because there are NO delay() functions, the loop runs thousands of times
  // per second, allowing the Arduino to perform other tasks concurrently in a
  // real mechatronic system.
}

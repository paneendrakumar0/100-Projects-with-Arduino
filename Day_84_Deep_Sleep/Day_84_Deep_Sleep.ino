/*
 * 100 Projects with Arduino - Day 84
 * Project: Deep Sleep & Power Saving Modes (AVR Sleep Manager with External Interrupt Wake-up)
 * 
 * DESCRIPTION:
 * This project demonstrates advanced battery and power optimization techniques for embedded
 * mechatronics systems. To align with industrial remote sensing and low-power IoT designs:
 * 1. Deep Power-Down Mode: Configures the ATmega328P to enter its lowest power consumption state 
 *    (SLEEP_MODE_PWR_DOWN), reducing microcontroller current draw from ~15mA down to under 1µA (chip-only).
 * 2. Hardware Interrupt Wake-up: Connects an external tactile button (Pin 2, INT0) that acts as
 *    an active-low hardware interrupt. Pressing the button wakes the processor instantly from sleep.
 * 3. ADC Shutdown: Disables the internal Analog-to-Digital Converter (ADC) before entering sleep
 *    (saving an additional ~250µA of current) and re-enables it upon wake-up.
 * 4. Inactivity Timer: Implements a non-blocking 5-second timer. If no user interaction is detected,
 *    the system automatically enters deep sleep.
 * 
 * SLEEP MODES & POWER REDUCTION THEORY:
 * - SLEEP_MODE_PWR_DOWN: In this state, the main crystal oscillator is halted, stopping all internal
 *   clocks (CPU clock, I/O clock, flash clock). Only asynchronous interrupts (like low level on INT0)
 *   or the Watchdog timer can wake the chip.
 * - ADC Power Draw: The ADC contains active comparator circuitry. If the chip is put to sleep but
 *   the ADC is not disabled (`ADCSRA` bit `ADEN` = 0), the comparator remains powered and drains current.
 * - Board-level Power: Note that standard Arduino Uno boards contain a USB-to-Serial chip (ATmega16U2)
 *   and a 5V linear regulator that continue to draw ~10-15mA even when the main microcontroller is in
 *   deep sleep. For true micro-ampere operations, a bare chip (like an ATmega328P on a breadboard)
 *   or a board with removable LEDs/regulators (like the Arduino Pro Mini) must be used.
 * 
 * WIRING:
 * - Wake-up Button -> Connect Pin 2 to GND (utilizes internal INPUT_PULLUP)
 * - Status LED      -> Pin 13 (Built-in LED, flashes to indicate power state)
 */

#include <avr/sleep.h>
#include <avr/power.h>

// --- PIN DEFINITIONS ---
const int WAKE_BUTTON_PIN = 2; // Interrupt 0 pin (INT0)
const int STATUS_LED      = 13;

// --- TIMING CONFIGURATION ---
unsigned long lastActivityTime = 0;
const unsigned long INACTIVITY_TIMEOUT_MS = 5000; // Go to sleep after 5 seconds of inactivity

// --- STATE VARIABLES ---
volatile bool wokeUpFlag = false; // Flag set by ISR to indicate interrupt occurred

void setup() {
  Serial.begin(9600);
  
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);

  // Set button pin as input with internal pull-up resistor
  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 84: Deep Sleep & Power Saving Modes"));
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] Uptime started. Active monitoring..."));
  Serial.println(F("[SYSTEM] Press button on Pin 2 to reset inactivity timer."));
  
  lastActivityTime = millis();
}

void loop() {
  unsigned long now = millis();

  // 1. Check if the ISR flag was set
  if (wokeUpFlag) {
    wokeUpFlag = false;
    Serial.println(F("[WAKE] System woke up from deep sleep."));
    
    // Flash LED rapidly 3 times to indicate wake-up
    for (int i = 0; i < 3; i++) {
      digitalWrite(STATUS_LED, HIGH);
      delay(100);
      digitalWrite(STATUS_LED, LOW);
      delay(100);
    }
    lastActivityTime = millis(); // Reset inactivity timer
  }

  // 2. Poll the button to reset the inactivity timer
  if (digitalRead(WAKE_BUTTON_PIN) == LOW) {
    if (now - lastActivityTime > 200) { // Simple debounce
      Serial.println(F("[ACTIVITY] Button pressed. Timer reset."));
      lastActivityTime = now;
      digitalWrite(STATUS_LED, HIGH); // Keep LED on
    }
  }

  // 3. Monitor for inactivity and trigger sleep
  if (now - lastActivityTime >= INACTIVITY_TIMEOUT_MS) {
    enterDeepSleep();
  }

  // Heartbeat indicator during active mode (blinks slowly)
  if (!wokeUpFlag && (now - lastActivityTime < INACTIVITY_TIMEOUT_MS)) {
    // Blink LED slowly (100ms on, 900ms off) to show active mode
    if ((now / 1000) % 2 == 0) {
      digitalWrite(STATUS_LED, HIGH);
    } else {
      digitalWrite(STATUS_LED, LOW);
    }
  }
}

// =============================================================
//  LOW-LEVEL POWER MANAGEMENT OPERATIONS
// =============================================================

/**
 * Disables peripherals and puts the microcontroller into a deep sleep state.
 */
void enterDeepSleep() {
  Serial.println(F("[SYSTEM] Inactivity timeout reached. Entering Deep Sleep now..."));
  Serial.println(F("[SYSTEM] Serial console and MCU clocks halting."));
  Serial.flush(); // Wait for serial transmission to complete before halting clocks

  digitalWrite(STATUS_LED, LOW); // Turn off LED to save power

  // --- STEP 1: DISABLE ADC (Analog-to-Digital Converter) ---
  // The ADC consumes ~250µA of current even when idle. Clear the ADEN bit to disable it.
  byte prevADCSRA = ADCSRA;
  ADCSRA &= ~(1 << ADEN);

  // --- STEP 2: CONFIGURE EXTERNAL INTERRUPT ---
  // Set Pin 2 (INT0) as the wake-up trigger.
  // We use LOW level interrupt because it is the only trigger mode that can wake the MCU
  // from Power-down mode (change or edge interrupts require the I/O clock which is stopped).
  attachInterrupt(digitalPinToInterrupt(WAKE_BUTTON_PIN), wakeUpISR, LOW);

  // --- STEP 3: CONFIGURE SLEEP MODE ---
  // Select the lowest power mode: Power-down
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // Enable sleep mode in the control register
  sleep_enable();

  // --- STEP 4: GO TO SLEEP ---
  // This instruction halts the CPU and clocks. The program freezes here.
  sleep_cpu();

  // =============================================================
  //  THE MICROCONTROLLER IS NOW ASLEEP. EXECUTION HALTS.
  //  (Wakes up here once the button on Pin 2 is pressed)
  // =============================================================

  // --- STEP 5: WAKE UP & CLEANUP ---
  // The first thing the MCU does upon wake-up is execute the wakeUpISR(),
  // and then resumes execution at this point.
  
  sleep_disable(); // Disable sleep immediately for safety
  
  // Detach interrupt to prevent the ISR from firing repeatedly while the button is held
  detachInterrupt(digitalPinToInterrupt(WAKE_BUTTON_PIN));

  // Re-enable the ADC
  ADCSRA = prevADCSRA;

  // Set flag to print wake-up message in the main loop
  wokeUpFlag = true;
}

/**
 * Hardware Interrupt Service Routine (ISR) triggered by low level on Pin 2.
 */
void wakeUpISR() {
  // ISR must do as little as possible. We just set a flag.
  wokeUpFlag = true;
}

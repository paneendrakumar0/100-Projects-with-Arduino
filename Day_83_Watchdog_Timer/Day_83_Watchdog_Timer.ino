/*
 * 100 Projects with Arduino - Day 83
 * Project: Hardware Watchdog Timer (System Recovery & MCUSR Diagnostics)
 * 
 * DESCRIPTION:
 * This project demonstrates how to implement the AVR hardware Watchdog Timer (WDT) to ensure
 * high-reliability, fail-safe operation of embedded mechatronic systems.
 * 1. Boot Diagnostics (MCUSR): Reads the MCU Status Register on startup to determine if the
 *    reset was caused by a normal power-on (PORF) or a critical watchdog timeout (WDRF).
 * 2. Watchdog Enable: Configures the WDT to a 2-second timeout window.
 * 3. Watchdog Feeding: Periodically calls wdt_reset() inside the main loop before the 2-second
 *    window expires, indicating normal program execution.
 * 4. Fault Injection (Simulated Crash): Provides a Serial command ('k') to intentionally force
 *    an infinite loop (simulating a sensor lockup, stack overflow, or firmware freeze).
 *    This prevents the watchdog from being fed, triggering a hardware CPU reset after 2.0s.
 * 
 * WATCHDOG TIMER (WDT) THEORY:
 * - Independent Oscillator: The WDT is driven by an independent internal 128 kHz watchdog oscillator
 *   on the ATmega328P. This ensures that even if the main crystal oscillator halts or the CPU registers
 *   get corrupted by electromagnetic interference (EMI) from motors, the WDT still counts down.
 * - System Recovery: If the WDT is enabled and counts down to zero, it pulls the CPU reset line,
 *   reinitializing the program.
 * - Boot Loops: If the WDT is not disabled immediately on boot, it can cause a permanent boot-loop
 *   if the setup() initialization takes longer than the WDT timeout window.
 * 
 * WIRING:
 * No external components are strictly required.
 * - Onboard LED (Pin 13) is used to display normal heartbeat vs. startup blink sequence.
 */

#include <avr/wdt.h>

void setup() {
  Serial.begin(9600);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Flash LED rapidly 5 times on boot to visually indicate a processor reset
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(80);
    digitalWrite(LED_BUILTIN, LOW);
    delay(80);
  }

  Serial.println(F("=================================================="));
  Serial.println(F("Day 83: AVR Hardware Watchdog Timer (WDT)"));
  Serial.println(F("=================================================="));

  // --- DIAGNOSE RESET SOURCE VIA MCUSR ---
  // MCUSR holds bits indicating the reset trigger source:
  // - Bit 0 (PORF) : Power-On Reset
  // - Bit 1 (EXTRF): External Reset (Reset Button pressed)
  // - Bit 2 (BORF) : Brown-Out Reset (Voltage drop)
  // - Bit 3 (WDRF) : Watchdog Reset
  byte resetSource = MCUSR;
  
  Serial.print(F("[DIAGNOSTICS] MCUSR register: 0x"));
  Serial.println(resetSource, HEX);

  if (resetSource & (1 << WDRF)) {
    Serial.println(F("[SYSTEM] CRITICAL: Processor was reset by WATCHDOG TIMER timeout!"));
  } else if (resetSource & (1 << EXTRF)) {
    Serial.println(F("[SYSTEM] Reset source: External Reset Pin (Reset Button)."));
  } else if (resetSource & (1 << PORF)) {
    Serial.println(F("[SYSTEM] Reset source: Power-On Reset (Normal Power-up)."));
  } else {
    Serial.println(F("[SYSTEM] Reset source: Software or brown-out trigger."));
  }

  // Clear MCUSR reset flags. This is required because the flags persist across reboots,
  // and we must clear them to read the next reset source correctly.
  MCUSR = 0x00;

  // --- SAFE WATCHDOG CONFIGURATION ---
  // 1. Disable watchdog immediately during initialization to prevent boot loops
  wdt_disable();
  delay(100); // Allow stabilization

  // 2. Enable watchdog with a 2-second timeout window
  // Options: WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS, WDTO_250MS,
  //          WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
  wdt_enable(WDTO_2S);
  Serial.println(F("[SYSTEM] Watchdog Timer armed with 2.0-second timeout."));
  
  printMenu();
}

void loop() {
  // Feed the watchdog (reset WDT counter back to zero)
  wdt_reset();

  // Pulse onboard LED as a heartbeat (non-blocking)
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  }

  // Check for console inputs
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    if (cmd == 'k' || cmd == 'K') {
      Serial.println(F("\n[FAULT INJECTION] Simulating firmware hang (infinite loop) now!"));
      Serial.println(F("[SYSTEM] Watchdog feeding halted. CPU will reset in 2 seconds..."));
      Serial.flush(); // Ensure serial buffer writes out before crash

      // --- SIMULATED FIRMWARE HANG ---
      // Enter an infinite loop. This prevents loop() from completing and wdt_reset()
      // from being called. The WDT hardware will overflow and reset the processor.
      while (1) {
        // LED stays solid HIGH to indicate a crashed system state
        digitalWrite(LED_BUILTIN, HIGH);
      }
    } 
    else if (cmd == 'h' || cmd == 'H') {
      printMenu();
    }
  }
}

void printMenu() {
  Serial.println(F("\n--- WATCHDOG TIMER COMMAND MENU ---"));
  Serial.println(F(" Send 'k' : Crash the program (initiates infinite loop)"));
  Serial.println(F(" Send 'h' : Display this help command list"));
  Serial.println(F("------------------------------------"));
}

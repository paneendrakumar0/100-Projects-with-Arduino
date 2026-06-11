/*
 * 100 Projects with Arduino - Day 81
 * Project: FreeRTOS Preemptive Multitasking (Independent Dual Blinkers & Telemetry)
 *
 * DESCRIPTION:
 * This project introduces Real-Time Operating System (RTOS) concepts on the 8-bit AVR architecture
 * using the standard FreeRTOS kernel. Instead of a single thread of execution where delay() blocks
 * all processing, this sketch configures the FreeRTOS scheduler to run three concurrent, preemptive
 * threads:
 * 1. TaskBlink1 (Priority 1): Drives the onboard LED (Pin 13) at a 1 Hz frequency (500ms ON, 500ms
 * OFF).
 * 2. TaskBlink2 (Priority 1): Drives an external LED (Pin 12) at a 0.5 Hz frequency (1000ms ON,
 * 1000ms OFF).
 * 3. TaskDiagnostics (Priority 2): Periodically outputs system uptime and CPU statistics over the
 *    serial interface. Because it has a higher priority, it preempts the blinker tasks.
 *
 * FreeRTOS & SCHEDULER THEORY:
 * - Preemptive Scheduling: FreeRTOS runs a tick timer (typically 1ms) using Arduino's hardware
 * Timer0. At each tick, the scheduler reviews the active task queue. If a higher priority task is
 * ready, the scheduler performs a context switch: it pushes the current CPU registers onto the
 * active task's stack, switches the stack pointer to the new task, pops its registers, and resumes
 * execution.
 * - Non-blocking Delay: vTaskDelay() suspends the calling task and yields the CPU to other tasks,
 *   allowing the processor to run other code rather than idling in a busy-wait loop.
 * - Stack Size: On ATmega328P (only 2 KB SRAM), stack sizes must be kept extremely small (typically
 *   100-128 words, where 1 word = 2 bytes) to prevent stack overflows and memory collisions.
 *
 * LIBRARY REQUIREMENT:
 * This sketch requires the "FreeRTOS" library by Phillip Stevens (install via Arduino Library
 * Manager).
 *
 * WIRING:
 * - Green LED (Pin 13) -> Onboard LED (no extra wiring required)
 * - Red LED   (Pin 12) -> Connect Anode to Pin 12 via 220Ω resistor, Cathode to GND
 */

#include <Arduino_FreeRTOS.h>

// --- TASK PROTOTYPES ---
void TaskBlink1(void *pvParameters);
void TaskBlink2(void *pvParameters);
void TaskDiagnostics(void *pvParameters);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // Wait for USB connection
  }

  Serial.println(F("=================================================="));
  Serial.println(F("Day 81: FreeRTOS Preemptive Multitasking"));
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] Initializing FreeRTOS kernel tasks..."));

  // Create Task 1 (Blink Pin 13)
  // Arguments: Task Function, Name, Stack Size (words), Parameters, Priority, Task Handle
  xTaskCreate(TaskBlink1, "Blink1",
              100,  // 100 words = 200 bytes stack space
              NULL,
              1,  // Priority 1 (Low)
              NULL);

  // Create Task 2 (Blink Pin 12)
  xTaskCreate(TaskBlink2, "Blink2",
              100,  // 100 words = 200 bytes stack space
              NULL,
              1,  // Priority 1 (Low)
              NULL);

  // Create Task 3 (Diagnostics Console Output)
  xTaskCreate(TaskDiagnostics, "Diagnostics",
              128,  // 128 words = 256 bytes stack space (needs more for String processing)
              NULL,
              2,  // Priority 2 (Higher than blinkers)
              NULL);

  Serial.println(F("[SYSTEM] Starting scheduler. Loop() is now bypassed."));

  // Note: The FreeRTOS scheduler automatically takes over after setup() ends.
  // loop() will never be called again!
}

void loop() {
  // Bypassed!
}

// =============================================================
//  RTOS TASK IMPLEMENTATIONS
// =============================================================

/**
 * Task 1: Blinks onboard Pin 13 LED at 1 Hz (500ms ON / 500ms OFF).
 */
void TaskBlink1(void *pvParameters) {
  (void)pvParameters;  // Unused parameter

  pinMode(13, OUTPUT);

  for (;;) {
    digitalWrite(13, HIGH);
    // Suspension delay: Yields CPU to other tasks for 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);

    digitalWrite(13, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

/**
 * Task 2: Blinks external Pin 12 LED at 0.5 Hz (1000ms ON / 1000ms OFF).
 */
void TaskBlink2(void *pvParameters) {
  (void)pvParameters;

  pinMode(12, OUTPUT);

  for (;;) {
    digitalWrite(12, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    digitalWrite(12, LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/**
 * Task 3: Outputs system telemetry. Runs every 3.0 seconds.
 * Higher priority ensures immediate execution.
 */
void TaskDiagnostics(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    unsigned long uptimeSeconds = millis() / 1000;

    Serial.println(F("\n--- FREERTOS SYSTEM REPORT ---"));
    Serial.print(F(" Uptime      : "));
    Serial.print(uptimeSeconds);
    Serial.println(F(" seconds"));
    Serial.print(F(" Scheduler   : Preemptive Tick Timer (1ms)"));
    Serial.print(F(" | Blinker 1 : 1 Hz (D13)"));
    Serial.println(F(" | Blinker 2 : 0.5 Hz (D12)"));
    Serial.println(F("------------------------------"));

    // Yield CPU for 3000ms
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

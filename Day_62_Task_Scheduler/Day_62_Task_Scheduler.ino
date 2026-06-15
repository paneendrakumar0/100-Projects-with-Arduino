/*
 * 100 Projects with Arduino - Day 62
 * Project: Cooperative Task Scheduler (Round-Robin State Machine Kernel)
 *
 * DESCRIPTION:
 * This project builds a minimal cooperative multitasking kernel from scratch,
 * completely without any OS library. It demonstrates how real embedded RTOS
 * frameworks (FreeRTOS, mbed) work at their core.
 *
 * COOPERATIVE vs PRE-EMPTIVE MULTITASKING:
 *  - COOPERATIVE: Each task runs to completion and VOLUNTARILY yields control.
 *    The kernel scheduler only runs when a task finishes or explicitly yields.
 *    Advantage: No race conditions, no need for mutexes, zero overhead.
 *    Risk: A badly-behaved (blocking) task can starve all others.
 *
 *  - PRE-EMPTIVE: The OS can interrupt a task mid-execution using a timer ISR
 *    and force a context switch. Advantage: Fair CPU sharing. Risk: Complex
 *    context save/restore, requires RTOS.
 *
 * KERNEL DESIGN — Task Control Block (TCB):
 *  Each task is described by a struct:
 *    - name[]       : Human-readable task name
 *    - taskFn       : Pointer to the task function (void fn())
 *    - periodMs     : Desired execution period in milliseconds
 *    - lastRunMs    : Timestamp of last execution (for period timing)
 *    - enabled      : Whether the task is active
 *    - runCount     : Number of times the task has been dispatched
 *    - totalTimeUs  : Cumulative execution time (for CPU profiling)
 *
 * SCHEDULER ALGORITHM — Round-Robin with Period Timers:
 *  On every loop() iteration, iterate through all tasks.
 *  For each task: if (now - lastRunMs >= periodMs), call taskFn().
 *  This is called "tick-based cooperative scheduling" or "super-loop scheduling".
 *
 * TASKS INCLUDED:
 *  Task 0: LED Blink       (1 Hz   — toggles pin 13)
 *  Task 1: Sensor Read     (10 Hz  — reads A0 ADC and prints to serial)
 *  Task 2: Serial Heartbeat(0.5 Hz — prints system uptime and CPU load)
 *  Task 3: Button Monitor  (20 Hz  — debounces pin 2, toggles LED blink task)
 *  Task 4: Idle Counter    (100 Hz — counts idle loop cycles for jitter analysis)
 *
 * WIRING:
 *  LED (or built-in LED 13) -> D13
 *  Button -> D2 (other pin to GND)
 *  Any sensor (pot, LDR etc.) -> A0
 */

// ============================================================
//  TASK CONTROL BLOCK DEFINITION
// ============================================================
typedef void (*TaskFn)();  // Function pointer type for task functions

struct Task {
  const char *name;         // Descriptive name for logging
  TaskFn taskFn;            // Pointer to the task function
  uint16_t periodMs;        // Execution period (ms)
  unsigned long lastRunMs;  // millis() at last execution
  bool enabled;             // Active or suspended
  uint32_t runCount;        // Lifetime execution count
  uint32_t totalTimeUs;     // Cumulative CPU time in microseconds
};

// ============================================================
//  PIN DEFINITIONS
// ============================================================
const int LED_PIN = 13;
const int BUTTON_PIN = 2;
const int SENSOR_PIN = A0;

// ============================================================
//  TASK FUNCTION FORWARD DECLARATIONS
// ============================================================
void task_ledBlink();
void task_sensorRead();
void task_heartbeat();
void task_buttonMonitor();
void task_idleCounter();

// ============================================================
//  TASK TABLE — Edit this to add/remove tasks
// ============================================================
Task tasks[] = {
    //  name                  fn                  period  lastRun  enabled  runCount  totalUs
    {"LED_Blink", task_ledBlink, 1000, 0, true, 0, 0},
    {"Sensor_Read", task_sensorRead, 100, 0, true, 0, 0},
    {"Heartbeat", task_heartbeat, 2000, 0, true, 0, 0},
    {"Button_Monitor", task_buttonMonitor, 50, 0, true, 0, 0},
    {"Idle_Counter", task_idleCounter, 10, 0, true, 0, 0},
};
const int NUM_TASKS = sizeof(tasks) / sizeof(tasks[0]);

// ============================================================
//  GLOBAL STATE
// ============================================================
volatile uint32_t idleCycles = 0;  // Incremented by idle task
bool ledBlinkState = false;        // Current LED state

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println(F("\n======================================"));
  Serial.println(F("  Cooperative Task Scheduler Kernel"));
  Serial.println(F("======================================"));
  Serial.print(F("Tasks registered: "));
  Serial.println(NUM_TASKS);
  for (int i = 0; i < NUM_TASKS; i++) {
    Serial.print(F("  ["));
    Serial.print(i);
    Serial.print(F("] "));
    Serial.print(tasks[i].name);
    Serial.print(F(" @ "));
    Serial.print(tasks[i].periodMs);
    Serial.println(F(" ms"));
  }
  Serial.println(F("======================================\n"));
}

// ============================================================
//  MAIN LOOP — The Scheduler Dispatcher
// ============================================================
void loop() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_TASKS; i++) {
    if (!tasks[i].enabled) continue;
    if (now - tasks[i].lastRunMs >= tasks[i].periodMs) {
      // Dispatch the task and measure its execution time
      unsigned long t0 = micros();
      tasks[i].taskFn();
      unsigned long elapsed = micros() - t0;

      // Update TCB metadata
      tasks[i].lastRunMs = now;
      tasks[i].runCount++;
      tasks[i].totalTimeUs += elapsed;
    }
  }
}

// ============================================================
//  TASK IMPLEMENTATIONS
// ============================================================

// Task 0 — LED Blink: Toggle the LED
void task_ledBlink() {
  ledBlinkState = !ledBlinkState;
  digitalWrite(LED_PIN, ledBlinkState ? HIGH : LOW);
}

// Task 1 — Sensor Read: Read ADC and print
void task_sensorRead() {
  int value = analogRead(SENSOR_PIN);
  float voltage = value * (5.0f / 1023.0f);
  Serial.print(F("[Sensor] ADC="));
  Serial.print(value);
  Serial.print(F(" | V="));
  Serial.print(voltage, 3);
  Serial.println(F("V"));
}

// Task 2 — Heartbeat: Print system stats
void task_heartbeat() {
  unsigned long upSec = millis() / 1000;

  Serial.println(F("--- HEARTBEAT ---"));
  Serial.print(F("  Uptime: "));
  Serial.print(upSec);
  Serial.println(F(" s"));
  Serial.print(F("  Idle cycles (last 2s): "));
  Serial.println(idleCycles);
  idleCycles = 0;  // Reset idle counter between reports

  // Print per-task stats
  Serial.println(F("  Task                 Runs    AvgUs"));
  for (int i = 0; i < NUM_TASKS; i++) {
    uint32_t avgUs = (tasks[i].runCount > 0) ? (tasks[i].totalTimeUs / tasks[i].runCount) : 0;
    Serial.print(F("  "));
    Serial.print(tasks[i].name);
    // Pad spacing
    for (int p = strlen(tasks[i].name); p < 20; p++) Serial.print(' ');
    Serial.print(tasks[i].runCount);
    Serial.print(F("      "));
    Serial.println(avgUs);
  }
  Serial.println(F("-----------------"));
}

// Task 3 — Button Monitor: Debounced toggle of LED task
void task_buttonMonitor() {
  static bool lastState = HIGH;
  bool curState = digitalRead(BUTTON_PIN);
  if (curState == LOW && lastState == HIGH) {
    // Toggle LED Blink task
    tasks[0].enabled = !tasks[0].enabled;
    Serial.print(F("[Button] LED Blink task: "));
    Serial.println(tasks[0].enabled ? F("ENABLED") : F("DISABLED"));
    if (!tasks[0].enabled) digitalWrite(LED_PIN, LOW);
  }
  lastState = curState;
}

// Task 4 — Idle Counter: Count how many times idle task runs (measures CPU slack)
void task_idleCounter() {
  idleCycles++;
}

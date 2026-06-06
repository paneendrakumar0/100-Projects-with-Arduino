# Day 62: Cooperative Task Scheduler (Round-Robin Kernel from Scratch)

Welcome to Day 62! Today we implement a **bare-metal cooperative multitasking scheduler** on the Arduino — from scratch, no library, no RTOS. We define a **Task Control Block (TCB)** struct, an array of task descriptors with function pointers, and a **round-robin dispatcher** in `loop()` that calls each task at its configured period. This is exactly how embedded RTOS systems like FreeRTOS and Zephyr work at their core.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Potentiometer.jpg" alt="Potentiometer" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

In most Arduino projects, `loop()` contains all the logic in a single sequential flow. This creates problems when:
* You need **different update rates** (e.g., LED at 1 Hz, sensor at 100 Hz, OLED at 5 Hz).
* A `delay()` call **blocks everything** — the sensor can't be read, the button can't be checked.
* The project grows beyond one concept — the code becomes spaghetti.

The solution real embedded engineers use is a **scheduler** — a central piece of code that decides which task runs when. Today we build one.

---

## 🔬 Architecture Deep Dive

### 1. Task Control Block (TCB) — The Heart of Any RTOS

Every schedulable unit of work is described by a `Task` struct:

```cpp
struct Task {
  const char*   name;         // Human-readable identifier
  TaskFn        taskFn;       // Function pointer — called by dispatcher
  uint16_t      periodMs;     // How often to run (milliseconds)
  unsigned long lastRunMs;    // millis() timestamp of last invocation
  bool          enabled;      // Can be suspended at runtime
  uint32_t      runCount;     // Lifetime dispatch count
  uint32_t      totalTimeUs;  // Cumulative execution time (for CPU profiling)
};
```

**Function pointers** (`typedef void (*TaskFn)()`) allow the scheduler to call any task function by address — this is C's version of polymorphism.

### 2. Round-Robin Dispatcher Algorithm

```
For each loop() iteration:
  now = millis()
  For each task[i] in the task table:
    if task[i].enabled AND (now - task[i].lastRunMs >= task[i].periodMs):
      t_start = micros()
      call task[i].taskFn()         ← Execute the task
      task[i].lastRunMs = now       ← Reset the period timer
      task[i].totalTimeUs += elapsed  ← Accumulate CPU time
```

This is an **O(N) scan per loop iteration** — for N=5 tasks, this is negligible overhead.

### 3. Cooperative vs Pre-Emptive — Key Differences

| Property | **Cooperative (Our Design)** | Pre-Emptive (FreeRTOS) |
| :--- | :--- | :--- |
| **Context switch trigger** | Task voluntarily finishes | Timer ISR forces switch |
| **RAM overhead** | Zero (no context save) | ~500+ bytes per task (stack) |
| **Race conditions** | None — only one task runs at a time | Possible — requires mutexes |
| **Worst-case latency** | Longest single task execution time | Timer tick period (e.g., 1 ms) |
| **Best for** | Simple embedded projects | Complex real-time systems |

### 4. CPU Load Analysis (Idle Cycles)
The `Idle_Counter` task runs at 100 Hz. In 2 seconds, if the CPU were 100% free it would run `2000 / 10 = 200` times. The heartbeat reports the actual idle count, giving a rough **CPU slack indicator**:

$$\text{CPU Load} \approx 1 - \frac{\text{Idle Cycles}}{200} \times 100\%$$

The Heartbeat task also prints each task's average execution time in microseconds, allowing identification of which task dominates CPU time.

### 5. Timing Jitter
Because this is cooperative, task timing has jitter equal to the **worst-case execution time of any single task**. If Task A takes 5 ms to run, Task B scheduled at 10 ms may fire at 10 ms + 5 ms = 15 ms. For audio/motor control requiring sub-millisecond precision, use a **Timer ISR** instead.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Main controller |
| LED | 1 | Task 0 visual output (or use built-in D13) |
| Push Button | 1 | Task 3 input (enable/disable LED task) |
| 10 kΩ Resistor | 1 | Button pull-down (or use INPUT_PULLUP in code) |
| Potentiometer / Sensor | 1 | Task 1 analog input on A0 |

---

## 🔌 Pin-to-Pin Wiring

| Component | Arduino Pin | Description |
| :--- | :--- | :--- |
| **LED Anode** | **D13** | Task 0 output (built-in LED OK) |
| **Button** | **D2** | Task 3 input (pull to GND when pressed) |
| **Sensor / Pot Wiper** | **A0** | Task 1 analog input |

---

## 💻 How to Test & Validate

1. Upload [Day_62_Task_Scheduler.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_62_Task_Scheduler/Day_62_Task_Scheduler.ino).
2. Open **Serial Monitor** at **9600 Baud**.
3. Observe the startup banner listing all registered tasks and their periods.
4. Every 100 ms the sensor reading is printed. Every 2 seconds the heartbeat prints a CPU load table.
5. Press the button — the LED Blink task will be **disabled** (LED goes off, `[Button] LED Blink task: DISABLED`). Press again to re-enable.
6. Observe the `AvgUs` column in the heartbeat — `Heartbeat` itself should be the most expensive task (Serial.print overhead). All others should be <100 µs.

---

## 📈 Extending the Scheduler

| Feature | How to Add |
| :--- | :--- |
| **New task** | Add a new `Task` entry to the `tasks[]` array and write the function |
| **One-shot task** | Set `enabled = false` inside the task function after running |
| **Priority** | Sort tasks by period (shorter period = higher priority) |
| **Delayed start** | Initialize `lastRunMs = millis() + delayMs` instead of 0 |

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| LED never blinks | `task_ledBlink` not enabled or period misconfigured | Check `tasks[0].enabled = true` and `periodMs = 1000` |
| All tasks delayed | One task contains a `delay()` call | Remove all `delay()` calls from task functions — use non-blocking timing |
| Heartbeat shows 0 idle cycles | CPU is fully saturated | Increase task periods or remove heavy tasks |
| Button press not detected | Debounce period too long | Reduce `periodMs` of `Button_Monitor` task to 20 ms |

## 🧠 Code Explanation

Let's break down how we built a Cooperative RTOS Kernel:

### 1. The Task Control Block (TCB)
```cpp
struct Task {
  const char* name;
  TaskFn      taskFn;
  uint16_t    periodMs;
  unsigned long lastRunMs;
  // ...
};
```
- Every operating system tracks programs using a Task Control Block. Our simple array holds all the metadata for our 5 functions.
- Instead of using `delay()` (which paralyzes the entire CPU), we give each task a `periodMs`. The scheduler promises to call the function pointer (`taskFn`) only when enough time has elapsed since `lastRunMs`.

### 2. The Super-Loop Dispatcher
```cpp
for (int i = 0; i < NUM_TASKS; i++) {
  if (now - tasks[i].lastRunMs >= tasks[i].periodMs) {
    tasks[i].taskFn();
    tasks[i].lastRunMs  = now;
  }
}
```
- The main `loop()` does nothing but rapidly scan through the task array. 
- Because this is a **Cooperative** scheduler, the system relies on trust. When `taskFn()` is called, the task *must* do its job quickly and return. If a task accidentally runs a `while(true)` loop, the entire OS freezes because the dispatcher never gets control back!

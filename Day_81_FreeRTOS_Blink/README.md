# Day 81: FreeRTOS Blink (Preemptive Multitasking on Arduino)

Welcome to Day 81! Today we enter the world of **Real-Time Operating Systems (RTOS)**. We break away from the traditional, single-threaded Arduino execution model and configure the **FreeRTOS kernel** to run three concurrent, preemptive tasks on the 8-bit ATmega328P. We will study **context switching physics**, **stack memory allocations**, and preemptive task scheduling.

---

## 🎯 The "Why" and "What"

Standard Arduino programs run on a single thread inside the `loop()` function.
- **The Blocking Problem**: If you call `delay(1000)` to blink an LED, the microcontroller sits in a busy-wait loop for 1 second. During this time, it cannot read sensors, update displays, or compute motor controls.
- **The RTOS Solution**: A Real-Time Operating System divides your code into independent threads (**Tasks**). The RTOS scheduler allocates CPU time to each task based on its **Priority**. When a task waits (e.g. calls `vTaskDelay()`), the scheduler suspends it and hands CPU control to another ready task, maximizing processor utilization.

For complex mechatronic systems (like quadcopters balancing while reading GPS and responding to radio commands), multitasking is mandatory.

---

## 🔬 Physics & Hardware Theory

### 1. Preemptive Multitasking & Tick Timers
FreeRTOS on AVR runs as a **preemptive scheduler**.
- **The System Tick**: The kernel configures a hardware timer (typically Timer0 on the ATmega328P) to fire a periodic interrupt every **$1\,\text{ms}$** (known as the Scheduler Tick).
- **Preemption**: When the tick interrupt fires, the scheduler ISR runs. It checks if the current task has run out of time or if a higher priority task has woken up. If so, it halts the current task and swaps execution.

### 2. Context Switching Physics
When the scheduler switches tasks, it must perform a **Context Switch**:
1. **Save Context**: The CPU halts. All internal registers (32 general-purpose registers `r0`–`r31`, Status Register `SREG`, and the Program Counter `PC`) of the active task are pushed onto its dedicated stack space in SRAM.
2. **Select Task**: The scheduler updates its internal pointer to the stack of the next task to run.
3. **Restore Context**: The CPU registers, status bits, and program counter of the new task are popped off its stack. The processor resumes execution exactly where it was halted.

```
       Task A Running            Scheduler Tick (ISR)          Task B Resuming
    ┌──────────────────┐         ┌──────────────────┐         ┌──────────────────┐
    │  Execute code    │ ──────► │ Save registers   │ ──────► │ Pop registers    │
    │  r0-r31, PC, SP  │         │ of Task A to Stack│        │ of Task B        │
    └──────────────────┘         └──────────────────┘         └──────────────────┘
```

### 3. Stack Memory Limits (AVR SRAM Constraints)
The ATmega328P has only **$2\,\text{KB}$ of SRAM**.
- **SRAM allocation**: Every task requires its own stack block in SRAM to hold local variables, function calls, and CPU context registers.
- **Word sizes**: Stack size is configured in "words" (1 word = 2 bytes on AVR).
- If we configure a task with 128 words, it uses $256\,\text{bytes}$ of SRAM. Spawning four such tasks consumes $1024\,\text{bytes}$ (50% of the entire Uno memory). Stack sizes must be budgeted carefully to prevent **Stack Overflow** (where task stacks collide and crash the program).

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microprocessor |
| Red LED | 1 | External task indicator |
| 220 Ω Resistor | 1 | LED current limiting |
| Breadboard & Wires | 1 | Connections |

---

## 🔌 Pin-to-Pin Wiring

| Component Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **Green LED (Onboard)**| **D13** | Driven by Task 1 |
| **Red LED Anode** | **D12** | Driven by Task 2 (via 220Ω resistor) |
| **LED Cathodes** | **GND** | Ground |

---

## 💾 Multitasking Alternatives

| Method | Timing Accuracy | Context Switch Cost | Coding Complexity | CPU Overheads |
| :--- | :--- | :--- | :--- | :--- |
| **FreeRTOS (Preemptive)**| Extremely High | High (~100-200 cycles) | Moderate | Low |
| **Cooperative Scheduler**| Low | Low (~10-20 cycles) | Low | Very Low |
| **State Machine (`millis`)**| Moderate | Zero | High | Zero |
| **Timer Interrupts** | Extremely High | Low | High | Minimal |

---

## 💻 How to Test & Validate

1. Wire the external Red LED to Pin 12 as shown in the table.
2. Install the **FreeRTOS** library in your Arduino IDE:
   - Go to *Sketch* -> *Include Library* -> *Manage Libraries...*
   - Search for **FreeRTOS** by *Phillip Stevens* and install it.
3. Upload [Day_81_FreeRTOS_Blink.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_81_FreeRTOS_Blink/Day_81_FreeRTOS_Blink.ino).
4. Open the **Serial Monitor** at **9600 Baud**.
5. Observe the behavior of the LEDs:
   - Onboard green LED (Pin 13) blinks at 1 Hz.
   - External red LED (Pin 12) blinks at 0.5 Hz.
   - The two LEDs flash independently and concurrently.
6. Check the Serial Monitor: every 3 seconds, the high-priority Diagnostics task prints the system report, preempting the blinker tasks for a fraction of a millisecond.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Code uploads but nothing happens (no LEDs blink, no Serial) | Stack Overflow or out of memory | The Arduino Uno ran out of SRAM. Reduce the stack size of your tasks (e.g. from 128 words to 100 or 80) or reduce the number of tasks. |
| Diagnostic output prints corrupted characters | Serial buffer collision | String calculations inside tasks can exceed stack limits. Increase the stack size of `TaskDiagnostics` to 128 words, or reduce string complexity. |
| LEDs flash at wrong frequencies | Tick rate mismatch | FreeRTOS configuration on AVR typically sets the tick rate to 15.6ms or 16ms by default if using watchdog timers, or 1ms if using Timer1. Ensure `portTICK_PERIOD_MS` is used to scale delays. |
| `loop()` code does not run | RTOS overrides loop() | Once the scheduler starts (`vTaskStartScheduler()` which is called implicitly by the library after `setup()`), `loop()` is bypassed. Put all active code inside task functions. |

## 🧠 Code Explanation

Let's break down how a Real-Time Operating System manages parallel execution on a single core:

### 1. Task Creation and Prioritization
```cpp
xTaskCreate(TaskBlink1, "Blink1", 100, NULL, 1, NULL);
```
- Standard Arduino programs execute sequentially in `loop()`. If one function takes too long or uses `delay()`, everything else freezes.
- Here, we create discrete Tasks. We assign them a memory stack size (100 words = 200 bytes) and a priority.
- If a low-priority task is running, and a high-priority task needs to run, FreeRTOS instantly pauses the low-priority task mid-execution, runs the critical task, and then resumes the original task perfectly!

### 2. Time Slicing via the Scheduler
```cpp
vTaskDelay(500 / portTICK_PERIOD_MS);
```
- In standard Arduino, `delay(500)` forces the CPU to burn cycles in a busy-wait loop doing absolutely nothing.
- In FreeRTOS, `vTaskDelay()` tells the RTOS Scheduler: "I don't need the CPU for 500ms."
- The Scheduler immediately marks this task as "Blocked" and hands the CPU to the next available task. The CPU is never idle, allowing true cooperative multitasking!

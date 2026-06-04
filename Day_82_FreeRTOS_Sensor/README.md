# Day 82: FreeRTOS Sensor Reading vs Display Updates (Task Synchronization)

Welcome to Day 82! Today we master **Thread-Safety** and **Inter-Task Communication (ITC)** inside Real-Time Operating Systems. We implement a classic **Producer-Consumer Pattern** on the Arduino Uno using a **FreeRTOS Queue** to exchange sensor telemetry safely between two tasks of different priorities, without risking memory corruption or race conditions.

---

## 🎯 The "Why" and "What"

In an RTOS environment, tasks execute concurrently. If they share variables directly, serious errors can occur:
- **Race Condition**: Suppose Task A is writing a 16-bit integer (which takes two instructions on an 8-bit CPU) and the scheduler interrupts it midway to run Task B, which reads the same variable. Task B reads a corrupted mixture of the old and new bytes.
- **CPU Waste**: If Task B has to wait for a sensor update, having it run a loop checking a flag (`while(!flag)`) wastes valuable CPU cycles.

A **Queue** resolves both issues. It is a thread-safe mailbox:
- It encapsulates mutual exclusion, ensuring only one task writes or reads the memory buffer at a time.
- It allows tasks to **block** (go to sleep) when empty or full, releasing CPU execution time to other ready tasks immediately.

---

## 🔬 Physics & Hardware Theory

### 1. The Producer-Consumer Pattern
Our architecture splits processing into three threads:

```
               [ TaskSensorRead ]  (Producer)
                       │
                 xQueueSend()
                       ▼
                 [ sensorQueue ]   (FIFO Mailbox)
                       │
               xQueueReceive()
                       ▼
             [ TaskSerialDisplay ] (Consumer)
```

1. **The Producer (TaskSensorRead)**: Acquires raw analog data (A0 Potentiometer) and pushes the value to the back of the queue using `xQueueSend()`.
2. **The Consumer (TaskSerialDisplay)**: Pulls data from the front of the queue using `xQueueReceive()`. Because printing to Serial is slow, this task runs at a lower priority.
3. **The Queue**: A First-In-First-Out (FIFO) buffer allocated in SRAM by the kernel.

### 2. Task States & Queue Blocking
FreeRTOS tasks can be in one of four states: **Running, Ready, Blocked, or Suspended**.
- **Blocking on Receive**: When the Consumer task calls `xQueueReceive(sensorQueue, &val, portMAX_DELAY)` and the queue is empty, the task is moved from the `Running` state into the `Blocked` state. The scheduler stops checking it, saving CPU power.
- **Waking on Send**: The instant the Producer task calls `xQueueSend()`, the kernel registers that data is available. It instantly moves the Consumer task back into the `Ready` state. Because the scheduler tick is high frequency, the consumer runs immediately if it has the highest ready priority.

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | RTOS Processor |
| Potentiometer (10kΩ) | 1 | Simulated sensor input |
| Breadboard & Jumper Wires | 1 | Circuit assembly |

---

## 🔌 Pin-to-Pin Wiring

| Potentiometer Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **Pin 1 (GND)** | **GND** | Ground rail |
| **Pin 2 (Wiper)** | **A0** | Analog input |
| **Pin 3 (VCC)** | **5V** | Power rail |

---

## 💾 Task Synchronization Alternatives

| Method | Thread Safe | Data Buffer | CPU Overhead | Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **Queue (Our Choice)**| Yes | Yes (FIFO Array) | Moderate | Message passing, producer-consumer models. |
| **Mutex / Semaphore** | Yes | No (Binary lock) | Low | Protecting shared hardware (like I2C bus or LCD). |
| **Volatile Global** | No | No (Single value) | Zero | Unsafe for multi-byte values. Needs manual locks. |
| **Message Buffer** | Yes | Yes (Variable size) | High | Sending strings or multi-byte structures. |

---

## 💻 How to Test & Validate

1. Wire the potentiometer to the A0 pin of the Arduino Uno.
2. Upload [Day_82_FreeRTOS_Sensor.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_82_FreeRTOS_Sensor/Day_82_FreeRTOS_Sensor.ino) to the board.
3. Open the **Serial Monitor** at **9600 Baud**.
4. Observe the console:
   - Every 200ms, the producer reads the potentiometer and puts it in the queue.
   - The consumer immediately prints:
     `[CONSUMER] Received ADC: 512 | Calculated Voltage: 2.50 V`
   - Simultaneously, watch the onboard LED (Pin 13) blink rapidly at 5 Hz.
5. Rotate the potentiometer wiper. Notice how the printed voltage changes instantly and smoothly.
6. The rapid, smooth blinking of the heartbeat LED demonstrates that even though the consumer task is blocking and waiting, other tasks continue to execute concurrently without interruption.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `[PRODUCER ERROR] Queue is full! Data dropped.` | Consumer task is too slow or locked | Ensure `TaskSerialDisplay` does not contain blocking loops (`delay()`). If the console cannot keep up, increase the queue size during `xQueueCreate()`. |
| Program freezes on boot | Out of SRAM (Heap exhaustion) | The kernel allocates queue buffers and task stacks from the heap. On the Uno, keep task stacks under 100 words and queue lengths under 5 slots. |
| Diagnostic logs show corrupted numbers | Shared resource violation | If you access the analog pin or Serial port from multiple tasks simultaneously, conflict occurs. Ensure only one task writes to the serial port, or protect it with a Mutex semaphore. |

/*
 * 100 Projects with Arduino - Day 82
 * Project: FreeRTOS Task Synchronization (Queues, Producer-Consumer Pattern & Heartbeat)
 *
 * DESCRIPTION:
 * This project demonstrates task synchronization, thread-safety, and inter-task communication
 * inside FreeRTOS. Rather than sharing raw global variables (which causes race conditions
 * and data corruption), we implement a robust **Producer-Consumer Pattern** using a
 * **FreeRTOS Queue**:
 * 1. Producer Task (TaskSensorRead - Priority 2): Polls an analog input (Pin A0 potentiometer)
 *    every 200ms and pushes the reading into the queue.
 * 2. Consumer Task (TaskSerialDisplay - Priority 1): Blocks and waits for data in the queue.
 *    Once available, it pulls the data out and outputs it to the Serial Monitor.
 * 3. Heartbeat Task (TaskHeartbeat - Priority 1): Blinks the onboard LED (Pin 13) at 5 Hz
 *    independently, showing that system multitasking remains active even when other tasks
 *    are blocked waiting for data.
 *
 * FreeRTOS QUEUES & TASK SYNCHRONIZATION:
 * - Thread Safety: A Queue is a kernel object that handles mutual exclusion automatically.
 *   Only one thread can access the queue buffer at a time, preventing race conditions.
 *   It behaves as a First-In-First-Out (FIFO) buffer.
 * - Blocking on Queue: When the consumer calls xQueueReceive() and the queue is empty, the
 *   scheduler immediately suspends the task (`Blocked` state) and releases the CPU. When the
 *   producer pushes data via xQueueSend(), the scheduler instantly wakes up the consumer and
 *   moves it back to the `Ready` state.
 *
 * WIRING:
 * - Potentiometer (10kΩ) -> Outer pins to 5V and GND, center wiper to Pin A0
 * - Onboard LED (Pin 13) -> Heartbeat visualizer
 */

#include <Arduino_FreeRTOS.h>
#include <queue.h>

// --- TASK PROTOTYPES ---
void TaskSensorRead(void *pvParameters);
void TaskSerialDisplay(void *pvParameters);
void TaskHeartbeat(void *pvParameters);

// --- QUEUE HANDLE ---
QueueHandle_t sensorQueue;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // Wait for USB serial connection
  }

  Serial.println(F("=================================================="));
  Serial.println(F("Day 82: FreeRTOS Task Synchronization via Queues"));
  Serial.println(F("=================================================="));

  // 1. Create a Queue to hold up to 5 integers
  sensorQueue = xQueueCreate(5,           // Queue length (maximum number of items)
                             sizeof(int)  // Size of each item (2 bytes for int on AVR)
  );

  if (sensorQueue != NULL) {
    Serial.println(F("[SYSTEM] FreeRTOS Queue created successfully."));

    // 2. Create tasks with prioritized scheduling
    xTaskCreate(TaskSensorRead, "SensorRead",
                100,  // Stack size (words)
                NULL,
                2,  // Priority 2 (Higher: critical timing for sampling)
                NULL);

    xTaskCreate(TaskSerialDisplay, "SerialDisplay",
                128,  // Stack size (words)
                NULL,
                1,  // Priority 1 (Lower: display can be delayed)
                NULL);

    xTaskCreate(TaskHeartbeat, "Heartbeat",
                80,  // Stack size (words)
                NULL,
                1,  // Priority 1
                NULL);

    Serial.println(F("[SYSTEM] Starting Scheduler. Tasks activated."));
  } else {
    Serial.println(F("[ERROR] Failed to create FreeRTOS Queue!"));
    while (1)
      ;  // Freeze
  }
}

void loop() {
  // Bypassed!
}

// =============================================================
//  RTOS TASK IMPLEMENTATIONS
// =============================================================

/**
 * Producer Task: Polls the analog sensor and sends values to the queue.
 * Runs at 5 Hz (every 200ms).
 */
void TaskSensorRead(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    // Read raw sensor data
    int rawValue = analogRead(A0);

    // Send the value to the back of the queue
    // Arguments: Queue Handle, Pointer to item, Block Time (0 = do not wait if full)
    BaseType_t status = xQueueSend(sensorQueue, &rawValue, 0);

    if (status != pdPASS) {
      Serial.println(F("[PRODUCER ERROR] Queue is full! Data dropped."));
    }

    // Delay task for 200ms
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

/**
 * Consumer Task: Reads values from the queue and prints them to the console.
 * Blocks indefinitely when the queue is empty.
 */
void TaskSerialDisplay(void *pvParameters) {
  (void)pvParameters;
  int receivedValue = 0;

  for (;;) {
    // Read value from the front of the queue
    // Arguments: Queue Handle, Buffer pointer, Block Time (portMAX_DELAY = wait forever)
    // The task enters the Blocked state automatically if the queue is empty.
    if (xQueueReceive(sensorQueue, &receivedValue, portMAX_DELAY) == pdPASS) {
      // Print the received value and compute voltage
      float voltage = (receivedValue * 5.0f) / 1023.0f;

      Serial.print(F("[CONSUMER] Received ADC: "));
      Serial.print(receivedValue);
      Serial.print(F(" | Calculated Voltage: "));
      Serial.print(voltage, 2);
      Serial.println(F(" V"));
    }
  }
}

/**
 * Heartbeat Task: Blinks the onboard LED at 5 Hz (100ms ON / 100ms OFF).
 * Demonstrates independent multitasking.
 */
void TaskHeartbeat(void *pvParameters) {
  (void)pvParameters;
  pinMode(13, OUTPUT);

  for (;;) {
    digitalWrite(13, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    digitalWrite(13, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

# Day 88: Hardware Interrupts & Software Debouncing (E-Stop and Pin Loopback Simulation)

Welcome to Day 88! Today we dive into the core execution architecture of microcontrollers: **Hardware Interrupts**. We will explore how to interrupt the main loop execution instantly in response to external real-world events, configure Interrupt Service Routines (ISRs) under critical constraints, and implement a software debouncing algorithm. To test it without physical buttons, we will design an on-board **Hardware Loopback Simulator** by jumpering two digital pins!

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Resistor.jpg" alt="Resistor" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Rotary_Encoder.jpg" alt="Rotary Encoder" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

In standard microcontroller code, we use the `loop()` function to check the status of pins. This is called **polling**. While simple, polling has a fatal flaw: if your loop is doing heavy calculations, waiting for a sensor read, or communicating over I2C, it might take 10ms, 50ms, or even 1 second to complete a single iteration.
If a critical event happens during that time—such as a robot hitting a wall limit switch, or a user pressing an **Emergency Stop (E-Stop)** button—the microcontroller will miss it or react too late, causing mechanical damage or injury.

### What is an Interrupt?
An **Interrupt** is a hardware mechanism that forces the CPU to pause whatever it is doing, save its current state, jump to a special function called an **Interrupt Service Routine (ISR)**, execute it, and then resume the main program. 

```
Normal Program Flow:    ───► [ Loop Code ] ───► [ Loop Code ] ───► [ Loop Code ] ───►
                              │                                      ▲
                              ▼ (Pin 2 goes LOW)                     │ (ISR finished)
Interrupt Event:              └─────────────► [ RUN ISR ] ───────────┘
```

In robotics, interrupts are mandatory for:
1. **Safety critical events** (E-Stops, collision bumpers).
2. **High-frequency pulses** (Rotary encoder ticks, tachometers).
3. **Precise timing events** (Timer-driven execution).

---

## 🔬 Physics & Hardware Theory

### 1. Mechanical Button Bounce
When you press a mechanical switch, the two metal contacts do not make a clean connection instantly. Instead, they spring back and forth (bounce) against each other for a brief moment—typically **1 to 10 milliseconds**—before settling down.
Because the Arduino Uno's clock runs at 16 MHz, it sees these microsecond-scale logic transitions as multiple individual presses. If you attach an interrupt to a bouncy button, a single press will trigger the ISR 5 to 20 times!

### 2. The AVR Interrupt Architecture (INT0 and INT1)
On the ATmega328P (Arduino Uno), there are two dedicated external hardware interrupt pins:
- **INT0** on Digital Pin 2
- **INT1** on Digital Pin 3

When an interrupt is triggered, the AVR hardware:
1. Disables global interrupts by clearing the Global Interrupt Enable bit in the Status Register (`SREG`).
2. Jumps to the corresponding address in the **Interrupt Vector Table**.
3. Runs the user's ISR.
4. Restores the `SREG` register, re-enabling global interrupts, and returns.

### 3. Critical Rules of ISR Programming
Because interrupts halt the rest of the program, they must follow strict rules:
- **Keep it Short**: Execute only basic state changes or counters.
- **No Blocking Code**: Never call `delay()` or wait inside an ISR.
- **No Serial I/O**: `Serial.print()` relies on interrupts to transmit data; calling it inside an ISR can lock up the MCU.
- **Volatile Keyword**: Any variable shared between the ISR and the main loop must be marked `volatile`. This tells the compiler to always read the variable from SRAM, preventing it from optimizing it into a CPU register where the ISR wouldn't see it.
- **Atomic Operations**: On 8-bit AVR microcontrollers, reading a 16-bit or 32-bit variable (like a `uint32_t` or `unsigned long`) takes multiple clock cycles. If an interrupt fires halfway through reading a 32-bit variable in `loop()`, the data will be corrupted! We must wrap these reads in a block that temporarily disables interrupts:
  ```cpp
  noInterrupts();
  unsigned long safeCopy = mySharedVariable;
  interrupts();
  ```

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| Jumper Wire | 1 | Loopback connection from Pin 4 to Pin 2 |
| Pushbutton (Optional) | 1 | For testing real physical bounces |

---

## 🔌 Pin-to-Pin Wiring

| Connect From Pin | Connect To Pin | Description |
| :--- | :--- | :--- |
| **Digital Pin 4** | **Digital Pin 2 (INT0)** | Loopback wire to generate simulated button pulses |
| **GND** (Optional) | **Digital Pin 2 (INT0)** | Connect via physical button if not using loopback |

---

## 💾 Alternatives to Software Debouncing

| Debouncing Method | Type | Components | Cost | CPU Overhead | Performance |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Software Lockout (Our Code)** | Software | None | Free | Very Low | Excellent. Simple window check. |
| **Hardware RC Filter** | Hardware | 1 Resistor, 1 Capacitor | Low | Zero | Good. Rounds off sharp bounce edges. |
| **Schmitt Trigger IC** | Hardware | MC14490 or 74HC14 | Moderate | Zero | Perfect. Cleans up slow analog transitions into sharp digital edges. |
| **Polling Timer Check** | Software | None | Free | Moderate | Poor. Requires constant polling in loop. |

---

## 💻 How to Test & Validate

1. Connect a single jumper wire between **Pin 4** and **Pin 2** on your Arduino Uno.
2. Open the Arduino IDE, load [Day_88_Hardware_Interrupts.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_88_Hardware_Interrupts/Day_88_Hardware_Interrupts.ino), and select the COM port.
3. Open the **Serial Monitor** at **9600 Baud**.
4. You will see the CLI prompt.
5. **Clean Press Test**:
   - Send `c`.
   - The simulator pulls Pin 4 LOW for 100ms.
   - The output should display:
     - `Raw ISR Triggers: 1`
     - `Valid Debounced Counts: 1`
6. **Bouncy Press Test**:
   - Send `b`.
   - The simulator toggles Pin 4 rapidly to mimic contact bounce.
   - The output should display:
     - `Raw ISR Triggers: 5` (or more)
     - `Valid Debounced Counts: 1`
     - `Bounces Filtered Out: 4`
   - This proves the software debounce logic successfully rejected the high-speed contact bounces!
7. **Emergency Stop (E-Stop) Test**:
   - Send `e`.
   - The system triggers E-stop status and locks the main loop in a safety halt state.
   - Send `r` to clear the E-Stop state and reset statistics back to zero.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| `Raw ISR Triggers` is 0 during simulation | Missing jumper wire | Ensure a jumper wire is plugged firmly into Digital Pin 4 and Digital Pin 2. |
| Physical button triggers twice when pressed | Debounce lockout window is too small | Increase `DEBOUNCE_DELAY_MS` in the code to `200` or `250` ms. |
| System hangs when E-Stop is triggered | Intended behavior | This is a safety lock. Send `r` in the serial input to clear the error. |
| Shared variables read corrupt values in main loop | Missing atomic block | Ensure you use `noInterrupts()` and `interrupts()` when copying volatile variables in `loop()`. |

## 🧠 Code Explanation

Let's break down how Hardware Interrupts respond instantly to physical events:

### 1. Bypassing the Main Loop
```cpp
attachInterrupt(digitalPinToInterrupt(2), buttonISR, FALLING);
```
- If you use `digitalRead()` inside `loop()`, you might miss a button press if the Arduino is busy doing math or executing a `delay()`.
- An Interrupt connects directly to the CPU core. When Pin 2 drops from HIGH to LOW (`FALLING`), the CPU literally freezes whatever it was doing in `loop()`, jumps instantly to the `buttonISR()` function, executes it, and goes right back to where it left off. This is mandatory for Emergency Stop (E-Stop) buttons!

### 2. The Mechanical Bounce Problem
- When two pieces of metal touch inside a physical button, they act like a tiny diving board, bouncing on a microscopic level. The logic pin goes `HIGH-LOW-HIGH-LOW` in a span of 2 milliseconds.
- Because hardware interrupts are so fast, the Arduino will register 1 physical press as 5 to 10 separate triggers!

### 3. Software Debouncing inside the ISR
```cpp
if (currentTime - lastInterruptTime > DEBOUNCE_DELAY_MS) {
    debouncedTriggerCount++;
    lastInterruptTime = currentTime;
}
```
- Inside the ISR, we compare the current timestamp to the last time we processed a trigger. If it's been less than 150ms, we mathematically ignore the trigger because we know it's a physical metal bounce. We only register "true" presses!

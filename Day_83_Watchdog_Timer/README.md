# Day 83: Watchdog Timer (WDT) Implementation (System Recovery & MCUSR Diagnostics)

Welcome to Day 83! Today we study **firmware fault tolerance** and **system recovery**. We will implement the AVR hardware **Watchdog Timer (WDT)** using `avr/wdt.h`. We will analyze the **MCU Status Register (MCUSR)** to diagnose reboot triggers, establish a safe watchdog feeding cycle, and build a crash simulator to witness a full hardware CPU reset and diagnostic recovery.

---

## 🎯 The "Why" and "What"

In real-world mechatronics, microcontrollers are deployed in harsh environments.
- **The Threat**: Electromagnetic interference (EMI) from high-power motors, static discharges, power brownouts, or logical bugs (such as an infinite loop waiting for a broken sensor) can cause the CPU to lock up, freeze, or jump to undefined program instructions.
- **The Solution**: A **Watchdog Timer (WDT)** is an on-chip hardware timer. When enabled, it continuously counts down. The program must periodically reset ("feed") this timer. If the program freezes, the timer overflows, pulling the microcontroller's hardware Reset line. This reboot recovers the system automatically, preventing catastrophic failures.

---

## 🔬 Physics & Hardware Theory

### 1. The Watchdog Oscillator Physics
The Watchdog Timer on the ATmega328P is completely independent of the main system clock.
- **128 kHz RC Oscillator**: The WDT is driven by an internal, low-power $128\,\text{kHz}$ RC oscillator. This oscillator is separate from the main $16\,\text{MHz}$ crystal oscillator.
- **Fail-Safe Integrity**: Even if the external crystal oscillator fails or the main CPU clock is shorted, the WDT oscillator will continue to run and will successfully reset the processor.

### 2. MCU Status Register (MCUSR)
When the Arduino reboots, the CPU records what caused the reset in the **MCU Status Register (MCUSR)**. By reading this register during `setup()`, we can determine the health of our system:

```
MCUSR Bits:
┌─────┬─────┬─────┬─────┬────────┬────────┬────────┬────────┐
│  -  │  -  │  -  │  -  │  WDRF  │  BORF  │  EXTRF │  PORF  │
└─────┴─────┴─────┴─────┴────┬───┴────┬───┴────┬───┴────┬───┘
                             │        │        │        └─ Power-On Reset
                             │        │        └────────── Reset Pin (Button)
                             │        └─────────────────── Brown-out (Power dip)
                             └──────────────────────────── Watchdog Reset
```

- **PORF (Bit 0)**: Power-On Reset. Set if power was completely disconnected and reconnected.
- **EXTRF (Bit 1)**: External Reset. Set if the physical reset button was pressed.
- **BORF (Bit 2)**: Brown-out Reset. Set if supply voltage dipped below the threshold (e.g. 2.7V or 4.3V).
- **WDRF (Bit 3)**: Watchdog Reset. Set if the watchdog timer timed out.

*Note: In setup(), we must clear these flags (`MCUSR = 0x00`) so that we can accurately detect the cause of the next reboot.*

### 3. The Boot Loop Trap
If the watchdog is configured for a very short timeout (e.g. $15\,\text{ms}$) and the bootloader or `setup()` initialization takes longer than $15\,\text{ms}$, the watchdog will trigger a reset before the main loop is reached. This puts the microcontroller into an **infinite boot loop**.
- **The Fix**: We call `wdt_disable()` as the very first line of `setup()` to disable the timer until the initialization sequence completes safely.

---

## 🔩 Components Needed

No external components are required. The project runs on the Arduino's internal hardware registers and uses the built-in LED (Pin 13) for indicators.

---

## 🔌 Pin-to-Pin Wiring

No external wiring is required.

---

## 💾 System Recovery Alternatives

| Method | Response Time | Hardware Complexity | Reliability | Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **Internal WDT (Our Choice)**| Configurable ($15\,\text{ms} - 8\,\text{s}$) | Zero (on-chip) | High | Standard embedded systems. |
| **External Watchdog IC** | Fixed/HW set | Low (needs external IC) | Extremely High | Safety-critical space/medical systems. Immune to chip locks. |
| **Windowed Watchdog** | Milliseconds | Zero (requires WDT setup) | Very High | Triggers if fed *too early* or *too late*, capturing clock drifts. |
| **Software Supervisor** | Seconds | Zero (cooperative code) | Low | Multi-core systems, does not recover from hard CPU lockups. |

---

## 💻 How to Test & Validate

1. Upload [Day_83_Watchdog_Timer.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_83_Watchdog_Timer/Day_83_Watchdog_Timer.ino) to the Arduino Uno.
2. Open the **Serial Monitor** at **9600 Baud**.
3. **Observation - Normal Boot**:
   - The onboard LED flashes rapidly 5 times to show a reboot occurred.
   - The console prints:
     `[DIAGNOSTICS] MCUSR register: 0x01` (or `0x02` if you pressed the reset button).
     `[SYSTEM] Reset source: Power-On Reset (Normal Power-up).`
     `[SYSTEM] Watchdog Timer armed with 2.0-second timeout.`
   - The LED begins to pulse at a steady 1 Hz (heartbeat). The watchdog is fed continuously.
4. **Observation - Simulated Crash**:
   - Type `k` in the Serial input line and press Enter.
   - The console reports:
     `[FAULT INJECTION] Simulating firmware hang (infinite loop) now!`
     `[SYSTEM] Watchdog feeding halted. CPU will reset in 2 seconds...`
   - The LED stops blinking and stays solid HIGH.
   - Wait exactly 2 seconds. The LED flashes rapidly 5 times as the bootloader restarts.
   - The console prints:
     `[DIAGNOSTICS] MCUSR register: 0x08` (indicating `WDRF` is set).
     `[SYSTEM] CRITICAL: Processor was reset by WATCHDOG TIMER timeout!`

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| Arduino gets stuck in an infinite reset loop | WDT not disabled immediately on boot | Ensure `wdt_disable()` is called at the beginning of `setup()`. Some older Arduino Uno bootloaders do not clear the WDT flag on reset, causing an infinite boot loop. Flash the Optiboot bootloader to resolve this, or increase the WDT timeout range. |
| Watchdog resets during normal operation | Main loop taking longer than 2.0 seconds | Ensure no slow blocking operations (like `delay()` or long `while` loops waiting for sensors) exist in your code. Sprinkle `wdt_reset()` in slow functions, or increase the WDT window (e.g., `WDTO_4S`). |
| MCUSR reads `0x00` on boot | Register cleared by bootloader | Some bootloaders read and clear `MCUSR` before jumping to the main sketch. Optiboot preserves this register, whereas older stock bootloaders might clear it. |

## 🧠 Code Explanation

Let's break down how we build self-recovering, crash-proof firmware:

### 1. Hardware Watchdog Overview
- Deep inside the silicon of the ATmega328P is a totally independent countdown timer running on its own dedicated 128 kHz oscillator. Even if the main CPU crystal breaks or the software freezes completely, this timer keeps ticking.

### 2. Arming and "Feeding the Dog"
```cpp
wdt_enable(WDTO_2S);
// Inside loop:
wdt_reset();
```
- We arm the Watchdog with a 2-second timeout.
- Inside our `loop()`, we call `wdt_reset()`. This "feeds the dog" by pushing the countdown back to 2 seconds.
- If our code gets stuck in an infinite loop (which we simulate by pressing 'k'), `wdt_reset()` is never called. The countdown hits zero, and the Watchdog physically pulls the CPU reset pin, instantly rebooting the crashed system!

### 3. Forensic Crash Diagnostics
```cpp
byte resetSource = MCUSR;
```
- How do we know *why* the robot rebooted? Did someone unplug it, or did it crash?
- The MCU Status Register (`MCUSR`) sets a specific bit in hardware based on what triggered the reboot. We read this on boot to detect if a Watchdog Timeout (`WDRF`), Power-Outage (`PORF`), or external reset button (`EXTRF`) caused the restart.

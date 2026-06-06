# Day 84: Deep Sleep and Power Saving Modes (AVR Sleep Manager)

Welcome to Day 84! Today we explore **low-power embedded systems engineering**. We will configure the **ATmega328P** to enter its deepest sleep state (**Power-down Mode**), write direct register commands to shut down the **Analog-to-Digital Converter (ADC)**, and implement a **hardware interrupt wake-up (INT0)** to wake the controller from sleep instantly.

---

## 🎯 The "Why" and "What"

For battery-powered robots, remote sensor stations, and wearable electronics, power is the primary constraint.
- **Active Draw**: A standard Arduino Uno consumes about $15 - 20\,\text{mA}$ while running. If powered by a standard $9\text{V}$ battery ($600\,\text{mAh}$ capacity), it will run for less than $30\,\text{hours}$.
- **Sleep Draw**: By placing the microcontroller into **Deep Sleep (Power-down Mode)** during periods of inactivity, we can reduce the chip's current draw to **under $1\,\mu\text{A}$**. This extends battery life from days to years.

Writing low-power firmware requires understanding how to turn off internal peripherals (like clocks, timers, and converters) and configure hardware interrupts to wake the CPU.

---

## 🔬 Physics & Hardware Theory

### 1. Power-down Sleep Mode & Clock Gating
The ATmega328P features six software-selectable sleep modes. The lowest power state is `SLEEP_MODE_PWR_DOWN`.
- **Clock Gating**: In this mode, the CPU clock and all internal I/O clocks are gated (stopped). The external crystal oscillator halts.
- **Halted Operations**: All active timers (Timer0, Timer1, Timer2) and CPU execution freeze.
- **Wake-up Trigger**: The only way to wake the processor is through a hardware reset, a watchdog timeout, or an **asynchronous external interrupt**.

### 2. ADC Power Dissipation
The Analog-to-Digital Converter (ADC) consists of a successive-approximation register and an active analog comparator. Even when the CPU clock is halted, the analog comparator remains biased and continuously draws about **$250\,\mu\text{A}$** of current.
- **The Solution**: We disable the ADC by clearing the Analog Digital Enable (`ADEN`) bit in the ADC Control and Status Register A (`ADCSRA`) before sleeping:
  $$\text{ADCSRA} = \text{ADCSRA} \ \& \ \sim\!(1 \ll \text{ADEN})$$
- We re-enable it (`ADCSRA |= (1 << ADEN)`) immediately upon wake-up to restore analog reading capabilities.

### 3. Level vs. Edge Interrupts
In Power-down mode, because all internal clock trees are stopped, the microcontroller cannot sample digital edges (such as a RISING or FALLING transition).
- **The Rule**: The only external interrupt trigger that can wake the MCU from Power-down is a **LOW Level interrupt** on Pin 2 (INT0) or Pin 3 (INT1).
- When the pin is pulled LOW, asynchronous logic bypasses the clock tree to trigger the wake-up sequence.

```
       Active State (15mA)          Inactivity (5s)          Power-down Sleep (<1µA)
 ┌─────────────────────────────┐                    ┌─────────────────────────────┐
 │ Clocks running, CPU active  │ ─────────────────► │ Clocks gated, Osc stopped   │
 │ ADC enabled, Heartbeat LED  │                    │ ADC disabled, LED off       │
 └─────────────────────────────┘                    └─────────────────────────────┘
                ▲                                                  │
                │                                                  │
                └─────────────── Pin D2 Pulled LOW ────────────────┘
                            (Asynchronous INT0 Trigger)
```

### 4. Board-Level Power Leakage
Note that while the ATmega328P chip draws $<1\,\mu\text{A}$ in deep sleep, a standard Arduino Uno board contains other components that continue to consume power:
- **USB-to-Serial Chip** (ATmega16U2): ~10 mA.
- **5V Linear Regulator**: ~3 mA.
- **Onboard Power LED**: ~2 mA.
- To achieve true micro-ampere operation in a custom design, use a bare ATmega328P chip on a breadboard or an Arduino board designed for low power (like the Arduino Pro Mini, with its regulator and power LED removed).

---

## 🔩 Components Needed

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| Arduino Uno | 1 | Microcontroller |
| Tactile Pushbutton | 1 | Wake-up trigger button |
| Breadboard & Wires | 1 | Connections |

---

## 🔌 Pin-to-Pin Wiring

| Button Pin | Arduino Uno Pin | Description |
| :--- | :--- | :--- |
| **Pin 1** | **D2** | INT0 Wake-up input (uses internal pull-up) |
| **Pin 2** | **GND** | Button Ground |
| **LED (Onboard)** | **D13** | Status Indicator |

---

## 💾 Power Optimization Alternatives

| Sleep Mode | CPU Clock | I/O Clock | ADC Power | Current Draw (Chip Only) | Wake-up Source |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Power-down** | Halted | Halted | Off (manual) | **$< 1\,\mu\text{A}$** | INT0/INT1 Low Level, Reset |
| **Power-save** | Halted | Halted | Off | $\sim 1.6\,\mu\text{A}$ | Timer2 (Asynchronous), INT0/1 |
| **Idle** | Halted | Active | On | $\sim 15\,\text{mA}$ | Any interrupt, Serial, Timers |
| **Active (Run)**| Active | Active | On | $\sim 15\,\text{mA}$ | Running normally |

---

## 💻 How to Test & Validate

1. Wire the pushbutton to Pin 2 and GND of the Arduino Uno.
2. Upload [Day_84_Deep_Sleep.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_84_Deep_Sleep/Day_84_Deep_Sleep.ino).
3. Open the **Serial Monitor** at **9600 Baud**.
4. **Observation - Active Mode**:
   - The console prints system initialization diagnostics.
   - The built-in LED blinks slowly, indicating active state.
   - Press Pin 2 button. The console prints: `[ACTIVITY] Button pressed. Timer reset.` Uptime resets.
5. **Observation - Sleeping**:
   - Leave the button untouched for 5 seconds.
   - The console outputs:
     `[SYSTEM] Inactivity timeout reached. Entering Deep Sleep now...`
     `[SYSTEM] Serial console and MCU clocks halting.`
   - The built-in LED turns off completely. The board is now asleep.
6. **Observation - Waking Up**:
   - Press the button on Pin 2.
   - The LED instantly flashes rapidly 3 times, and the console outputs:
     `[WAKE] System woke up from deep sleep.`
   - The system is active again. It will run for another 5 seconds before returning to sleep unless the button is pressed.

---

## 🛠️ Troubleshooting Guide

| Symptom | Likely Cause | Fix |
| :--- | :--- | :--- |
| System enters sleep but wakes up instantly | Floating interrupt pin | Ensure Pin 2 is configured with `INPUT_PULLUP`. A floating pin will drift LOW and trigger false wake-ups. |
| Current draw in sleep is still high (e.g. 15mA) | Board-level power consumption | This is due to the Arduino Uno's onboard USB chip and linear regulator. To measure true micro-ampere sleep current, measure the current on a bare ATmega328P or an Arduino Pro Mini with LEDs removed. |
| Serial Monitor prints garbage after waking | Clock synchronization lag | After sleep, the internal clock takes a few microseconds to stabilize. The sketch disables interrupts and serial operations during transition to prevent garbage characters. |
| Edge interrupt (`RISING`/`FALLING`) fails to wake | Clock halted | Edge interrupts require the I/O clock to sample the transition. You must use `LOW` level interrupt mode for wake-ups in Power-down sleep. |

## 🧠 Code Explanation

Let's break down how we drastically reduce power consumption for battery operations:

### 1. Putting the CPU to Sleep
```cpp
set_sleep_mode(SLEEP_MODE_PWR_DOWN);
sleep_cpu();
```
- A standard `delay(1000)` doesn't save power; the CPU is still running at 16 MHz executing empty math loops, burning ~15mA.
- `sleep_cpu()` physically halts the 16 MHz crystal oscillator! The CPU completely freezes on that line of code. By using `SLEEP_MODE_PWR_DOWN`, the chip's current draw drops from 15,000 µA down to less than 1 µA!

### 2. Disabling Power-Hungry Peripherals
```cpp
ADCSRA &= ~(1 << ADEN); // Turn off ADC
```
- Even if the CPU clock is halted, the Analog-to-Digital Converter uses active silicon comparators that burn ~250 µA in the background. We manipulate the `ADCSRA` register to physically disconnect power to the ADC before sleeping.

### 3. Waking Up via Hardware Interrupts
```cpp
attachInterrupt(digitalPinToInterrupt(WAKE_BUTTON_PIN), wakeUpISR, LOW);
```
- Because the CPU is frozen, it cannot check `digitalRead()`.
- We map an external button to a hardware interrupt (INT0). When the voltage drops LOW, it triggers an asynchronous logic gate deep in the silicon that instantly restarts the main 16 MHz clock, executes the ISR, and resumes our code right after `sleep_cpu()`!

# Day 54: Addressable RGB LED Strip (Bit-Banged Assembly)

Welcome to Day 54 of the 100-Day Arduino Masterclass! Today, we dive into **sub-microsecond timing and low-level AVR architecture** by interfacing a **WS2812B Addressable RGB LED Strip (NeoPixel)**. To understand how ultra-precise digital protocols are generated on resource-constrained microcontrollers, we will write a raw **bit-banged transmitter using inline assembly nops and direct port register manipulation** from scratch, without external LED libraries (like Adafruit_NeoPixel or FastLED).

---

## 🎯 The "Why" and "What"

Driving multiple RGB LEDs using standard analog methods requires three PWM pins and three power transistors *per LED*, leading to massive wiring bundles.
* **The Problem:** The WS2812B solves this by integrating a control chip inside the LED package, allowing hundreds of LEDs to be daisy-chained over a **single data line**. However, this control chip uses a strict single-wire protocol running at a high frequency of **$800\,\text{kHz}$**. At this speed, the difference between a binary `0` and a binary `1` is determined by pulse widths of a few hundred nanoseconds. Standard Arduino C functions like `digitalWrite()` are too slow (taking $3-5\,\mu\text{s}$) to generate these pulses.
* **The Solution:** We implement direct port writes to bypass the Arduino abstraction layer and combine them with inline assembly `nop` (No Operation) instructions to halt the CPU for exact fractions of a microsecond.

---

## 🔬 Physics & Mathematics

### 1. WS2812B Time-Width Protocol
The WS2812B reads a stream of 24-bit color words (8 bits Green, 8 bits Red, 8 bits Blue) per LED. 
Each bit is represented by a specific ratio of HIGH to LOW time on the line:

```
  Bit '0' Waveform (0.4us HIGH, 0.85us LOW)
  +------\
  |      \_______________________
  <-0.4us-><-------0.85us------->
  
  Bit '1' Waveform (0.8us HIGH, 0.45us LOW)
  +--------------\
  |              \_______________
  <----0.8us-----><----0.45us---->
```

* **Latch/Reset**: Once all color bytes are streamed to the strip, holding the line LOW for **$\ge 300\,\mu\text{s}$** latches the color buffer and drives the physical LEDs.

---

### 2. AVR Clock Cycle Calculations
The ATmega328P microcontroller on the Arduino Uno runs at a clock frequency of **$16\,\text{MHz}$**.
The period of a single CPU clock cycle ($T_{\text{cycle}}$) is:
$$T_{\text{cycle}} = \frac{1}{16,000,000\,\text{Hz}} = 62.5\,\text{ns}$$

Let's calculate the required clock cycles for each pulse width:

#### Bit '0' High Pulse:
$$\text{Cycles} = \frac{0.40\,\mu\text{s}}{62.5\,\text{ns}} = \frac{400\,\text{ns}}{62.5\,\text{ns}} = 6.4\,\text{cycles}$$
We target **$6$ clock cycles** of HIGH state.

#### Bit '1' High Pulse:
$$\text{Cycles} = \frac{0.80\,\mu\text{s}}{62.5\,\text{ns}} = \frac{800\,\text{ns}}{62.5\,\text{ns}} = 12.8\,\text{cycles}$$
We target **$13$ clock cycles** of HIGH state.

In our code, direct writing to a port pointer (`*port = portValHigh`) takes a few cycles to execute. We fill the remaining duration with inline `__asm__ __volatile__ ("nop\n\t")` assembly directives, each representing exactly 1 clock cycle ($62.5\,\text{ns}$).

---

### 3. Interrupt Gates (Preventing Timing Corruptions)
The Arduino executes background interrupt routines (e.g. updating the millisecond counter via Timer 0). 
If a Timer 0 interrupt occurs during our sub-microsecond write loop, the CPU will jump to the interrupt handler for $3 - 5\,\mu\text{s}$, shifting the output line timing and corrupting the colors down the LED strip.
To prevent this, we surround our bit-stream loop with **Interrupt Gates**:
* `cli();` (Clear Global Interrupts) - Disables all interrupts during data transmission.
* `sei();` (Set Global Interrupts) - Re-enables interrupts once the strip data is complete.

---

## 🔄 LED Strip Protocol Comparison

| LED Chip Type | Interface Interface | Timing Jitter Sensitivity | Max LED Count | Onboard Driver Logic |
| :--- | :--- | :--- | :--- | :--- |
| **WS2812B (NeoPixel)** | **1-Wire (Data only)** | **Extremely High (Needs Assembly) (Our choice)** | **$\approx 500$ nodes** | **GRB (8 bits per channel)** |
| **APA102 (DotStar)** | 2-Wire (SPI: Data + Clock) | None (Clock driven) | $>2048$ nodes | BGR (plus 5-bit global brightness) |
| **WS2811** | 1-Wire (External chip) | High | $\approx 300$ nodes | RGB (External constant-current drivers) |
| **Analog RGB LED** | 3-Wire (R, G, B PWM) | None | Depends on Power | None (Analog voltage drive) |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x WS2812B Addressable LED Strip (or ring module, typically 8 or 16 LEDs)
* 1x Breadboard & Jumper wires
* **1x $330\,\Omega - 470\,\Omega$ resistor** (critical to absorb signal reflections on the data line)
* **1x $1000\,\mu\text{F}$ capacitor** (to smooth out current surges on the power rails)

---

## 🔌 Pin-to-Pin Wiring

> [!CAUTION]
> A single WS2812B LED draws up to **$60\,\text{mA}$** when lit at full white ($20\,\text{mA}$ each for Red, Green, and Blue). An 8-LED ring draws up to $480\,\text{mA}$, which is close to the limit of the Arduino Uno's regulator. If you connect a strip of **more than 8 LEDs**, you **MUST** power the strip from an external 5V power supply.

```
 Arduino D8 (Port B0) ---[ 330 Ohm Resistor ]---> DIN (LED Strip)
 Arduino GND <------------------------------------> GND (LED Strip)
 Arduino 5V  <------------------------------------> 5V (LED Strip)
```

If using an external power supply:
```
 External 5V (+) ---------------------------------> 5V (LED Strip)
 External GND (-) ---------+----------------------> GND (LED Strip)
                           |
 Arduino GND --------------+
```

---

## 💻 How to Test & Validate

1. Wire the strip to **Pin 8** as shown, placing a $330\,\Omega$ resistor in series with the DIN line.
2. Upload [Day_54_WS2812B_Bitbang.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_54_WS2812B_Bitbang/Day_54_WS2812B_Bitbang.ino) to the Uno.
3. Observe the animations:
   - **Animation 1**: Saturated red, green, and blue pixels sweeping across the strip.
   - **Animation 2**: A smooth, shifting rainbow color wheel.
   - **Animation 3**: A warm orange-fire breathing light effect where the entire strip pulses in intensity.

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The LEDs remain completely dark or output dim, random static colors**:
  * Verify the DIN connection is on **Pin 8** (Port B, Bit 0 on the Uno). If you are using a Mega or Leonardo, the port mappings are different, and the code must be updated to write to the correct port register for that board.
  * Ensure the shared Ground wire is connected between the external 5V power supply, the LED strip, and the Arduino GND pin.
* **The first few LEDs light up correctly, but the colors become corrupted down the line**:
  * High-speed signals suffer from electrical noise. Ensure your series resistor ($330\,\Omega - 470\,\Omega$) is placed close to the DIN pin of the first LED to absorb line reflections.
  * Keep the jumper wires between the Arduino and the LED strip as short as possible.
* **The Arduino crashes or resets repeatedly when the strip lights up**:
  * The strip is drawing too much current, causing a voltage brown-out. Place a large capacitor ($1000\,\mu\text{F}$) across the 5V and GND terminals of the strip, or use an external 5V regulator to power the LEDs.

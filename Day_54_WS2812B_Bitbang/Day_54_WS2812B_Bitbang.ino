/*
 * 100 Projects with Arduino - Day 54
 * Project: WS2812B Addressable RGB LED Strip (Bit-Banged Assembly Timings)
 *
 * DESCRIPTION:
 * This project interfaces a WS2812B (NeoPixel) addressable RGB LED strip.
 * Because the WS2812B uses a high-speed, single-wire sub-microsecond pulse-width protocol
 * running at 800 kHz, standard Arduino GPIO pin operations (digitalWrite) are too slow.
 * To demonstrate low-level firmware timing and AVR architecture capabilities:
 * 1. Assembly-Level Timing Loops: Uses direct port register writes combined with raw inline
 *    assembly instructions (`__asm__ __volatile__ ("nop\n\t")`) to generate pulse widths with
 *    nanosecond-level precision.
 * 2. Interrupt Gate Control: Temporarily disables global interrupts (cli/sei) during transmit
 *    frames to prevent timer interrupts from corrupting the critical high/low waveforms.
 * 3. Color Shift Mathematics: Generates smooth GRB transitions (rainbow cycles and breathing
 *    patterns) computed programmatically in memory.
 *
 * WS2812B SUB-MICROSECOND PROTOCOL TIMING (at 16 MHz = 62.5ns per clock cycle):
 * - Total Bit Time: 1.25µs (20 cycles)
 * - Bit '0':
 *   - High Time: 0.40µs (6-7 cycles)
 *   - Low Time:  0.85µs (13-14 cycles)
 * - Bit '1':
 *   - High Time: 0.80µs (13-14 cycles)
 *   - Low Time:  0.45µs (6-7 cycles)
 * - Latch/Reset time: Keep line low for >= 300µs to display/load colors.
 *
 * WIRING:
 * - WS2812B LED Strip (e.g., 8-LED ring or strip) -> Arduino Uno
 *   - VCC  -> 5V (For long strips, connect to an external 5V power supply)
 *   - GND  -> GND (Common ground)
 *   - DIN  -> Pin 8 (Port B, Bit 0)
 */

// --- CONFIGURATION ---
const int NUM_LEDS = 8;
const int BYTES_PER_LED = 3;  // GRB (Green, Red, Blue) order
const int TOTAL_BYTES = NUM_LEDS * BYTES_PER_LED;

uint8_t ledBuffer[TOTAL_BYTES];  // Framebuffer containing color bytes

void setup() {
  // Configure Pin 8 (PORTB bit 0) as OUTPUT
  DDRB |= 0x01;
  PORTB &= ~0x01;  // Drive low default

  clearStrip();
  show();
}

void loop() {
  // Cycle 1: Saturated color sweep (Red -> Green -> Blue)
  for (int colorIdx = 0; colorIdx < 3; colorIdx++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      clearStrip();
      setLEDColor(i, colorIdx == 0 ? 255 : 0, colorIdx == 1 ? 255 : 0, colorIdx == 2 ? 255 : 0);
      show();
      delay(120);
    }
  }

  // Cycle 2: Smooth rainbow fade
  for (int hue = 0; hue < 256; hue += 2) {
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t r, g, b;
      hueToRGB((hue + (i * 256 / NUM_LEDS)) & 0xFF, &r, &g, &b);
      setLEDColor(i, r, g, b);
    }
    show();
    delay(20);
  }

  // Cycle 3: Warm Breathing Light
  for (int brightness = 5; brightness < 120; brightness++) {
    fillStrip(brightness, brightness / 3, 0);  // Orange flame breath
    show();
    delay(10);
  }
  for (int brightness = 120; brightness >= 5; brightness--) {
    fillStrip(brightness, brightness / 3, 0);
    show();
    delay(15);
  }
}

// --- CORE WS2812B PROTOCOL TRANSMITTER ---

void show() {
  // Pin 8 corresponds to PORTB, Bit 0 on the ATmega328P
  volatile uint8_t* port = &PORTB;
  uint8_t pinMask = 0x01;

  uint8_t portValHigh = *port | pinMask;
  uint8_t portValLow = *port & ~pinMask;

  // Disable interrupts. Any interrupt during the sub-microsecond transmission
  // will cause timing violations, resulting in flickering or incorrect colors.
  cli();

  for (uint16_t i = 0; i < TOTAL_BYTES; i++) {
    uint8_t byteVal = ledBuffer[i];

    for (uint8_t bit = 0; bit < 8; bit++) {
      if (byteVal & 0x80) {  // MSB is 1
        // Waveform for '1': 0.8us HIGH, 0.45us LOW
        // Step 1: Drive pin HIGH
        *port = portValHigh;
        // Step 2: Hold HIGH for 12 clock cycles (12 * 62.5ns = 750ns)
        __asm__ __volatile__(
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t");
        // Step 3: Drive pin LOW
        *port = portValLow;
        // Step 4: Hold LOW for 6 clock cycles (6 * 62.5ns = 375ns)
        __asm__ __volatile__(
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t");
      } else {  // MSB is 0
        // Waveform for '0': 0.4us HIGH, 0.85us LOW
        // Step 1: Drive pin HIGH
        *port = portValHigh;
        // Step 2: Hold HIGH for 6 clock cycles (6 * 62.5ns = 375ns)
        __asm__ __volatile__(
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t");
        // Step 3: Drive pin LOW
        *port = portValLow;
        // Step 4: Hold LOW for 13 clock cycles (13 * 62.5ns = 812.5ns)
        __asm__ __volatile__(
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t");
      }
      byteVal <<= 1;  // Shift to next bit
    }
  }

  // Restore interrupts
  sei();

  // Latch command: Hold line low for 300 microseconds
  delayMicroseconds(300);
}

// --- FRAMEBUFFER MANIPULATORS ---

void setLEDColor(int ledIdx, uint8_t r, uint8_t g, uint8_t b) {
  if (ledIdx < NUM_LEDS) {
    int baseIdx = ledIdx * BYTES_PER_LED;
    ledBuffer[baseIdx] = g;  // WS2812B uses GRB format
    ledBuffer[baseIdx + 1] = r;
    ledBuffer[baseIdx + 2] = b;
  }
}

void fillStrip(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    setLEDColor(i, r, g, b);
  }
}

void clearStrip() {
  fillStrip(0, 0, 0);
}

// --- COLOR MATH: HUE-TO-RGB CONVERTER ---

void hueToRGB(uint8_t hue, uint8_t* r, uint8_t* g, uint8_t* b) {
  if (hue < 85) {
    *r = 255 - hue * 3;
    *g = hue * 3;
    *b = 0;
  } else if (hue < 170) {
    hue -= 85;
    *r = 0;
    *g = 255 - hue * 3;
    *b = hue * 3;
  } else {
    hue -= 170;
    *r = hue * 3;
    *g = 0;
    *b = 255 - hue * 3;
  }
}

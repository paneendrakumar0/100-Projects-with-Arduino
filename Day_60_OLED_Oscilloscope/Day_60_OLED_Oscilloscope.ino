/*
 * 100 Projects with Arduino - Day 60
 * Project: OLED Oscilloscope (Real-Time Waveform Display on SSD1306 128x64)
 *
 * DESCRIPTION:
 * This sketch implements a minimal oscilloscope that samples analog input (A0)
 * at a configurable rate and renders the live waveform on a 128x64 SSD1306 OLED
 * display over I2C. Each of the 128 horizontal pixel columns represents one sample;
 * the amplitude is mapped vertically across 48 pixels (leaving room for status bar).
 *
 * SSD1306 INTERNAL ARCHITECTURE:
 * The 128x64 OLED is organised as 8 "pages" (rows), each page = 8 vertical pixels.
 * We use a 128-byte RAM framebuffer per page (1024 bytes total). The MCU writes the
 * full buffer on each redraw cycle using the Page Addressing mode:
 *   CMD: 0x00 = Set Start Column Low nibble
 *   CMD: 0x10 = Set Start Column High nibble
 *   CMD: 0xB0..0xB7 = Set Page Address 0..7
 *
 * WAVEFORM RENDERING:
 * Each frame:
 *  1. Sample 128 ADC values from A0 (10-bit -> scaled to 0..47 pixel rows)
 *  2. Build a 128-column framebuffer by setting pixels at the sample y-position
 *  3. Flush the full framebuffer to the OLED via I2C
 *
 * TIME BASE (Horizontal):
 *  The horizontal axis represents time. With a sampling delay of D microseconds:
 *  Time per frame = 128 * D microseconds
 *  Effective sample rate = 1,000,000 / D Hz
 *
 * AMPLITUDE (Vertical):
 *  0 ADC count (0V)   -> pixel row 47 (bottom)
 *  1023 ADC count (5V) -> pixel row 0 (top)
 *  Pixel Y = 47 - (ADC_value * 47 / 1023)
 *
 * WIRING:
 *  SSD1306 VCC -> 3.3V (some modules tolerate 5V — check your breakout board)
 *  SSD1306 GND -> GND
 *  SSD1306 SDA -> A4
 *  SSD1306 SCL -> A5
 *  Signal Input -> A0 (connect any analog waveform: audio, sensor output, etc.)
 */

#include <Wire.h>

// --- OLED I2C ADDRESS ---
const uint8_t OLED_ADDR = 0x3C;

// --- DISPLAY DIMENSIONS ---
const uint8_t SCREEN_W = 128;
const uint8_t SCREEN_H = 64;
const uint8_t PAGES = 8;       // 64 pixels / 8 bits per page
const uint8_t WAVE_ROWS = 48;  // Pixels used for waveform (rows 0..47)
const uint8_t STATUS_Y = 56;   // Bottom 8 pixels for status text (page 7)

// --- FRAMEBUFFER ---
// Each element = 8 vertically stacked pixels (1 page column)
uint8_t framebuffer[SCREEN_W][PAGES];

// --- SAMPLING ---
const int SIGNAL_PIN = A0;
// Sampling delay in microseconds (decrease for higher freq signals)
uint16_t sampleDelayUs = 500;  // Default: ~2 kHz effective sample rate

// --- TRIGGER SETTINGS ---
// Rising edge trigger: wait for signal to cross TRIGGER_LEVEL before sampling
const int TRIGGER_LEVEL = 512;  // ADC midpoint (2.5V)
const bool TRIGGER_ENABLE = true;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);  // Fast I2C (400 kHz) for faster display refresh

  oledInit();
  clearFramebuffer();
  oledFlush();

  Serial.println(F("[OLED Scope] Initialized. Signal input on A0."));
  Serial.println(F("[OLED Scope] Adjust sampleDelayUs in code for time base."));
}

void loop() {
  // --- Trigger: wait for rising edge crossing ---
  if (TRIGGER_ENABLE) {
    unsigned long triggerTimeout = millis() + 50;  // 50ms timeout
    while (analogRead(SIGNAL_PIN) > TRIGGER_LEVEL && millis() < triggerTimeout);
    while (analogRead(SIGNAL_PIN) < TRIGGER_LEVEL && millis() < triggerTimeout);
  }

  // --- Sample 128 ADC values ---
  uint16_t samples[SCREEN_W];
  for (uint8_t i = 0; i < SCREEN_W; i++) {
    samples[i] = analogRead(SIGNAL_PIN);
    delayMicroseconds(sampleDelayUs);
  }

  // --- Build framebuffer from samples ---
  clearFramebuffer();
  for (uint8_t col = 0; col < SCREEN_W; col++) {
    // Map ADC to pixel row (invert: 0V = bottom, 5V = top)
    uint8_t pixelY = WAVE_ROWS - 1 - (uint8_t)((uint32_t)samples[col] * (WAVE_ROWS - 1) / 1023);
    setPixel(col, pixelY);

    // Draw vertical line from previous to current sample (anti-aliasing/connect dots)
    if (col > 0) {
      uint8_t prevY =
          WAVE_ROWS - 1 - (uint8_t)((uint32_t)samples[col - 1] * (WAVE_ROWS - 1) / 1023);
      int8_t dy = (int8_t)pixelY - (int8_t)prevY;
      if (dy > 1) {
        for (uint8_t y = prevY + 1; y < pixelY; y++) setPixel(col, y);
      } else if (dy < -1) {
        for (uint8_t y = pixelY + 1; y < prevY; y++) setPixel(col, y);
      }
    }
  }

  // --- Draw grid lines (horizontal center, vertical center) ---
  for (uint8_t x = 0; x < SCREEN_W; x += 8) {
    setPixel(x, WAVE_ROWS / 2);  // Midline
  }

  oledFlush();

  // --- Serial telemetry (optional, uncomment for debugging) ---
  // Serial.print(F("[Scope] Samples per frame: 128 | Delay: "));
  // Serial.print(sampleDelayUs); Serial.println(F(" us"));
}

// --- SET A SINGLE PIXEL IN THE FRAMEBUFFER ---
void setPixel(uint8_t x, uint8_t y) {
  if (x >= SCREEN_W || y >= SCREEN_H) return;
  uint8_t page = y / 8;
  uint8_t bit = y % 8;
  framebuffer[x][page] |= (1 << bit);
}

// --- CLEAR THE FRAMEBUFFER ---
void clearFramebuffer() {
  memset(framebuffer, 0, sizeof(framebuffer));
}

// --- FLUSH FRAMEBUFFER TO SSD1306 OVER I2C ---
void oledFlush() {
  for (uint8_t page = 0; page < PAGES; page++) {
    oledCommand(0xB0 | page);  // Set page address
    oledCommand(0x00);         // Set column low nibble = 0
    oledCommand(0x10);         // Set column high nibble = 0
    // Send 128 bytes of pixel data for this page
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40);  // Data continuation byte
    for (uint8_t col = 0; col < SCREEN_W; col++) {
      Wire.write(framebuffer[col][page]);
      // I2C buffer is 32 bytes on many AVR platforms — we flush every 28 bytes
      if ((col + 1) % 28 == 0) {
        Wire.endTransmission();
        Wire.beginTransmission(OLED_ADDR);
        Wire.write(0x40);
      }
    }
    Wire.endTransmission();
  }
}

// --- SEND A SINGLE COMMAND BYTE TO SSD1306 ---
void oledCommand(uint8_t cmd) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);  // Co=0, D/C=0 -> Command byte
  Wire.write(cmd);
  Wire.endTransmission();
}

// --- INITIALIZE SSD1306 ---
void oledInit() {
  delay(100);  // Let the OLED power up
  uint8_t cmds[] = {
      0xAE,        // Display OFF (sleep mode)
      0xD5, 0x80,  // Set display clock divide ratio
      0xA8, 0x3F,  // Set multiplex ratio (64 lines for 128x64)
      0xD3, 0x00,  // Set display offset = 0
      0x40,        // Set display start line = 0
      0x8D, 0x14,  // Enable charge pump
      0x20, 0x00,  // Set memory addressing mode to Horizontal
      0xA1,        // Segment re-map (column 127 mapped to SEG0)
      0xC8,        // COM output scan direction: remapped
      0xDA, 0x12,  // Set COM pins (alternative configuration)
      0x81, 0xCF,  // Set contrast
      0xD9, 0xF1,  // Set pre-charge period
      0xDB, 0x40,  // Set Vcomh deselect level
      0xA4,        // Entire display ON (resume from RAM)
      0xA6,        // Set normal display (not inverted)
      0xAF         // Display ON
  };
  for (uint8_t i = 0; i < sizeof(cmds); i++) {
    oledCommand(cmds[i]);
  }
}

/*
 * 100 Projects with Arduino - Day 66
 * Project: MAX7219 8x8 LED Matrix Driver (SPI Register Control)
 *
 * DESCRIPTION:
 * This project drives an 8x8 LED matrix using the MAX7219 LED driver IC via
 * direct SPI register writes — without any display library. The MAX7219 handles
 * all LED multiplexing internally; the MCU only needs to send 16-bit words
 * (register address + data) over SPI to update any row or global settings.
 *
 * MAX7219 REGISTER MAP:
 *   0x01–0x08 = Digit/Row registers (Row 1–Row 8), 8 bits = 8 column LEDs
 *   0x09      = Decode Mode  (0x00 = no BCD decode for LED matrix)
 *   0x0A      = Intensity    (0x00–0x0F, 0=dimmest, 15=brightest)
 *   0x0B      = Scan Limit   (0x07 = all 8 digits/rows active)
 *   0x0C      = Shutdown     (0x00=shutdown, 0x01=normal operation)
 *   0x0F      = Display Test (0x01=all LEDs ON, 0x00=normal)
 *
 * SPI PROTOCOL (MAX7219):
 *   - 16 bits per transaction: [D15:D12]=don't care, [D11:D8]=address, [D7:D0]=data
 *   - CS (LOAD pin) LOW before sending, HIGH after 16 bits to latch
 *   - MSB first, SPI Mode 0 (CPOL=0, CPHA=0)
 *   - Max clock: 10 MHz
 *
 * ANIMATION SHOWN:
 *   1. Full display test (all LEDs on briefly)
 *   2. Scrolling text "HI" across the matrix
 *   3. Bouncing ball animation
 *   4. Knight Rider / Cylon scanning effect
 *
 * WIRING:
 *   MAX7219 DIN  -> D11 (MOSI)
 *   MAX7219 CLK  -> D13 (SCK)
 *   MAX7219 LOAD -> D10 (CS)
 *   MAX7219 VCC  -> 5V
 *   MAX7219 GND  -> GND
 *   (100nF decoupling cap between VCC and GND, close to MAX7219)
 */

#include <SPI.h>

// --- SPI PINS ---
const int CS_PIN = 10;  // LOAD/CS pin

// --- MAX7219 REGISTER ADDRESSES ---
const uint8_t REG_NOOP        = 0x00;
const uint8_t REG_DIGIT0      = 0x01; // Row 1
const uint8_t REG_DECODE_MODE = 0x09;
const uint8_t REG_INTENSITY   = 0x0A;
const uint8_t REG_SCAN_LIMIT  = 0x0B;
const uint8_t REG_SHUTDOWN    = 0x0C;
const uint8_t REG_DISP_TEST   = 0x0F;

// --- FONT: 5x8 bitmap font for characters ---
// Each character = 5 bytes, each byte = one column, bit0=top row
const uint8_t FONT_5x8[][5] = {
  { 0x7C, 0x82, 0x82, 0x82, 0x7C }, // 0: 'A'
  { 0x3E, 0x49, 0x49, 0x49, 0x36 }, // 1: 'B'
  { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // 2: 'C'
  { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // 3: 'D'
  { 0x7F, 0x49, 0x49, 0x41, 0x41 }, // 4: 'E'
  { 0x7F, 0x09, 0x09, 0x01, 0x01 }, // 5: 'F'
  { 0x3E, 0x41, 0x49, 0x49, 0x7A }, // 6: 'G'
  { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // 7: 'H'
  { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // 8: 'I'
  { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // 9: 'J'
  { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // 10: 'K'
  { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // 11: 'L'
  { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, // 12: 'M'
  { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // 13: 'N'
  { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // 14: 'O'
  { 0x7F, 0x09, 0x09, 0x09, 0x06 }, // 15: 'P'
};

// --- FRAMEBUFFER: 8 rows, 8 columns per row ---
uint8_t framebuffer[8] = {0};

// --- SCROLLING MESSAGE ---
const char* MESSAGE = "HI ARDUINO";
const int   MSG_LEN = 10; // Length of MESSAGE

void setup() {
  Serial.begin(9600);
  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  // Initialize MAX7219
  maxWrite(REG_DISP_TEST,   0x00); // Normal mode (no test)
  maxWrite(REG_SHUTDOWN,    0x01); // Wake up
  maxWrite(REG_DECODE_MODE, 0x00); // No BCD decode (raw LED control)
  maxWrite(REG_SCAN_LIMIT,  0x07); // All 8 rows active
  maxWrite(REG_INTENSITY,   0x04); // Medium brightness

  clearDisplay();

  Serial.println(F("[MAX7219] 8x8 LED Matrix initialized. Starting animations..."));

  // --- Display Test: all LEDs on for 1 second ---
  maxWrite(REG_DISP_TEST, 0x01);
  delay(1000);
  maxWrite(REG_DISP_TEST, 0x00);
  clearDisplay();
  delay(300);
}

void loop() {
  // Demo 1: Scrolling text
  scrollText(MESSAGE, MSG_LEN, 80);
  delay(300);

  // Demo 2: Bouncing ball
  bouncingBall(3000);
  delay(300);

  // Demo 3: Knight rider scan
  knightRider(5, 60);
  delay(300);
}

// =============================================================
//  ANIMATION: SCROLL TEXT
// =============================================================
void scrollText(const char* msg, int len, int delayMs) {
  // Build wide buffer: concatenate all character columns with 1 blank col between
  // Each char = 5 cols + 1 blank = 6 cols total
  int totalCols = len * 6 + 8; // Extra 8 cols padding at start and end
  uint8_t* buf = (uint8_t*)calloc(totalCols, 1);
  if (!buf) return;

  int pos = 8; // Start after 8 blank columns
  for (int c = 0; c < len; c++) {
    char ch = msg[c];
    int fi = 0;
    if (ch >= 'A' && ch <= 'P') fi = ch - 'A';
    else fi = 14; // Default to 'O' for unknown chars
    for (int col = 0; col < 5; col++) buf[pos++] = FONT_5x8[fi][col];
    buf[pos++] = 0x00; // Column gap between characters
  }

  // Scroll through buffer
  for (int x = 0; x < totalCols - 8; x++) {
    for (int row = 0; row < 8; row++) {
      uint8_t pixel = 0;
      for (int col = 0; col < 8; col++) {
        uint8_t colByte = (x + col < totalCols) ? buf[x + col] : 0;
        if (colByte & (1 << row)) pixel |= (0x80 >> col);
      }
      framebuffer[row] = pixel;
    }
    flushFramebuffer();
    delay(delayMs);
  }

  free(buf);
  clearDisplay();
}

// =============================================================
//  ANIMATION: BOUNCING BALL
// =============================================================
void bouncingBall(unsigned long durationMs) {
  float bx = 3.5f, by = 3.5f;
  float vx = 0.9f, vy = 0.7f;
  unsigned long start = millis();

  while (millis() - start < durationMs) {
    memset(framebuffer, 0, sizeof(framebuffer));

    bx += vx; by += vy;
    if (bx <= 0 || bx >= 7) vx = -vx;
    if (by <= 0 || by >= 7) vy = -vy;

    int px = constrain((int)bx, 0, 7);
    int py = constrain((int)by, 0, 7);
    framebuffer[py] |= (0x80 >> px);

    flushFramebuffer();
    delay(60);
  }
  clearDisplay();
}

// =============================================================
//  ANIMATION: KNIGHT RIDER SCAN
// =============================================================
void knightRider(int passes, int delayMs) {
  for (int p = 0; p < passes; p++) {
    for (int row = 0; row < 8; row++) {
      memset(framebuffer, 0, sizeof(framebuffer));
      framebuffer[row] = 0xFF; // Full row of LEDs lit
      flushFramebuffer();
      delay(delayMs);
    }
    for (int row = 6; row >= 1; row--) {
      memset(framebuffer, 0, sizeof(framebuffer));
      framebuffer[row] = 0xFF;
      flushFramebuffer();
      delay(delayMs);
    }
  }
  clearDisplay();
}

// =============================================================
//  LOW-LEVEL: FLUSH FRAMEBUFFER TO MAX7219
// =============================================================
void flushFramebuffer() {
  for (uint8_t row = 0; row < 8; row++) {
    maxWrite(REG_DIGIT0 + row, framebuffer[row]);
  }
}

// =============================================================
//  LOW-LEVEL: CLEAR DISPLAY
// =============================================================
void clearDisplay() {
  memset(framebuffer, 0, sizeof(framebuffer));
  flushFramebuffer();
}

// =============================================================
//  LOW-LEVEL: WRITE 16-BIT WORD TO MAX7219 OVER SPI
// =============================================================
void maxWrite(uint8_t reg, uint8_t data) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(reg);   // Address byte (D11:D8 in the 16-bit word)
  SPI.transfer(data);  // Data byte (D7:D0)
  digitalWrite(CS_PIN, HIGH); // Latch on rising edge of LOAD
}

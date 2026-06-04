/*
 * 100 Projects with Arduino - Day 55
 * Project: Parallel HD44780 LCD (Direct 8-Bit GPIO Interface & Port Splitting)
 * 
 * DESCRIPTION:
 * This project interfaces a standard HD44780 character LCD using a raw 8-bit parallel data bus.
 * Rather than using standard helper libraries (LiquidCrystal) or simple serial interfaces (I2C), 
 * we write the parallel bus strobe waveforms, command-set structures, and custom CGRAM character
 * generation routines from scratch at the raw register level.
 * 
 * THE HARDWARE COUPLING & SERIAL DEBUGGING TRADEOFF:
 * - Direct 8-bit data buses require 8 GPIO pins.
 * - On the ATmega328P (Uno), Port D contains pins D0 to D7. However, D0 and D1 are the RX/TX lines 
 *   for the USB Serial hardware, crucial for diagnostics.
 * - To preserve Serial logging capability, we implement **Port Splitting**:
 *   - Value bits [0...5] are written to PORTD bits [2...7] (Uno Pins D2 to D7).
 *   - Value bits [6...7] are written to PORTB bits [0...1] (Uno Pins D8 to D9).
 *   - The data is reconstructed on the LCD bus by splitting and shifting bytes in software.
 * 
 * WIRING:
 * - HD44780 LCD Pinout -> Arduino Uno
 *   - Pin 1 (VSS)      -> GND
 *   - Pin 2 (VDD)      -> 5V
 *   - Pin 3 (VO)       -> Potentiometer wiper (Contrast adjustment)
 *   - Pin 4 (RS)       -> Pin A0 (PORTC bit 0 - Register Select: 0=Cmd, 1=Data)
 *   - Pin 5 (R/W)      -> GND (Write-only configuration to save a GPIO pin)
 *   - Pin 6 (E)        -> Pin A1 (PORTC bit 1 - Enable signal strobe)
 *   - Pin 7-12 (D0-D5) -> Pins D2 to D7 (PORTD bits 2 to 7)
 *   - Pin 13-14 (D6-D7)-> Pins D8 to D9 (PORTB bits 0 to 1)
 *   - Pin 15 (A)       -> 5V (Backlight Anode)
 *   - Pin 16 (K)       -> GND (Backlight Cathode)
 */

// --- PIN REGISTER BIT MASKS ---
const int PIN_RS = A0; // Command / Data select
const int PIN_E  = A1; // Enable Strobe

// Custom progress-bar block characters (5x8 pixels)
const uint8_t CUSTOM_CHARS[2][8] = {
  { 0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F }, // Empty box block [0]
  { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }  // Solid filled block [1]
};

void setup() {
  Serial.begin(9600);

  // Set Pins A0, A1 (RS, E) as OUTPUT
  pinMode(PIN_RS, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  digitalWrite(PIN_E, LOW);

  // Configure Data pins: D2-D7 (Port D) and D8-D9 (Port B) as OUTPUT
  // Port D DDRD bits 2-7
  DDRD |= 0xFC; 
  // Port B DDRB bits 0-1
  DDRB |= 0x03;

  Serial.println(F("[LCD] Starting HD44780 8-bit parallel initialization..."));
  initLCD();
  Serial.println(F("[LCD] Initialization Successful."));

  // Write custom characters to CGRAM
  loadCustomCharacters();

  // Print static layouts
  setCursor(0, 0);
  printString("Day 55 Parallel");
  
  setCursor(0, 1);
  printString("Loading: ");
}

void loop() {
  // Simple progressive loading bar using custom blocks
  for (int progress = 0; progress <= 7; progress++) {
    setCursor(9, 1);
    
    // Draw filled blocks
    for (int i = 0; i < progress; i++) {
      lcdWriteData(1); // Solid block
    }
    // Draw empty blocks
    for (int i = progress; i < 7; i++) {
      lcdWriteData(0); // Empty outline block
    }
    
    delay(400);
  }
}

// --- PORT SPLITTING WAVEFORM TRANSMITTER ---

void write8BitBus(uint8_t value) {
  // Value bit structure: [7 6 5 4 3 2 1 0]
  // Write bits [0...5] to PORTD bits [2...7], keeping D0, D1 (Serial TX/RX) untouched
  PORTD = (PORTD & 0x03) | ((value & 0x3F) << 2);
  
  // Write bits [6...7] to PORTB bits [0...1], keeping other Port B bits untouched
  PORTB = (PORTB & 0xFC) | ((value & 0xC0) >> 6);
}

void strobeEnable() {
  // The HD44780 latches the data bus on the falling edge of the Enable (E) pin.
  // Set Enable HIGH, wait for register settling, then pull LOW.
  digitalWrite(PIN_E, HIGH);
  delayMicroseconds(5); // Minimum pulse width is 450ns
  digitalWrite(PIN_E, LOW);
  delayMicroseconds(100); // Execution cycle delay
}

void lcdWriteCommand(uint8_t cmd) {
  digitalWrite(PIN_RS, LOW); // RS LOW = Command register
  write8BitBus(cmd);
  strobeEnable();
}

void lcdWriteData(uint8_t data) {
  digitalWrite(PIN_RS, HIGH); // RS HIGH = Data register
  write8BitBus(data);
  strobeEnable();
}

// --- HD44780 8-BIT INITIALIZATION SEQUENCE ---

void initLCD() {
  // HD44780 datasheet specifies a strict startup timing sequence:
  delay(50); // Wait >40ms after power-up

  // Initialization commands (3x 0x30 reset sequences)
  lcdWriteCommand(0x30);
  delay(5);  // Wait >4.1ms
  
  lcdWriteCommand(0x30);
  delayMicroseconds(150); // Wait >100us
  
  lcdWriteCommand(0x30);
  delayMicroseconds(150);

  // Function Set: 8-bit mode, 2-line mode, 5x8 pixel font
  lcdWriteCommand(0x38);
  
  // Display Control: Display ON, Cursor OFF, Blink OFF
  lcdWriteCommand(0x0C);
  
  // Clear Display
  lcdWriteCommand(0x01);
  delay(3); // Clear requires >1.53ms to erase internal DDRAM
  
  // Entry Mode Set: Increment cursor, no shifting
  lcdWriteCommand(0x06);
}

// --- LAYOUT & CURSOR GEOMETRY CONTROLLERS ---

void setCursor(uint8_t col, uint8_t row) {
  // Address offsets for DDRAM characters:
  // Row 0 start: 0x00; Row 1 start: 0x40.
  // Set DDRAM Address instruction command: 0x80 | DDRAM Address
  uint8_t address = col;
  if (row == 1) {
    address += 0x40;
  }
  lcdWriteCommand(0x80 | address);
}

void printString(const char* str) {
  while (*str) {
    lcdWriteData(*str++);
  }
}

// --- CUSTOM CHARACTER DESIGN IN CGRAM ---

void loadCustomCharacters() {
  // CGRAM base address for writing custom fonts is 0x40.
  // Each character occupies 8 bytes (5x8 grid).
  // Total of 8 custom characters can be programmed.
  lcdWriteCommand(0x40); // Set CGRAM pointer to start (char 0)

  for (int charIdx = 0; charIdx < 2; charIdx++) {
    for (int row = 0; row < 8; row++) {
      lcdWriteData(CUSTOM_CHARS[charIdx][row]);
    }
  }

  // Reset pointer back to DDRAM coordinates (address 0x00)
  lcdWriteCommand(0x80); 
}

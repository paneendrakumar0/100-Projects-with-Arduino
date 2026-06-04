/*
 * 100 Projects with Arduino - Day 58
 * Project: Hardware Timer1 PWM Generator (Direct AVR Register Configuration)
 * 
 * DESCRIPTION:
 * This project demonstrates how to configure the ATmega328P's 16-bit Timer1 hardware
 * peripheral in Fast PWM (Mode 14) with ICR1 as the TOP value. By writing directly
 * to TCCR1A, TCCR1B, ICR1, and OCR1A registers we bypass the Arduino analogWrite()
 * abstraction and gain full control over:
 *   - PWM Frequency: Set by ICR1 = (F_CPU / (Prescaler * Freq)) - 1
 *   - Duty Cycle:    Set by OCR1A = (DutyCycle / 100) * (ICR1 + 1)
 * 
 * A potentiometer on A0 maps the duty cycle from 0% to 100% in real time.
 * A second potentiometer (or serial command) can switch frequency preset bands.
 * 
 * TIMER1 FAST PWM MODE 14 (WGM13:WGM10 = 1110):
 *   - TCCR1A: COM1A1=1, COM1A0=0 -> Non-inverted output on OC1A (Pin 9)
 *   - TCCR1A: WGM11=1, WGM10=0
 *   - TCCR1B: WGM13=1, WGM12=1
 *   - TCCR1B: CS12:CS10 = Prescaler bits
 *   - ICR1:   TOP value (controls frequency)
 *   - OCR1A:  Compare value (controls duty cycle)
 * 
 * FREQUENCY EQUATION:
 *   f_PWM = F_CPU / (Prescaler * (ICR1 + 1))
 *   ICR1  = (F_CPU / (Prescaler * f_PWM)) - 1
 * 
 * Example at f_PWM = 1 kHz, Prescaler = 8:
 *   ICR1 = (16,000,000 / (8 * 1000)) - 1 = 1999
 * 
 * DUTY CYCLE EQUATION:
 *   OCR1A = (DutyCycle% / 100.0) * (ICR1 + 1)
 * 
 * WIRING:
 * - Potentiometer (Duty Cycle) -> A0 (Analog Input)
 * - PWM Output                 -> Pin 9 (OC1A, Timer1 Channel A)
 * - Oscilloscope probe         -> Pin 9 to verify waveform (optional)
 */

// --- PIN DEFINITIONS ---
const int DUTY_POT_PIN   = A0;
const int FREQ_POT_PIN   = A1;
const int LED_STATUS_PIN = 13;

// --- PWM FREQUENCY PRESETS (Hz) ---
const uint16_t FREQ_PRESETS[] = {100, 500, 1000, 5000, 10000, 25000};
const int NUM_PRESETS = 6;

// --- PRESCALER SELECTION ---
// For Timer1 Fast PWM: CS12:CS11:CS10
// 001 = /1, 010 = /8, 011 = /64, 100 = /256, 101 = /1024
const uint8_t  PRESCALER_BITS = 0b010; // /8 prescaler (good for 100Hz–25kHz range)
const uint16_t PRESCALER_VALUE = 8;

void setup() {
  Serial.begin(9600);
  
  pinMode(LED_STATUS_PIN, OUTPUT);
  
  // Configure Pin 9 (OC1A) as output
  pinMode(9, OUTPUT);

  // Initialize Timer1 in Fast PWM Mode 14 (TOP = ICR1)
  // Clear Timer1 config registers first
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; // Reset counter

  // TCCR1A: COM1A1=1 (non-inverted), WGM11=1, WGM10=0
  TCCR1A = _BV(COM1A1) | _BV(WGM11);

  // TCCR1B: WGM13=1, WGM12=1 (Fast PWM Mode 14) + prescaler bits
  TCCR1B = _BV(WGM13) | _BV(WGM12) | PRESCALER_BITS;

  // Set default frequency: 1 kHz
  uint16_t icr = computeICR(1000);
  ICR1  = icr;
  OCR1A = icr / 2; // Start at 50% duty cycle

  Serial.println(F("[PWM] Timer1 Hardware PWM Generator active."));
  Serial.println(F("[PWM] A0 = Duty Cycle (0-100%), A1 = Frequency Preset Selector"));
}

void loop() {
  // Read duty cycle from potentiometer (0-1023 -> 0-100%)
  int rawDuty = analogRead(DUTY_POT_PIN);
  float dutyCycle = (float)rawDuty / 1023.0 * 100.0;

  // Read frequency preset from second pot
  int rawFreq = analogRead(FREQ_POT_PIN);
  int presetIndex = map(rawFreq, 0, 1023, 0, NUM_PRESETS - 1);
  uint16_t targetFreq = FREQ_PRESETS[presetIndex];

  // Update ICR1 (frequency) and OCR1A (duty cycle) registers
  uint16_t icr = computeICR(targetFreq);
  ICR1  = icr;
  OCR1A = (uint16_t)((dutyCycle / 100.0) * (float)(icr + 1));

  // Blink LED at the PWM frequency for visual confirmation (scaled)
  digitalWrite(LED_STATUS_PIN, (millis() / (500 / max(presetIndex + 1, 1))) % 2);

  // Serial telemetry (throttled)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    Serial.print(F("[PWM] Freq: ")); Serial.print(targetFreq);
    Serial.print(F(" Hz | Duty: ")); Serial.print(dutyCycle, 1);
    Serial.print(F("% | ICR1: ")); Serial.print(icr);
    Serial.print(F(" | OCR1A: ")); Serial.println(OCR1A);
  }
}

// --- ICR1 COMPUTATION ---
uint16_t computeICR(uint16_t freq) {
  // ICR1 = (F_CPU / (Prescaler * Freq)) - 1
  uint32_t icr = ((uint32_t)F_CPU / ((uint32_t)PRESCALER_VALUE * (uint32_t)freq)) - 1;
  if (icr > 65535) icr = 65535; // Clamp to 16-bit max
  return (uint16_t)icr;
}

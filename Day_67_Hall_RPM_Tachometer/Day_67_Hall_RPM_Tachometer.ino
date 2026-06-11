/*
 * 100 Projects with Arduino - Day 67
 * Project: Hall Effect RPM Tachometer (Timer1 Input Capture Unit)
 *
 * DESCRIPTION:
 * This project measures the rotational speed (RPM) of a motor or wheel using
 * a Hall Effect sensor and the ATmega328P's Timer1 INPUT CAPTURE UNIT (ICU).
 *
 * HALL EFFECT SENSOR WORKING PRINCIPLE:
 * A Hall Effect sensor (e.g., A3144, SS49E) outputs a digital pulse when a
 * magnet attached to the rotating shaft passes near it. One pulse per revolution.
 *
 * TIMER1 INPUT CAPTURE UNIT (ICU):
 * The ICU is a hardware peripheral that automatically records the value of
 * Timer1 (TCNT1) into the Input Capture Register (ICR1) the INSTANT a selected
 * edge (rising or falling) is detected on ICP1 (Pin 8). This is much more
 * accurate than software interrupt timestamps (no interrupt latency jitter).
 *
 * RPM CALCULATION:
 * Given two consecutive capture timestamps (T1, T2) both in Timer1 ticks:
 *   Period_ticks = T2 - T1
 *   Period_seconds = Period_ticks / (F_CPU / Prescaler)
 *   Frequency_Hz   = 1 / Period_seconds
 *   RPM = Frequency_Hz * 60
 *
 * With Prescaler = 8, Timer1 at 2 MHz tick rate (0.5 us per tick):
 *   Period_seconds = Period_ticks / 2,000,000
 *   For 1000 RPM: Period = 60/1000 = 60 ms = 120,000 ticks
 *   For 10,000 RPM: Period = 6 ms = 12,000 ticks (well within 16-bit range)
 *   For 60 RPM: Period = 1 s = 2,000,000 ticks (OVERFLOW risk — handled below)
 *
 * OVERFLOW HANDLING:
 * If the motor is very slow (< ~2 RPM at prescaler=8), Timer1 overflows before
 * the next capture. We track overflow count to extend the effective period range.
 *
 * MAGNETS PER REVOLUTION:
 * If the shaft has N magnets, the sensor fires N pulses per revolution:
 *   RPM = Frequency_Hz * 60 / MAGNETS_PER_REV
 *
 * WIRING:
 *   Hall Sensor OUT -> D8  (ICP1 — Input Capture Pin)
 *   Hall Sensor VCC -> 5V
 *   Hall Sensor GND -> GND
 *   (10k pull-up between D8 and 5V recommended for open-collector sensors)
 *   LED indicator  -> D13
 */

// --- CONFIGURATION ---
const uint8_t MAGNETS_PER_REV = 1;                       // Magnets on the shaft (1 = 1 pulse/rev)
const uint16_t PRESCALER_VALUE = 8;                      // Timer1 prescaler (CS12:0 = 010)
const float TICK_FREQ = (float)F_CPU / PRESCALER_VALUE;  // 2,000,000 Hz
const float TICK_PERIOD_US = 1e6f / TICK_FREQ;           // 0.5 µs per tick

const int LED_PIN = 13;

// --- VOLATILE: Shared between ISR and main loop ---
volatile uint16_t captureValue = 0;   // ICR1 snapshot at capture moment
volatile uint16_t lastCapture = 0;    // Previous capture value
volatile uint16_t overflowCount = 0;  // Timer1 overflow count between captures
volatile bool newCapture = false;     // Flag: new RPM period available

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  // -------- CONFIGURE TIMER1 FOR INPUT CAPTURE --------
  // TCCR1A: No PWM output, no OC mode
  TCCR1A = 0x00;

  // TCCR1B:
  //   ICNC1 = 1 (Input Capture Noise Canceller — requires 4 equal samples)
  //   ICES1 = 0 (capture on FALLING edge — Hall sensor active-LOW)
  //   WGM13:WGM12 = 00 (Normal mode, no TOP)
  //   CS12:CS10 = 010 (Prescaler /8 → 2 MHz ticks)
  TCCR1B = _BV(ICNC1) | _BV(CS11);  // ICNC1 + /8 prescaler

  TCNT1 = 0;  // Reset counter

  // Enable Input Capture Interrupt (ICIE1) and Overflow Interrupt (TOIE1)
  TIMSK1 = _BV(ICIE1) | _BV(TOIE1);

  // Configure ICP1 (Pin 8) as input with pull-up
  pinMode(8, INPUT_PULLUP);

  sei();  // Enable global interrupts

  Serial.println(F("[ICU Tacho] Timer1 Input Capture RPM Meter initialized."));
  Serial.print(F("[ICU Tacho] Tick frequency: "));
  Serial.print(TICK_FREQ / 1000.0f, 1);
  Serial.print(F(" kHz | Period: "));
  Serial.print(TICK_PERIOD_US, 1);
  Serial.println(F(" µs/tick"));
  Serial.print(F("[ICU Tacho] Magnets per revolution: "));
  Serial.println(MAGNETS_PER_REV);
  Serial.println(F("[ICU Tacho] Waiting for rotation..."));
}

void loop() {
  if (newCapture) {
    newCapture = false;

    // Atomically capture values (disable interrupts briefly)
    noInterrupts();
    uint16_t cap = captureValue;
    uint16_t last = lastCapture;
    uint16_t oflows = overflowCount;
    interrupts();

    // Compute period in Timer1 ticks (account for overflows)
    uint32_t periodTicks = ((uint32_t)oflows << 16) + (uint32_t)cap - (uint32_t)last;
    // If cap < last (single overflow, no overflow counter incremented yet), correct:
    if (cap < last && oflows == 0) periodTicks += 65536UL;

    float periodSec = (float)periodTicks / TICK_FREQ;
    float freqHz = 1.0f / periodSec;
    float rpm = freqHz * 60.0f / MAGNETS_PER_REV;
    float periodMs = periodSec * 1000.0f;

    digitalWrite(LED_PIN, HIGH);
    Serial.print(F("[TACHO] RPM: "));
    Serial.print(rpm, 1);
    Serial.print(F(" | Freq: "));
    Serial.print(freqHz, 2);
    Serial.print(F(" Hz"));
    Serial.print(F(" | Period: "));
    Serial.print(periodMs, 1);
    Serial.print(F(" ms"));
    Serial.print(F(" | Ticks: "));
    Serial.println(periodTicks);
    digitalWrite(LED_PIN, LOW);
  }

  // Stale timeout: if no pulse for >2 seconds, print "STOPPED"
  static unsigned long lastPulseTime = 0;
  if (newCapture) lastPulseTime = millis();
  if (millis() - lastPulseTime > 2000 && lastPulseTime > 0) {
    static bool stoppedPrinted = false;
    if (!stoppedPrinted) {
      Serial.println(F("[TACHO] Motor STOPPED or RPM < 1"));
      stoppedPrinted = true;
      lastPulseTime = 0;
    }
  } else {
    static bool stoppedPrinted = false;
    stoppedPrinted = false;  // Reset when pulses resume
  }
}

// =============================================================
//  TIMER1 INPUT CAPTURE ISR
// =============================================================
ISR(TIMER1_CAPT_vect) {
  uint16_t captured = ICR1;  // Read capture register (hardware latched at edge moment)

  lastCapture = captureValue;  // Shift current → last
  captureValue = captured;
  overflowCount = 0;  // Reset overflow counter for this period
  newCapture = true;
}

// =============================================================
//  TIMER1 OVERFLOW ISR (counts rollovers for slow RPM)
// =============================================================
ISR(TIMER1_OVF_vect) {
  if (!newCapture) {  // Only count overflows between captures
    overflowCount++;
  }
}

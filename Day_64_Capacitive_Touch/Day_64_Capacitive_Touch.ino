/*
 * 100 Projects with Arduino - Day 64
 * Project: Capacitive Touch Sensor (RC Charge-Time Measurement — No Library)
 *
 * DESCRIPTION:
 * This project implements a software capacitive touch sensor by measuring the
 * time it takes to charge a capacitor formed by the human body and a sensing
 * electrode through a known resistor. No dedicated touch IC is needed — just
 * two Arduino pins and a high-value resistor.
 *
 * WORKING PRINCIPLE — RC CHARGE TIME MEASUREMENT:
 * The sensing circuit is an RC network where:
 *   R = Series resistor (1 MΩ to 10 MΩ)
 *   C = Stray capacitance of wire/pad + body capacitance (~10–50 pF baseline,
 *       +100–300 pF when touched)
 *
 * Time to charge from 0V to VIH (digital HIGH threshold ≈ 2.5V on 5V Arduino):
 *   t = -R * C * ln(1 - VIH/VCC)
 *   For VIH/VCC ≈ 0.5 (2.5V/5V):
 *   t ≈ R * C * 0.693   (≈ one time constant when threshold = 63% of VCC)
 *
 * MEASUREMENT TECHNIQUE (Arduino CapacitiveSensor method):
 *   1. Set SEND_PIN LOW and SENSE_PIN as INPUT (discharge stray capacitance)
 *   2. Set SEND_PIN HIGH — this drives current into the RC circuit
 *   3. Count loop iterations until SENSE_PIN reads HIGH (busy-loop counter)
 *   4. The counter value is proportional to the RC charge time
 *   5. Average N samples to reduce noise
 *
 * TOUCH DETECTION:
 *   - Baseline: measure charge count with no touch
 *   - Delta: charge count increases significantly when body capacitance adds to C
 *   - Threshold: if (reading - baseline) > TOUCH_THRESHOLD → touch detected
 *
 * WIRING:
 *   SEND_PIN (D4)  ─── 1 MΩ resistor ─── SENSE_PIN (D5)
 *   SENSE_PIN (D5) ─── sensing electrode (foil pad, wire, conductive tape)
 *   (Optional: 47pF cap between SENSE_PIN and GND for noise filtering)
 */

// --- PINS ---
const int SEND_PIN  = 4;  // Drives the RC network charge
const int SENSE_PIN = 5;  // Reads the RC network charge state
const int LED_PIN   = 13; // Touch indicator LED

// --- PARAMETERS ---
const int  SAMPLES          = 30;    // Number of ADC samples to average per reading
const long TOUCH_THRESHOLD  = 150;   // Delta above baseline to count as touch
const int  MAX_CYCLES       = 5000;  // Max loop iterations (timeout guard)
const int  CALIBRATE_READS  = 50;    // Samples taken during auto-calibration

// --- GLOBALS ---
long baseline  = 0;
bool touched   = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SEND_PIN, OUTPUT);
  
  Serial.println(F("[CapTouch] Calibrating baseline — do NOT touch the sensor..."));
  delay(1000);

  // Auto-calibrate: measure baseline with no touch
  long calSum = 0;
  for (int i = 0; i < CALIBRATE_READS; i++) {
    calSum += measureCharge();
    delay(10);
  }
  baseline = calSum / CALIBRATE_READS;

  Serial.print(F("[CapTouch] Baseline established: ")); Serial.println(baseline);
  Serial.print(F("[CapTouch] Touch threshold delta: ")); Serial.println(TOUCH_THRESHOLD);
  Serial.println(F("[CapTouch] Ready — touch the sensor electrode!"));
}

void loop() {
  long reading = measureCharge();
  long delta   = reading - baseline;

  bool isTouched = (delta > TOUCH_THRESHOLD);

  // Detect state change (avoid repeating same message)
  if (isTouched != touched) {
    touched = isTouched;
    if (touched) {
      Serial.println(F("[CapTouch] *** TOUCH DETECTED ***"));
    } else {
      Serial.println(F("[CapTouch] --- Released ---"));
    }
  }

  digitalWrite(LED_PIN, touched ? HIGH : LOW);

  // Periodic raw data log
  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 200) {
    lastLog = millis();
    Serial.print(F("[CapTouch] Reading: ")); Serial.print(reading);
    Serial.print(F(" | Delta: ")); Serial.print(delta);
    Serial.print(F(" | Touched: ")); Serial.println(isTouched ? F("YES") : F("NO"));
  }
}

// =============================================================
//  CORE MEASUREMENT: RC CHARGE TIME (LOOP COUNTER METHOD)
// =============================================================
long measureCharge() {
  long total = 0;

  for (int s = 0; s < SAMPLES; s++) {
    // DISCHARGE phase: drive both pins LOW to fully discharge stray C
    pinMode(SENSE_PIN, OUTPUT);
    digitalWrite(SENSE_PIN, LOW);
    digitalWrite(SEND_PIN, LOW);
    delayMicroseconds(10);

    // CHARGE phase: set SENSE_PIN as input, drive SEND_PIN HIGH
    pinMode(SENSE_PIN, INPUT);
    digitalWrite(SEND_PIN, HIGH);

    // COUNT loop cycles until SENSE_PIN goes HIGH
    int count = 0;
    while (digitalRead(SENSE_PIN) == LOW && count < MAX_CYCLES) {
      count++;
    }
    total += count;

    // Reset: pull SENSE_PIN LOW to discharge
    pinMode(SENSE_PIN, OUTPUT);
    digitalWrite(SENSE_PIN, LOW);
  }

  return total / SAMPLES;
}

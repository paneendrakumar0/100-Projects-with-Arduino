/*
 * 100 Projects with Arduino - Day 65
 * Project: EEPROM Servo Motion Recorder & Playback (Non-Volatile Motion Capture)
 *
 * DESCRIPTION:
 * This project turns the Arduino into a servo motion recorder. A potentiometer
 * controls a servo in real time. While the RECORD button is held, every servo
 * position is sampled at a fixed rate and saved to the ATmega328P's internal
 * EEPROM. When the PLAY button is pressed, the exact same motion is replayed
 * at the same speed — persistently, even after power off.
 *
 * EEPROM LAYOUT:
 *   Byte 0:       Frame count (how many positions recorded, max 253)
 *   Bytes 1–253:  Servo positions (0–180 degrees), one byte per frame
 *   Byte 254:     Magic sentinel byte (0xA5) — confirms valid recording exists
 *
 * EEPROM FACTS (ATmega328P):
 *   Total EEPROM: 1024 bytes
 *   Endurance:    100,000 write/erase cycles per cell (rated)
 *   Retention:    20+ years at 25°C
 *   Write time:   ~3.3 ms per byte (handled internally by avr/eeprom.h)
 *   Read time:    ~0 ms (synchronous read from NVM array)
 *
 * WEAR-LEVELING NOTE:
 *   We always write to the same addresses (no wear-leveling). With max 100
 *   recordings per day, at 253 bytes per recording, the EEPROM would last
 *   100,000 / 100 = 1000 days ≈ 2.7 years. For production use, implement a
 *   rotating page-based write pointer.
 *
 * SAMPLE RATE: One servo position stored per RECORD_INTERVAL_MS (default 50ms)
 *   → 50ms per frame = 20 Hz recording rate
 *   → 253 frames max = 12.65 seconds of motion per recording
 *
 * WIRING:
 *   Potentiometer Wiper  -> A0
 *   Servo Signal         -> D9
 *   RECORD Button        -> D2 (to GND, INPUT_PULLUP)
 *   PLAY Button          -> D3 (to GND, INPUT_PULLUP)
 *   STATUS LED (record)  -> D12 (HIGH during recording)
 *   STATUS LED (play)    -> D11 (HIGH during playback)
 */

#include <Servo.h>
#include <EEPROM.h>

// --- PINS ---
const int POT_PIN         = A0;
const int SERVO_PIN       = 9;
const int RECORD_BTN_PIN  = 2;
const int PLAY_BTN_PIN    = 3;
const int REC_LED_PIN     = 12;
const int PLAY_LED_PIN    = 11;

// --- EEPROM LAYOUT CONSTANTS ---
const int  EEPROM_COUNT_ADDR   = 0;     // Byte 0: frame count
const int  EEPROM_DATA_ADDR    = 1;     // Bytes 1–253: servo angles
const int  EEPROM_MAGIC_ADDR   = 254;   // Byte 254: magic sentinel
const uint8_t EEPROM_MAGIC     = 0xA5;  // Valid recording marker
const int  MAX_FRAMES          = 253;   // Maximum recordable frames

// --- TIMING ---
const unsigned long RECORD_INTERVAL_MS = 50; // 20 Hz sample rate
const unsigned long DEBOUNCE_MS        = 30;

// --- SERVO ---
Servo myServo;

// --- STATE MACHINE ---
enum State { IDLE, RECORDING, PLAYING };
State state = IDLE;

void setup() {
  Serial.begin(9600);
  pinMode(RECORD_BTN_PIN, INPUT_PULLUP);
  pinMode(PLAY_BTN_PIN,   INPUT_PULLUP);
  pinMode(REC_LED_PIN,    OUTPUT);
  pinMode(PLAY_LED_PIN,   OUTPUT);
  myServo.attach(SERVO_PIN);
  myServo.write(90); // Center position

  // Check for existing valid recording
  if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC) {
    uint8_t frames = EEPROM.read(EEPROM_COUNT_ADDR);
    Serial.print(F("[EEPROM] Valid recording found: "));
    Serial.print(frames); Serial.println(F(" frames."));
  } else {
    Serial.println(F("[EEPROM] No previous recording. Record a motion first."));
  }

  Serial.println(F("[Motion] HOLD RECORD button to record | PRESS PLAY button to replay."));
}

void loop() {
  bool recBtn  = (digitalRead(RECORD_BTN_PIN) == LOW);
  bool playBtn = (digitalRead(PLAY_BTN_PIN) == LOW);

  // --- Read pot and drive servo in real time (unless playing back) ---
  if (state != PLAYING) {
    int potVal   = analogRead(POT_PIN);
    int servoPos = map(potVal, 0, 1023, 0, 180);
    myServo.write(servoPos);

    // --- RECORDING ---
    if (recBtn) {
      if (state != RECORDING) {
        startRecording();
      }
      recordFrame();
    } else {
      if (state == RECORDING) {
        stopRecording();
      }
    }
  }

  // --- PLAYBACK ---
  if (playBtn && state == IDLE) {
    playRecording();
  }
}

// =============================================================
//  RECORDING LOGIC
// =============================================================
uint8_t frameCount = 0;
unsigned long lastRecordTime = 0;

void startRecording() {
  state      = RECORDING;
  frameCount = 0;
  lastRecordTime = millis();
  digitalWrite(REC_LED_PIN, HIGH);
  Serial.println(F("[REC] Recording started..."));
}

void recordFrame() {
  if (state != RECORDING) return;
  if (millis() - lastRecordTime < RECORD_INTERVAL_MS) return;
  if (frameCount >= MAX_FRAMES) {
    stopRecording();
    return;
  }

  int potVal    = analogRead(POT_PIN);
  uint8_t angle = (uint8_t)map(potVal, 0, 1023, 0, 180);

  EEPROM.write(EEPROM_DATA_ADDR + frameCount, angle);
  frameCount++;
  lastRecordTime = millis();

  // Show progress every 10 frames
  if (frameCount % 10 == 0) {
    Serial.print(F("[REC] Frame ")); Serial.print(frameCount);
    Serial.print(F(" | Angle: ")); Serial.println(angle);
  }
}

void stopRecording() {
  state = IDLE;
  // Save frame count and magic sentinel
  EEPROM.write(EEPROM_COUNT_ADDR, frameCount);
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  digitalWrite(REC_LED_PIN, LOW);
  Serial.print(F("[REC] Recording stopped. Total frames: "));
  Serial.print(frameCount);
  Serial.print(F(" ("));
  Serial.print((float)frameCount * RECORD_INTERVAL_MS / 1000.0f, 2);
  Serial.println(F(" seconds saved to EEPROM)"));
}

// =============================================================
//  PLAYBACK LOGIC
// =============================================================
void playRecording() {
  // Validate recording
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    Serial.println(F("[PLAY] No valid recording found. Record first."));
    return;
  }

  uint8_t frames = EEPROM.read(EEPROM_COUNT_ADDR);
  if (frames == 0) {
    Serial.println(F("[PLAY] Recording has 0 frames. Nothing to play."));
    return;
  }

  state = PLAYING;
  digitalWrite(PLAY_LED_PIN, HIGH);
  Serial.print(F("[PLAY] Playing back ")); Serial.print(frames); Serial.println(F(" frames..."));

  for (uint8_t i = 0; i < frames; i++) {
    uint8_t angle = EEPROM.read(EEPROM_DATA_ADDR + i);
    myServo.write(angle);
    delay(RECORD_INTERVAL_MS); // Replay at the same rate as recording

    if (i % 10 == 0) {
      Serial.print(F("[PLAY] Frame ")); Serial.print(i);
      Serial.print(F(" | Angle: ")); Serial.println(angle);
    }
  }

  digitalWrite(PLAY_LED_PIN, LOW);
  state = IDLE;
  Serial.println(F("[PLAY] Playback complete."));
}

/*
 * 100 Projects with Arduino - Day 63
 * Project: DS18B20 1-Wire Temperature Sensor (1-Wire Protocol from Scratch)
 *
 * DESCRIPTION:
 * This project implements the complete Dallas/Maxim 1-Wire protocol entirely in
 * software (bit-banging), without any library. The DS18B20 digital thermometer
 * communicates over a single data wire using strict timing sequences for:
 *   - Reset Pulse & Presence Detect
 *   - ROM Command: Skip ROM (0xCC) — broadcast to all devices
 *   - Function Command: Convert T (0x44) — start temperature conversion
 *   - Function Command: Read Scratchpad (0xBE) — read 9 bytes including temperature
 *   - CRC-8 (Dallas/Maxim polynomial 0x31) verification of scratchpad data
 *
 * 1-WIRE PROTOCOL TIMING (Standard Speed):
 *   Reset Pulse:       Pull LOW for >= 480 us
 *   Presence Detect:   DS18B20 pulls LOW for 60-240 us after release
 *   Write 1 Bit:       Pull LOW 1-15 us, release, total slot >= 60 us
 *   Write 0 Bit:       Pull LOW for entire slot >= 60 us
 *   Read Bit:          Pull LOW 1-15 us, release, sample within 15 us of start
 *
 * TEMPERATURE REGISTER FORMAT (DS18B20 Scratchpad Bytes 0-1):
 *   Byte 0 = Temperature LSB
 *   Byte 1 = Temperature MSB
 *   Raw 16-bit signed integer, default resolution 12-bit (0.0625°C per count)
 *   Temperature (°C) = raw16bit_signed / 16.0
 *
 * CRC-8 (MAXIM/DALLAS):
 *   Polynomial: x^8 + x^5 + x^4 + 1 = 0x31
 *   Input: first 8 bytes of scratchpad
 *   Expected: equals byte 8 (CRC byte) for valid data
 *
 * WIRING:
 *   DS18B20 VDD  -> 5V (or parasitic power from DQ — see README)
 *   DS18B20 GND  -> GND
 *   DS18B20 DQ   -> D2 (Data pin, with 4.7k pull-up to 5V)
 *   4.7k resistor between DQ and 5V (required!)
 */

// --- 1-WIRE DATA PIN ---
const int OW_PIN = 2;

void setup() {
  Serial.begin(9600);
  Serial.println(F("[1-Wire] DS18B20 Temperature Logger initialized."));
  Serial.println(F("[1-Wire] Sampling every 1 second. CRC verified."));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  float tempC = readDS18B20();

  if (tempC == -999.0f) {
    Serial.println(F("[ERROR] No device found or CRC mismatch!"));
  } else {
    float tempF = tempC * 9.0f / 5.0f + 32.0f;
    Serial.print(F("[Temp] "));
    Serial.print(tempC, 4); Serial.print(F(" °C  |  "));
    Serial.print(tempF, 2); Serial.println(F(" °F"));
  }

  delay(1000);
}

// =============================================================
//  1-WIRE LOW-LEVEL BIT-BANG FUNCTIONS
// =============================================================

// Drive bus LOW (pull-down)
void ow_low() {
  pinMode(OW_PIN, OUTPUT);
  digitalWrite(OW_PIN, LOW);
}

// Release bus (let pull-up pull HIGH)
void ow_release() {
  pinMode(OW_PIN, INPUT); // Hi-Z, external 4.7k pulls HIGH
}

// Read current bus state
uint8_t ow_read_bit_raw() {
  return digitalRead(OW_PIN);
}

// Reset pulse — returns true if presence detected
bool ow_reset() {
  ow_low();
  delayMicroseconds(480);   // Hold LOW >= 480 us
  ow_release();
  delayMicroseconds(70);    // Wait for presence pulse start (60 us typical)
  bool presence = (ow_read_bit_raw() == LOW); // DS18B20 pulls LOW
  delayMicroseconds(410);   // Complete reset window (total >= 480 us from release)
  return presence;
}

// Write a single bit
void ow_write_bit(uint8_t bit) {
  ow_low();
  if (bit) {
    delayMicroseconds(6);   // Short pull for '1': < 15 us
    ow_release();
    delayMicroseconds(64);  // Remainder of 70 us slot
  } else {
    delayMicroseconds(70);  // Hold LOW entire slot for '0'
    ow_release();
    delayMicroseconds(10);
  }
}

// Read a single bit
uint8_t ow_read_bit() {
  ow_low();
  delayMicroseconds(6);     // Initiate read slot
  ow_release();
  delayMicroseconds(9);     // Wait to sample (must be within 15 us of start)
  uint8_t bit = ow_read_bit_raw();
  delayMicroseconds(55);    // Complete 70 us slot
  return bit;
}

// Write one byte (LSB first)
void ow_write_byte(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    ow_write_bit(data & 0x01);
    data >>= 1;
  }
}

// Read one byte (LSB first)
uint8_t ow_read_byte() {
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    data |= (ow_read_bit() << i);
  }
  return data;
}

// =============================================================
//  CRC-8 (DALLAS/MAXIM, POLYNOMIAL 0x31)
// =============================================================
uint8_t crc8(uint8_t* buf, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t byte = buf[i];
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ byte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C; // Reflected 0x31
      byte >>= 1;
    }
  }
  return crc;
}

// =============================================================
//  HIGH-LEVEL: READ DS18B20 TEMPERATURE
// =============================================================
float readDS18B20() {
  // Step 1: Reset + presence check
  if (!ow_reset()) return -999.0f;

  // Step 2: Skip ROM (address all devices)
  ow_write_byte(0xCC);

  // Step 3: Start temperature conversion
  ow_write_byte(0x44);

  // Step 4: Wait for conversion (750 ms max for 12-bit)
  delay(750);

  // Step 5: Reset again, skip ROM, read scratchpad
  if (!ow_reset()) return -999.0f;
  ow_write_byte(0xCC);
  ow_write_byte(0xBE); // Read Scratchpad command

  // Step 6: Read 9 bytes of scratchpad
  uint8_t scratchpad[9];
  for (uint8_t i = 0; i < 9; i++) {
    scratchpad[i] = ow_read_byte();
  }

  // Step 7: Verify CRC
  if (crc8(scratchpad, 8) != scratchpad[8]) {
    return -999.0f; // CRC mismatch
  }

  // Step 8: Convert raw to temperature
  int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
  return (float)raw / 16.0f; // 12-bit resolution: 0.0625°C per LSB
}

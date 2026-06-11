/*
 * 100 Projects with Arduino - Day 69
 * Project: DHT22 Humidity & Temperature Sensor (Custom Single-Wire Bit-Bang Parser)
 *
 * DESCRIPTION:
 * This project implements the complete DHT22 (AM2302) communication protocol
 * from scratch — no library. The DHT22 uses a single-wire, half-duplex protocol
 * where the Arduino acts as master and the sensor responds with 40 bits of data:
 *   - 16 bits: Relative Humidity (RH%) x10 (big-endian, unsigned)
 *   - 16 bits: Temperature (°C) x10 (big-endian, bit15=sign)
 *   - 8 bits:  Checksum (sum of 4 data bytes, lower 8 bits)
 *
 * DHT22 PROTOCOL TIMING:
 *   1. HOST START: Pull data line LOW for ≥ 1 ms (we use 2 ms)
 *   2. HOST RELEASE: Let pull-up restore HIGH, wait 20-40 µs
 *   3. SENSOR RESPONSE: Sensor pulls LOW for 80 µs, then HIGH for 80 µs
 *   4. DATA BITS (×40): For each bit:
 *      - 50 µs LOW (bit start signal from sensor)
 *      - 26-28 µs HIGH = logical '0'
 *      - 70 µs HIGH    = logical '1'
 *   We distinguish '0' vs '1' by measuring HIGH pulse width:
 *     HIGH duration > 40 µs → bit = 1
 *     HIGH duration < 40 µs → bit = 0
 *
 * DATA DECODING:
 *   Byte 0: Humidity MSB
 *   Byte 1: Humidity LSB  → RH% = (Byte0<<8 | Byte1) / 10.0
 *   Byte 2: Temperature MSB (bit15 = sign for negative temps)
 *   Byte 3: Temperature LSB  → T°C = (Byte2<<8 | Byte3 after masking sign) / 10.0
 *   Byte 4: Checksum = (Byte0 + Byte1 + Byte2 + Byte3) & 0xFF
 *
 * HEAT INDEX (Feels Like):
 *   Uses the Steadman/Rothfusz regression equation (National Weather Service):
 *   HI = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH ...
 *   (simplified version used for moderate conditions)
 *
 * WIRING:
 *   DHT22 Pin 1 (VCC) -> 3.3V or 5V
 *   DHT22 Pin 2 (DATA)-> D2 (with 10kΩ pull-up to VCC)
 *   DHT22 Pin 3 (NC)  -> Not connected
 *   DHT22 Pin 4 (GND) -> GND
 *   10kΩ resistor between Pin 2 and VCC is REQUIRED
 */

// --- DATA PIN ---
const int DHT_PIN = 2;

// --- TIMING CONSTANTS (µs) ---
const int START_LOW_MS = 2;       // Host start: pull LOW this many ms
const int BIT_THRESHOLD_US = 40;  // HIGH > 40µs → '1', else → '0'
const int MAX_WAIT_US = 200;      // Timeout for sensor response

void setup() {
  Serial.begin(9600);
  Serial.println(F("[DHT22] Custom Protocol Parser initialized."));
  Serial.println(F("[DHT22] Sampling every 2.5 seconds (sensor min interval)."));
  delay(2000);  // DHT22 requires 2 seconds after power-on before first read
}

void loop() {
  float humidity, tempC, heatIndex;
  bool ok = readDHT22(humidity, tempC, heatIndex);

  if (ok) {
    float tempF = tempC * 9.0f / 5.0f + 32.0f;
    float dewPoint = calculateDewPoint(tempC, humidity);

    Serial.print(F("[DHT22] Temp: "));
    Serial.print(tempC, 1);
    Serial.print(F("°C / "));
    Serial.print(tempF, 1);
    Serial.print(F("°F"));
    Serial.print(F(" | RH: "));
    Serial.print(humidity, 1);
    Serial.print(F("%"));
    Serial.print(F(" | HeatIdx: "));
    Serial.print(heatIndex, 1);
    Serial.print(F("°C"));
    Serial.print(F(" | DewPt: "));
    Serial.print(dewPoint, 1);
    Serial.println(F("°C"));
  } else {
    Serial.println(F("[DHT22] ERROR: Read failed (timeout or checksum mismatch)"));
  }

  delay(2500);  // DHT22 minimum 2.5s between reads
}

// =============================================================
//  CORE DHT22 SINGLE-WIRE PROTOCOL READER
// =============================================================
bool readDHT22(float& humidity, float& tempC, float& heatIndex) {
  uint8_t data[5] = {0, 0, 0, 0, 0};

  // --- STEP 1: HOST START SIGNAL ---
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(START_LOW_MS);  // Pull LOW for 2 ms
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(30);    // Wait 20-40 µs
  pinMode(DHT_PIN, INPUT);  // Release bus

  // --- STEP 2: WAIT FOR SENSOR RESPONSE ---
  // Sensor should pull LOW for 80 µs
  unsigned long t = micros();
  while (digitalRead(DHT_PIN) == HIGH) {
    if (micros() - t > MAX_WAIT_US) return false;  // Timeout — no sensor
  }
  // Sensor pulls LOW (80 µs)
  t = micros();
  while (digitalRead(DHT_PIN) == LOW) {
    if (micros() - t > MAX_WAIT_US) return false;
  }
  // Sensor pulls HIGH (80 µs — ready to send data)
  t = micros();
  while (digitalRead(DHT_PIN) == HIGH) {
    if (micros() - t > MAX_WAIT_US) return false;
  }

  // --- STEP 3: READ 40 DATA BITS ---
  for (int i = 0; i < 40; i++) {
    // Wait for the 50 µs LOW bit-start from sensor
    t = micros();
    while (digitalRead(DHT_PIN) == LOW) {
      if (micros() - t > MAX_WAIT_US) return false;
    }
    // Measure HIGH pulse duration
    t = micros();
    while (digitalRead(DHT_PIN) == HIGH) {
      if (micros() - t > MAX_WAIT_US) return false;
    }
    unsigned long pulseWidth = micros() - t;

    // Determine bit value: >40µs → 1, else → 0
    uint8_t bit = (pulseWidth > BIT_THRESHOLD_US) ? 1 : 0;
    data[i / 8] = (data[i / 8] << 1) | bit;
  }

  // --- STEP 4: VERIFY CHECKSUM ---
  uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
  if (checksum != data[4]) return false;

  // --- STEP 5: DECODE VALUES ---
  // Humidity: always positive
  humidity = (float)((uint16_t)(data[0] << 8) | data[1]) / 10.0f;

  // Temperature: bit15 of byte2 is sign bit
  uint16_t rawTemp = ((uint16_t)(data[2] & 0x7F) << 8) | data[3];
  tempC = (float)rawTemp / 10.0f;
  if (data[2] & 0x80) tempC = -tempC;  // Negative temperature

  // --- STEP 6: HEAT INDEX (simplified Steadman equation) ---
  heatIndex = -8.78469475556f + 1.61139411f * tempC + 2.338548839f * humidity -
              0.14611605f * tempC * humidity - 0.012308094f * tempC * tempC -
              0.016424828f * humidity * humidity + 0.002211732f * tempC * tempC * humidity +
              0.00072546f * tempC * humidity * humidity -
              0.000003582f * tempC * tempC * humidity * humidity;

  return true;
}

// =============================================================
//  DEW POINT (Magnus Formula)
// =============================================================
float calculateDewPoint(float tempC, float rh) {
  // Magnus approximation constants (valid -40°C to +60°C):
  // a = 17.625, b = 243.04°C
  const float a = 17.625f, b = 243.04f;
  float alpha = log(rh / 100.0f) + (a * tempC) / (b + tempC);
  return (b * alpha) / (a - alpha);
}

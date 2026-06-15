/*
 * 100 Projects with Arduino - Day 49
 * Project: GPS Telemetry Decoder (Raw NMEA Sentence Parser & Checksum Verifier)
 *
 * DESCRIPTION:
 * This project implements a high-performance, non-blocking GPS parser that decodes raw NMEA-0183
 * protocol streams directly from a NEO-6M GPS receiver without any third-party libraries (e.g.
 * TinyGPS). To demonstrate advanced firmware engineering standards:
 * 1. Byte-Level State Machine Parser: Reads characters from SoftwareSerial non-blockingly,
 * reassembles individual sentences, and extracts fields dynamically using pointer index indexing.
 * 2. Hardware-Level Checksum Verification: Computes the XOR checksum of every sentence and verifies
 *    it against the received packet trailer hex byte, throwing away corrupted messages.
 * 3. NMEA Parsing Engine: Decodes:
 *    - $GPRMC: UTC time, Fix Status (A=Active, V=Void), Latitude, Longitude, Speed (knots), and
 * Date.
 *    - $GPGGA: Altitude (meters) and Number of Connected Satellites.
 * 4. Coordinate Conversion: Converts raw NMEA angular coordinates (DDMM.MMMM / DDDMM.MMMM)
 *    into standard decimal degrees (DD.DDDDD) with proper directional sign (+/-) representation.
 *
 * NMEA COORDINATE CONVERSION MATHEMATICS:
 * - Raw Latitude String: "1302.3456" -> 13 degrees, 02.3456 minutes.
 *   Decimal Latitude = 13 + (02.3456 / 60.0) = 13.039093°
 *   If direction is 'S' (South), multiply by -1.0.
 * - Raw Longitude String: "08015.6789" -> 80 degrees, 15.6789 minutes.
 *   Decimal Longitude = 80 + (15.6789 / 60.0) = 80.261315°
 *   If direction is 'W' (West), multiply by -1.0.
 *
 * WIRING:
 * - NEO-6M GPS Module -> Arduino Uno
 *   - VCC -> 5V (or 3.3V depending on module)
 *   - GND -> GND
 *   - TXD -> Pin 9 (Arduino SoftwareSerial RX)
 *   - RXD -> Pin 10 (Arduino SoftwareSerial TX via 1k/2k Ohm voltage divider to step 5V down
 * to 3.3V)
 */

#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const int GPS_RX_PIN = 9;
const int GPS_TX_PIN = 10;
const int LED_INDICATOR_PIN = 13;

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// --- PARSER LIMITS ---
const int MAX_SENTENCE_LEN = 90;
char sentenceBuffer[MAX_SENTENCE_LEN];
int bufferIndex = 0;
bool isCapturing = false;

// --- TELEMETRY CONTAINERS ---
char utcTime[10] = "00:00:00";
char fixStatus = 'V';  // 'A' = Active (3D Fix), 'V' = Void (No Fix)
float decimalLatitude = 0.0;
float decimalLongitude = 0.0;
float speedKnots = 0.0;
char dateString[7] = "000000";
int satelliteCount = 0;
float altitudeMeters = 0.0;

// Update tracking to throttle Serial Monitor prints
unsigned long lastDisplayTime = 0;
const unsigned long displayIntervalMs = 1500;  // Print telemetry summary every 1.5s

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);  // NEO-6M defaults to 9600 baud

  pinMode(LED_INDICATOR_PIN, OUTPUT);
  digitalWrite(LED_INDICATOR_PIN, LOW);

  Serial.println("[GPS] Starting raw NMEA telemetry decoder...");
  Serial.println("[GPS] Waiting for satellites 3D lock (LED blinks on active sentence parser)...");
}

void loop() {
  // Read raw character stream from GPS module non-blockingly
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    parseRawByte(c);
  }

  // Print formatted telemetry on a timed interval
  if (millis() - lastDisplayTime >= displayIntervalMs) {
    lastDisplayTime = millis();
    displayTelemetry();
  }
}

// --- BYTE-LEVEL NMEA PARSING STATE MACHINE ---

void parseRawByte(char c) {
  if (c == '$') {
    bufferIndex = 0;
    isCapturing = true;
    sentenceBuffer[bufferIndex] = '\0';
  } else if (isCapturing) {
    if (c == '\r' || c == '\n') {
      sentenceBuffer[bufferIndex] = '\0';
      isCapturing = false;

      // We have a complete sentence! Let's process it
      processCompleteSentence();
    } else if (bufferIndex < MAX_SENTENCE_LEN - 1) {
      sentenceBuffer[bufferIndex] = c;
      bufferIndex++;
    } else {
      // Buffer overflow protection: reset parser
      isCapturing = false;
      bufferIndex = 0;
    }
  }
}

// --- CHECKSUM VERIFICATION & SENTENCE IDENTIFICATION ---

void processCompleteSentence() {
  // A standard sentence looks like: GPRMC,225446,A,4916.45,N,12311.12,W...*68
  // Check for presence of checksum asterisk
  int asteriskIndex = -1;
  for (int i = 0; i < bufferIndex; i++) {
    if (sentenceBuffer[i] == '*') {
      asteriskIndex = i;
      break;
    }
  }

  if (asteriskIndex == -1) return;  // Invalid structure, throw out

  // Extract expected checksum hex code
  char hexChecksum[3];
  hexChecksum[0] = sentenceBuffer[asteriskIndex + 1];
  hexChecksum[1] = sentenceBuffer[asteriskIndex + 2];
  hexChecksum[2] = '\0';
  long expectedChecksum = strtol(hexChecksum, NULL, 16);

  // Compute actual XOR checksum
  long calculatedChecksum = 0;
  for (int i = 0; i < asteriskIndex; i++) {
    calculatedChecksum ^= sentenceBuffer[i];
  }

  // Verify integrity
  if (calculatedChecksum != expectedChecksum) {
    Serial.println("[GPS] WARNING: Checksum mismatch! Corrupted sentence discarded.");
    return;
  }

  // Flash the indicator LED briefly to show a valid frame was parsed
  digitalWrite(LED_INDICATOR_PIN, HIGH);
  delayMicroseconds(500);
  digitalWrite(LED_INDICATOR_PIN, LOW);

  // Null-terminate the string at the asterisk to isolate the data payload
  sentenceBuffer[asteriskIndex] = '\0';

  // Check identifier (e.g. "GPRMC" or "GNRMC" / "GPGGA" or "GNGGA")
  if (strstr(sentenceBuffer, "RMC") != NULL) {
    parseGPRMC();
  } else if (strstr(sentenceBuffer, "GGA") != NULL) {
    parseGPGGA();
  }
}

// --- COMMA-SEPARATED FIELD EXTRACTOR ---

// Helper function to extract a specific field (0-indexed) into output buffer
bool getCommaField(const char *sentence, int fieldIndex, char *output, int maxLen) {
  int commaCount = 0;
  int startIdx = -1;
  int endIdx = -1;
  int len = strlen(sentence);

  for (int i = 0; i < len; i++) {
    if (sentence[i] == ',') {
      commaCount++;
      if (commaCount == fieldIndex) {
        startIdx = i + 1;
      } else if (commaCount == fieldIndex + 1) {
        endIdx = i;
        break;
      }
    }
  }

  // Handle trailing fields which have no final comma
  if (startIdx != -1 && endIdx == -1) {
    endIdx = len;
  }

  if (startIdx != -1 && endIdx != -1 && (endIdx - startIdx) > 0) {
    int copyLen = endIdx - startIdx;
    if (copyLen > maxLen - 1) copyLen = maxLen - 1;
    strncpy(output, sentence + startIdx, copyLen);
    output[copyLen] = '\0';
    return true;
  }

  output[0] = '\0';
  return false;
}

// --- INDIVIDUAL PARSING LOOPS ---

void parseGPRMC() {
  char temp[20];

  // Field 1: UTC Time (HHMMSS.SS)
  if (getCommaField(sentenceBuffer, 1, temp, sizeof(temp))) {
    // Reformat into HH:MM:SS
    utcTime[0] = temp[0];
    utcTime[1] = temp[1];
    utcTime[2] = ':';
    utcTime[3] = temp[2];
    utcTime[4] = temp[3];
    utcTime[5] = ':';
    utcTime[6] = temp[4];
    utcTime[7] = temp[5];
    utcTime[8] = '\0';
  }

  // Field 2: Status (A = Active, V = Void)
  if (getCommaField(sentenceBuffer, 2, temp, sizeof(temp))) {
    fixStatus = temp[0];
  }

  if (fixStatus == 'A') {
    char rawLat[15], ns[2], rawLon[15], ew[2];

    // Field 3 & 4: Latitude & North/South Indicator
    if (getCommaField(sentenceBuffer, 3, rawLat, sizeof(rawLat)) &&
        getCommaField(sentenceBuffer, 4, ns, sizeof(ns))) {
      decimalLatitude = convertNMEAToDecimal(rawLat, ns[0]);
    }

    // Field 5 & 6: Longitude & East/West Indicator
    if (getCommaField(sentenceBuffer, 5, rawLon, sizeof(rawLon)) &&
        getCommaField(sentenceBuffer, 6, ew, sizeof(ew))) {
      decimalLongitude = convertNMEAToDecimal(rawLon, ew[0]);
    }

    // Field 7: Speed in knots
    if (getCommaField(sentenceBuffer, 7, temp, sizeof(temp))) {
      speedKnots = atof(temp);
    }

    // Field 9: Date (DDMMYY)
    if (getCommaField(sentenceBuffer, 9, dateString, sizeof(dateString))) {
      // Retained as raw DDMMYY representation
    }
  }
}

void parseGPGGA() {
  char temp[20];

  // We only parse GGA details if the GPS module has a valid 3D lock status
  if (fixStatus == 'A') {
    // Field 7: Satellites tracked
    if (getCommaField(sentenceBuffer, 7, temp, sizeof(temp))) {
      satelliteCount = atoi(temp);
    }

    // Field 9: Altitude above sea level
    if (getCommaField(sentenceBuffer, 9, temp, sizeof(temp))) {
      altitudeMeters = atof(temp);
    }
  }
}

// --- NMEA DDMM.MMMM TO DECIMAL DEGREES CONVERTER ---

float convertNMEAToDecimal(const char *rawCoord, char direction) {
  if (strlen(rawCoord) == 0) return 0.0;

  float rawVal = atof(rawCoord);
  int degrees = 0;
  float minutes = 0.0;

  if (direction == 'N' || direction == 'S') {
    // Latitude format: DDMM.MMMM
    degrees = (int)(rawVal / 100);
    minutes = rawVal - (degrees * 100);
  } else {
    // Longitude format: DDDMM.MMMM
    degrees = (int)(rawVal / 100);
    minutes = rawVal - (degrees * 100);
  }

  float decimal = (float)degrees + (minutes / 60.0);

  // Apply sign based on cardinal direction
  if (direction == 'S' || direction == 'W') {
    decimal = -decimal;
  }

  return decimal;
}

// --- TELEMETRY SUMMARY PRINTING ---

void displayTelemetry() {
  Serial.println(F("\n================ GPS TELEMETRY SUMMARY ================"));
  Serial.print(F("  UTC Time:        "));
  Serial.println(utcTime);
  Serial.print(F("  Date:            "));
  if (strcmp(dateString, "000000") != 0) {
    Serial.print(dateString[0]);
    Serial.print(dateString[1]);
    Serial.print(F("/"));
    Serial.print(dateString[2]);
    Serial.print(dateString[3]);
    Serial.print(F("/20"));
    Serial.print(dateString[4]);
    Serial.println(dateString[5]);
  } else {
    Serial.println(F("N/A"));
  }

  Serial.print(F("  Lock Status:     "));
  if (fixStatus == 'A') {
    Serial.println(F("3D FIX ACTIVE (Lock established)"));
  } else {
    Serial.println(F("NO FIX (Searching for satellites...)"));
  }

  if (fixStatus == 'A') {
    Serial.print(F("  Latitude:        "));
    Serial.print(decimalLatitude, 6);
    Serial.println(F("°"));
    Serial.print(F("  Longitude:       "));
    Serial.print(decimalLongitude, 6);
    Serial.println(F("°"));
    Serial.print(F("  Altitude:        "));
    Serial.print(altitudeMeters, 1);
    Serial.println(F(" m"));
    Serial.print(F("  Speed:           "));
    Serial.print(speedKnots * 1.852, 2);
    Serial.println(F(" km/h"));  // knots to km/h conversion
    Serial.print(F("  Satellites:      "));
    Serial.println(satelliteCount);
  } else {
    Serial.println(F("  Latitude/Lon:    Waiting for valid lock coordinates..."));
    Serial.println(F("  Altitude:        Waiting..."));
    Serial.println(F("  Speed:           Waiting..."));
  }
  Serial.println(F("======================================================="));
}

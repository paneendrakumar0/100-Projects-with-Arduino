/*
 * 100 Projects with Arduino - Day 78
 * Project: Raw NMEA GPS Sentence Parser (NEO-6M Software UART)
 * 
 * DESCRIPTION:
 * This project interfaces a NEO-6M GPS receiver module using SoftwareSerial and parses the
 * standard NMEA-0183 sentences from scratch — without using external libraries like TinyGPS.
 * To align with professional aerospace and navigation systems:
 * 1. NMEA Ring Buffer & State Machine: Scans the incoming serial byte stream, aligns with the
 *    '$' header character, and buffers complete strings until a carriage return '\n'.
 * 2. Sentence Classifier: Targets and decodes '$GPRMC' (Recommended Minimum Navigation) and
 *    '$GPGGA' (Fix Data) sentences.
 * 3. Coordinate Conversion: Parses raw degrees-minutes formatting (DDMM.MMMM) and translates
 *    them into floating-point Decimal Degrees (e.g. 12.99979°), adjusting signs for Hemisphere
 *    indicators (N/S, E/W).
 * 4. Simulation Shell: Allows users to copy-paste raw NMEA strings directly into the Serial
 *    Monitor to verify parser correctness without needing an active satellite lock or hardware.
 * 
 * NMEA FORMAT & COORDINATE PHYSICS:
 * - GPRMC Sentence:
 *   $GPRMC,123519.00,A,4807.03800,N,01131.00000,E,022.4,084.4,230394,,,A*6A
 *   Fields: [0]Header, [1]UTC Time, [2]Status (A=OK, V=Void), [3]Lat, [4]N/S, [5]Lon, [6]E/W...
 * - Coordinate Translation:
 *   Raw Latitude = 4807.03800. The first two digits are degrees (48°), the rest is minutes (07.038').
 *   $$\text{Decimal Degrees} = \text{Degrees} + \frac{\text{Minutes}}{60.0}$$
 *   $$\text{Dec} = 48 + \frac{7.038}{60.0} = 48.11730^\circ$$
 * 
 * WIRING:
 * - NEO-6M GPS Pin -> Arduino Uno Pin
 *   - VCC            -> 5V (or 3.3V)
 *   - GND            -> GND
 *   - TX             -> Pin 2 (Arduino Rx, SoftwareSerial)
 *   - RX             -> Pin 3 (Arduino Tx, SoftwareSerial - via 4.7k/10k divider to drop 5V to 3.3V)
 */

#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const int GPS_RX_PIN = 2; // Connects to GPS TX
const int GPS_TX_PIN = 3; // Connects to GPS RX

// Instantiate SoftwareSerial for GPS communication
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// --- BUFFER CONFIGURATION ---
const int BUFFER_SIZE = 100;
char sentenceBuffer[BUFFER_SIZE];
int bufferIndex = 0;
bool isRecording = false;

// --- GPS GLOBAL TELEMETRY ---
float latitude = 0.0f;
float longitude = 0.0f;
bool fixValid = false;
int satellitesTracked = 0;
String utcTime = "00:00:00";
String fixDate = "00-00-00";

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600); // NEO-6M default baud rate is 9600

  Serial.println(F("=================================================="));
  Serial.println(F("Day 78: Raw NMEA GPS Sentence Parser"));
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] Hardware Serial: 9600 Baud | Software Serial: 9600 Baud"));
  Serial.println(F("[SYSTEM] Listening for NEO-6M stream..."));
  
  printMenu();
}

void loop() {
  // 1. Read incoming stream from physical GPS module
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    processGPSChar(c);
  }

  // 2. Poll hardware serial for user command injections
  if (Serial.available() > 0) {
    String inputLine = Serial.readStringUntil('\n');
    inputLine.trim();
    
    if (inputLine.startsWith("$")) {
      // If user inputs a raw NMEA sentence, parse it
      Serial.print(F("[SHELL] Injecting sentence: "));
      Serial.println(inputLine);
      parseNMEASentence(inputLine);
      printGPSData();
    } else if (inputLine.equalsIgnoreCase("h")) {
      printMenu();
    }
  }
}

// =============================================================
//  NMEA BYTE-STREAM CHARACTER FILTERING
// =============================================================
void processGPSChar(char c) {
  if (c == '$') {
    // Start of a new NMEA sentence
    bufferIndex = 0;
    isRecording = true;
    sentenceBuffer[bufferIndex] = c;
    bufferIndex++;
  } 
  else if (isRecording) {
    if (c == '\r' || c == '\n') {
      // End of NMEA sentence
      sentenceBuffer[bufferIndex] = '\0';
      isRecording = false;
      
      // Parse the buffered sentence
      String sentence = String(sentenceBuffer);
      if (parseNMEASentence(sentence)) {
        // Output fresh telemetry when a targeted sentence parses successfully
        printGPSData();
      }
    } 
    else if (bufferIndex < BUFFER_SIZE - 1) {
      sentenceBuffer[bufferIndex] = c;
      bufferIndex++;
    }
  }
}

// =============================================================
//  NMEA FIELD-LEVEL DECODING
// =============================================================

/**
 * Splits comma-separated elements in an NMEA sentence and updates variables.
 */
bool parseNMEASentence(String sentence) {
  // Check checksum validity (optional but recommended for high-reliability)
  int starIndex = sentence.indexOf('*');
  if (starIndex != -1) {
    // Extract checksum byte
    String checksumStr = sentence.substring(starIndex + 1);
    uint8_t targetCheck = (uint8_t)strtol(checksumStr.c_str(), NULL, 16);
    
    // Calculate XOR checksum
    uint8_t calculatedCheck = 0;
    for (int i = 1; i < starIndex; i++) {
      calculatedCheck ^= sentence[i];
    }
    
    if (calculatedCheck != targetCheck) {
      Serial.println(F("[ERROR] Checksum failed! Ignoring corrupted sentence."));
      return false;
    }
  }

  // Tokenize sentence into fields
  String fields[20];
  int fieldCount = 0;
  int lastComma = 0;
  
  for (int i = 0; i < sentence.length(); i++) {
    if (sentence[i] == ',' || sentence[i] == '*') {
      fields[fieldCount] = sentence.substring(lastComma + 1, i);
      fieldCount++;
      lastComma = i;
      if (sentence[i] == '*' || fieldCount >= 20) break;
    }
  }

  // Identify sentence type
  String header = fields[0]; // e.g. "$GPRMC" or "$GPGGA"
  
  if (header.endsWith("RMC")) {
    // --- PARSE GPRMC (Recommended Minimum Navigation Data) ---
    // Field 1: UTC Time (hhmmss.ss)
    // Field 2: Status (A = Valid, V = Void/Invalid)
    // Field 3: Lat Raw
    // Field 4: Latitude Hemisphere (N/S)
    // Field 5: Lon Raw
    // Field 6: Longitude Hemisphere (E/W)
    // Field 9: Date (ddmmyy)
    
    if (fields[2] == "A") {
      fixValid = true;
      latitude  = convertToDecimalDegrees(fields[3], fields[4]);
      longitude = convertToDecimalDegrees(fields[5], fields[6]);
    } else {
      fixValid = false;
    }

    if (fields[1].length() >= 6) {
      utcTime = fields[1].substring(0, 2) + ":" + fields[1].substring(2, 4) + ":" + fields[1].substring(4, 6);
    }
    if (fields[9].length() == 6) {
      fixDate = fields[9].substring(0, 2) + "-" + fields[9].substring(2, 4) + "-20" + fields[9].substring(4, 6);
    }
    return true;
  } 
  else if (header.endsWith("GGA")) {
    // --- PARSE GPGGA (Global Positioning System Fix Data) ---
    // Field 2: Lat Raw | Field 3: N/S | Field 4: Lon Raw | Field 5: E/W
    // Field 6: Fix Quality (0 = invalid, 1 = GPS fix, 2 = DGPS fix)
    // Field 7: Satellites Tracked
    
    int fixQuality = fields[6].toInt();
    fixValid = (fixQuality > 0);
    
    if (fixValid) {
      latitude  = convertToDecimalDegrees(fields[2], fields[3]);
      longitude = convertToDecimalDegrees(fields[4], fields[5]);
    }
    
    satellitesTracked = fields[7].toInt();
    
    if (fields[1].length() >= 6) {
      utcTime = fields[1].substring(0, 2) + ":" + fields[1].substring(2, 4) + ":" + fields[1].substring(4, 6);
    }
    return true;
  }
  
  return false;
}

/**
 * Translates raw NMEA coordinate strings (DDMM.MMMM) to floating-point decimal degrees.
 */
float convertToDecimalDegrees(String rawCoord, String hemisphere) {
  if (rawCoord.length() == 0) return 0.0f;

  float rawValue = rawCoord.toFloat();
  
  // Find where the degree boundaries sit (depends on longitude vs latitude)
  // Latitude has 2 degree digits (DDMM.MMMM), Longitude has 3 digits (DDDMM.MMMM)
  // We can isolate degrees by dividing by 100 and casting to an integer:
  int degrees = (int)(rawValue / 100.0f);
  float minutes = rawValue - (degrees * 100.0f);
  
  float decimalDegrees = degrees + (minutes / 60.0f);
  
  // Adjust sign for Southern and Western hemispheres
  if (hemisphere == "S" || hemisphere == "W") {
    decimalDegrees = -decimalDegrees;
  }
  
  return decimalDegrees;
}

// =============================================================
//  TELEMETRY & COMMAND HELP PRINT UTILITIES
// =============================================================
void printGPSData() {
  Serial.println(F("\n----------------- GPS DECODED TELEMETRY -----------------"));
  Serial.print(F(" UTC Time   : ")); Serial.println(utcTime);
  Serial.print(F(" Date       : ")); Serial.println(fixDate);
  Serial.print(F(" Fix Status : ")); Serial.println(fixValid ? F("VALID (Lock Established)") : F("VOID (Searching Satellites)"));
  if (fixValid) {
    Serial.print(F(" Latitude   : ")); Serial.print(latitude, 6); Serial.println(F("°"));
    Serial.print(F(" Longitude  : ")); Serial.print(longitude, 6); Serial.println(F("°"));
  } else {
    Serial.println(F(" Latitude   : --.------"));
    Serial.println(F(" Longitude  : --.------"));
  }
  Serial.print(F(" Satellites : ")); Serial.println(satellitesTracked);
  Serial.println(F("---------------------------------------------------------"));
}

void printMenu() {
  Serial.println(F("\n--- GPS NMEA PARSER INTERACTIVE MENU ---"));
  Serial.println(F(" Paste raw NMEA sentences directly into the console to test the parser."));
  Serial.println(F(" Examples:"));
  Serial.println(F("  * GPRMC (Valid Fix): $GPRMC,123519.00,A,1258.9876,N,07735.1234,E,000.0,000.0,040626,,,A*6C"));
  Serial.println(F("  * GPGGA (Satellite Status): $GPGGA,123519.00,1258.9876,N,07735.1234,E,1,08,0.9,105.5,M,-30.0,M,,*41"));
  Serial.println(F("  * GPRMC (Void - No Fix): $GPRMC,123519.00,V,,,,,,,040626,,,N*47"));
  Serial.println(F(" 'h' : Display this help command list"));
  Serial.println(F("----------------------------------------"));
}

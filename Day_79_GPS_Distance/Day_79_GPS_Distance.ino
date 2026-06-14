/*
 * 100 Projects with Arduino - Day 79
 * Project: GPS Distance & Bearing Calculator (Haversine Formula)
 *
 * DESCRIPTION:
 * This project implements spherical geometry algorithms to calculate the shortest path
 * distance (great-circle distance) and bearing (compass heading) between two geographical
 * GPS coordinates (Latitude/Longitude). To meet autonomous robotic routing requirements:
 * 1. Haversine Formula: Implements double-precision trigonometric math to calculate the distance
 *    in meters, accounting for the Earth's ellipsoidal curvature.
 * 2. True North Bearing Solver: Computes the target heading angle ($0^\circ - 360^\circ$) from the
 *    Start (Home) point to the Current (Target) point.
 * 3. GPS Integration: Interfaces a physical NEO-6M GPS module over SoftwareSerial to calculate
 *    real-time distance and bearing back to a set "Home Base".
 * 4. Navigational Simulator Shell: Allows manual input of Home (`h lat lon`) and Current (`c lat
 * lon`) positions to verify distance/bearing math immediately in the console.
 *
 * NAVIGATIONAL MATHEMATICS:
 * - Haversine Formula:
 *   $$\Delta\phi = \text{lat}_2 - \text{lat}_1, \quad \Delta\lambda = \text{lon}_2 - \text{lon}_1$$
 *   $$a = \sin^2(\Delta\phi/2) + \cos(\text{lat}_1)\cos(\text{lat}_2)\sin^2(\Delta\lambda/2)$$
 *   $$c = 2 \cdot \text{atan2}(\sqrt{a}, \sqrt{1-a})$$
 *   $$\text{Distance } d = R \cdot c \quad (\text{where } R \approx 6,371,000\,\text{meters})$$
 * - Bearing Formula (True Heading):
 *   $$\theta = \text{atan2}(\sin(\Delta\lambda)\cos(\text{lat}_2),
 * \cos(\text{lat}_1)\sin(\text{lat}_2) - \sin(\text{lat}_1)\cos(\text{lat}_2)\cos(\Delta\lambda))$$
 *   Bearing (degrees) $= (\theta \cdot 180 / \pi + 360) \pmod{360}$
 *
 * WIRING:
 * - NEO-6M GPS Pin -> Arduino Uno Pin
 *   - VCC -> 5V | GND -> GND | TX -> Pin 2 | RX -> Pin 3 (via resistor divider)
 */

#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const int GPS_RX_PIN = 2;  // GPS TX
const int GPS_TX_PIN = 3;  // GPS RX

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// --- NMEA BUFFER VARIABLES ---
const int BUFFER_SIZE = 100;
char sentenceBuffer[BUFFER_SIZE];
int bufferIndex = 0;
bool isRecording = false;

// --- GEOGRAPHICAL VARIABLES (Decimal Degrees) ---
// Default Home base: Bangalore, India (roughly 12.9716° N, 77.5946° E)
float homeLat = 12.9716f;
float homeLon = 77.5946f;

float currentLat = 0.0f;
float currentLon = 0.0f;
bool currentFixValid = false;

// Earth radius in meters
const float EARTH_RADIUS_M = 6371000.0f;
// const float DEG_TO_RAD = PI / 180.0f; // Already defined in Arduino.h
// const float RAD_TO_DEG = 180.0f / PI; // Already defined in Arduino.h

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 79: GPS Distance & Bearing (Haversine Solver)"));
  Serial.println(F("=================================================="));

  printHomeBase();
  printMenu();
}

void loop() {
  // 1. Read input from physical GPS receiver
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    processGPSChar(c);
  }

  // 2. Poll hardware serial for CLI simulation commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'h':
      case 'H':
        // Set Home position: h lat lon
        homeLat = Serial.parseFloat();
        homeLon = Serial.parseFloat();
        Serial.print(F("[NAV] Home Base updated to: "));
        printCoords(homeLat, homeLon);
        calculateNavTelemetry();
        break;

      case 'c':
      case 'C':
        // Set Current/Target position: c lat lon
        currentLat = Serial.parseFloat();
        currentLon = Serial.parseFloat();
        currentFixValid = true;
        Serial.print(F("[NAV] Current Position updated to: "));
        printCoords(currentLat, currentLon);
        calculateNavTelemetry();
        break;

      case 'i':
      case 'I':
        printHomeBase();
        break;

      case 'm':
      case 'M':
        printMenu();
        break;

      default:
        break;
    }
  }
}

// =============================================================
//  SPHERICAL NAVIGATION SOLVERS (MATH)
// =============================================================

/**
 * Calculates distance (Haversine) and bearing (compass angle) from Home to Current.
 */
void calculateNavTelemetry() {
  if (homeLat == 0.0f && homeLon == 0.0f) {
    Serial.println(F("[NAV] ERROR: Home Base coordinates not initialized."));
    return;
  }

  // Convert coordinates to radians
  float lat1 = homeLat * DEG_TO_RAD;
  float lon1 = homeLon * DEG_TO_RAD;
  float lat2 = currentLat * DEG_TO_RAD;
  float lon2 = currentLon * DEG_TO_RAD;

  // Differences
  float dLat = lat2 - lat1;
  float dLon = lon2 - lon1;

  // --- HAVERSINE DISTANCE MATH ---
  float a = sin(dLat / 2.0f) * sin(dLat / 2.0f) +
            cos(lat1) * cos(lat2) * sin(dLon / 2.0f) * sin(dLon / 2.0f);

  float c = 2.0f * atan2(sqrt(a), sqrt(1.0f - a));
  float distanceMeters = EARTH_RADIUS_M * c;

  // --- BEARING (HEADING) MATH ---
  float y = sin(dLon) * cos(lat2);
  float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);

  float bearingRad = atan2(y, x);
  float bearingDegrees = bearingRad * RAD_TO_DEG;

  // Normalize bearing to 0 - 360 degrees
  if (bearingDegrees < 0.0f) {
    bearingDegrees += 360.0f;
  }

  // --- PRINT NAVIGATION SUMMARY ---
  Serial.println(F("\n====== NAVIGATIONAL VECTOR CALCULATED ======"));
  Serial.print(F(" Origin (Home) : "));
  printCoords(homeLat, homeLon);
  Serial.print(F(" Target        : "));
  printCoords(currentLat, currentLon);
  Serial.print(F(" Distance      : "));
  if (distanceMeters >= 1000.0f) {
    Serial.print(distanceMeters / 1000.0f, 3);
    Serial.println(F(" km"));
  } else {
    Serial.print(distanceMeters, 1);
    Serial.println(F(" meters"));
  }

  Serial.print(F(" Bearing       : "));
  Serial.print(bearingDegrees, 1);
  Serial.print(F("° ("));
  printCompassHeading(bearingDegrees);
  Serial.println(F(")"));
  Serial.println(F("============================================"));
}

// =============================================================
//  NMEA STREAM FILTERING & DECODING
// =============================================================
void processGPSChar(char c) {
  if (c == '$') {
    bufferIndex = 0;
    isRecording = true;
    sentenceBuffer[bufferIndex] = c;
    bufferIndex++;
  } else if (isRecording) {
    if (c == '\r' || c == '\n') {
      sentenceBuffer[bufferIndex] = '\0';
      isRecording = false;

      String sentence = String(sentenceBuffer);
      if (parseGPSCoordinates(sentence)) {
        // Automatically re-calculate navigation telemetry upon fresh GPS locks
        if (currentFixValid) {
          calculateNavTelemetry();
        }
      }
    } else if (bufferIndex < BUFFER_SIZE - 1) {
      sentenceBuffer[bufferIndex] = c;
      bufferIndex++;
    }
  }
}

bool parseGPSCoordinates(String sentence) {
  if (!sentence.startsWith("$GPRMC") && !sentence.startsWith("$GPGGA")) return false;

  // Split sentence by commas
  String fields[15];
  int fieldCount = 0;
  int lastComma = 0;

  for (int i = 0; i < sentence.length(); i++) {
    if (sentence[i] == ',' || sentence[i] == '*') {
      fields[fieldCount] = sentence.substring(lastComma + 1, i);
      fieldCount++;
      lastComma = i;
      if (sentence[i] == '*' || fieldCount >= 15) break;
    }
  }

  // Parse GPRMC
  if (fields[0].endsWith("RMC")) {
    if (fields[2] == "A") {
      currentFixValid = true;
      currentLat = convertToDecimalDegrees(fields[3], fields[4]);
      currentLon = convertToDecimalDegrees(fields[5], fields[6]);
      return true;
    } else {
      currentFixValid = false;
    }
  }
  // Parse GPGGA
  else if (fields[0].endsWith("GGA")) {
    int quality = fields[6].toInt();
    if (quality > 0) {
      currentFixValid = true;
      currentLat = convertToDecimalDegrees(fields[2], fields[3]);
      currentLon = convertToDecimalDegrees(fields[4], fields[5]);
      return true;
    } else {
      currentFixValid = false;
    }
  }
  return false;
}

float convertToDecimalDegrees(String rawCoord, String hemisphere) {
  if (rawCoord.length() == 0) return 0.0f;
  float val = rawCoord.toFloat();
  int deg = (int)(val / 100.0f);
  float min = val - (deg * 100.0f);
  float decimalDeg = deg + (min / 60.0f);
  if (hemisphere == "S" || hemisphere == "W") decimalDeg = -decimalDeg;
  return decimalDeg;
}

// =============================================================
//  FORMATTING PRINT UTILITIES
// =============================================================
void printCoords(float lat, float lon) {
  Serial.print(lat, 6);
  Serial.print(F(", "));
  Serial.println(lon, 6);
}

void printCompassHeading(float degrees) {
  if (degrees >= 337.5f || degrees < 22.5f)
    Serial.print(F("North"));
  else if (degrees >= 22.5f && degrees < 67.5f)
    Serial.print(F("North-East"));
  else if (degrees >= 67.5f && degrees < 112.5f)
    Serial.print(F("East"));
  else if (degrees >= 112.5f && degrees < 157.5f)
    Serial.print(F("South-East"));
  else if (degrees >= 157.5f && degrees < 202.5f)
    Serial.print(F("South"));
  else if (degrees >= 202.5f && degrees < 247.5f)
    Serial.print(F("South-West"));
  else if (degrees >= 247.5f && degrees < 292.5f)
    Serial.print(F("West"));
  else if (degrees >= 292.5f && degrees < 337.5f)
    Serial.print(F("North-West"));
}

void printHomeBase() {
  Serial.print(F("[NAV] Home Base location: "));
  printCoords(homeLat, homeLon);
}

void printMenu() {
  Serial.println(F("\n--- NAVIGATION VECTOR SIMULATOR SHELL ---"));
  Serial.println(F(" 'h [lat] [lon]' : Set Home Base Coordinates (e.g. h 12.9716 77.5946)"));
  Serial.println(F(" 'c [lat] [lon]' : Set Target Coordinates    (e.g. c 12.9789 77.6034)"));
  Serial.println(F(" 'i'             : Print current configuration"));
  Serial.println(F(" 'm'             : Print this command menu"));
  Serial.println(F("------------------------------------------"));
}

/*
 * 100 Projects with Arduino - Day 29
 * Project: DS3231 Real-Time Clock (Direct I2C Register Programming)
 *
 * DESCRIPTION:
 * This project interfaces the high-precision DS3231 Real-Time Clock (RTC) module.
 * Rather than relying on high-level wrappers, this sketch implements direct register-level I2C
 * communication using the standard Arduino Wire library.
 * 1. Reads and writes seconds, minutes, hours, day of week, day of month, month, and year.
 * 2. Implements conversions between Decimal and Binary Coded Decimal (BCD) formatting.
 * 3. Accesses the DS3231 internal Temperature registers (0x11, 0x12) to calculate chip temperature.
 * 4. Implements a non-blocking 1-second display refresh loop using millis().
 *
 * THE PHYSICS & HARDWARE THEORY:
 * - Real-Time Clock (RTC): Standard microcontrollers use crystal oscillators that drift due to
 *   temperature changes, causing time offsets of minutes per week. The DS3231 incorporates an
 * internal 32.768 kHz Temperature Compensated Crystal Oscillator (TCXO). An internal temperature
 * sensor measures temperature and adjusts the load capacitance of the crystal to maintain ±2ppm
 * accuracy (under 1 minute of drift per year).
 * - Binary Coded Decimal (BCD): The DS3231 stores time data in BCD format. In BCD, each decimal
 * digit is encoded into a 4-bit nibble. For example, the decimal number 45 is stored in a single
 * byte as 0x45 (which is 0100 0101 in binary) instead of its true mathematical binary
 * representation 0010 1101.
 * - Temperature Register: Registers 0x11 (MSB) and 0x12 (LSB) store the temperature. The MSB is a
 *   signed integer in 2's complement format. The upper 2 bits of the LSB represent the fractional
 *   values in steps of 0.25°C.
 *
 * WIRING:
 * - DS3231 VCC -> Arduino 5V
 * - DS3231 GND -> Arduino GND
 * - DS3231 SDA -> Arduino SDA (A4 on Uno)
 * - DS3231 SCL -> Arduino SCL (A5 on Uno)
 */

#include <Wire.h>

#define DS3231_I2C_ADDRESS 0x68  // Hardware address of the DS3231 RTC

// --- DATA STRUCTURE FOR TIME AND DATE ---
struct RTCDateTime {
  byte second;
  byte minute;
  byte hour;
  byte dayOfWeek;   // 1 = Sunday, 2 = Monday, etc.
  byte dayOfMonth;  // 1 - 31
  byte month;       // 1 - 12
  byte year;        // 0 - 99 (represents 2000 - 2099)
};

// Target date/time to program onto the module if the battery has been removed or time is unset.
// Keep setTimeFlag to 'false' after initial programming to avoid resetting time on every boot!
const bool setTimeFlag = false;

// --- TIMING VARIABLES ---
unsigned long lastDisplayRefresh = 0;
const unsigned long refreshInterval = 1000;  // 1 second refresh rate

void setup() {
  Wire.begin();  // Initialize I2C Bus as Master
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 29: DS3231 Real-Time Clock & Thermometer");
  Serial.println("==================================================");

  // Set initial time if flag is enabled
  if (setTimeFlag) {
    RTCDateTime initialTime = {
        0,   // second
        30,  // minute
        15,  // hour (15:30:00 / 3:30 PM in 24hr format)
        5,   // day of week (Thursday)
        4,   // day of month (4th)
        6,   // month (June)
        26   // year (2026)
    };
    setRTCDateTime(initialTime);
    Serial.println("[RTC] Time written to registers successfully!");
  } else {
    Serial.println("[RTC] Reading running registers (Set 'setTimeFlag = true' to program time).");
  }
}

void loop() {
  unsigned long currentTime = millis();

  // Non-blocking timer to read and print date/time every 1 second
  if (currentTime - lastDisplayRefresh >= refreshInterval) {
    lastDisplayRefresh = currentTime;

    RTCDateTime now;
    if (readRTCDateTime(&now)) {
      printDateTime(now);

      float temp = readRTCTemperature();
      Serial.print(" | Temp: ");
      Serial.print(temp, 2);
      Serial.println(" C");
    } else {
      Serial.println("[ERROR] Failed to read from DS3231 I2C interface.");
    }
  }
}

// --- CONVERSION UTILITIES ---

/**
 * Converts standard Decimal format to Binary Coded Decimal (BCD).
 */
byte decToBcd(byte val) {
  return ((val / 10) << 4) | (val % 10);
}

/**
 * Converts Binary Coded Decimal (BCD) format to standard Decimal.
 */
byte bcdToDec(byte val) {
  return ((val >> 4) * 10) + (val & 0x0F);
}

// --- REGISTER WRITE OPERATIONS ---

/**
 * Writes a full date-time structure to the DS3231 register map.
 */
void setRTCDateTime(RTCDateTime dt) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);  // Start at seconds register (register address pointer)

  Wire.write(decToBcd(dt.second));
  Wire.write(decToBcd(dt.minute));
  Wire.write(decToBcd(dt.hour) & 0x3F);  // Ensure 24-hour mode (bit 6 = 0)
  Wire.write(decToBcd(dt.dayOfWeek));
  Wire.write(decToBcd(dt.dayOfMonth));
  Wire.write(decToBcd(dt.month));
  Wire.write(decToBcd(dt.year));

  Wire.endTransmission();
}

// --- REGISTER READ OPERATIONS ---

/**
 * Reads the first 7 registers from the DS3231 and decodes BCD to Decimal.
 */
bool readRTCDateTime(RTCDateTime* dt) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);  // Point to seconds register
  if (Wire.endTransmission() != 0) {
    return false;  // Communication error
  }

  // Request 7 bytes of data (registers 0x00 to 0x06)
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);

  if (Wire.available() >= 7) {
    dt->second = bcdToDec(Wire.read() & 0x7F);  // Bit 7 is CH (Clock Halt), mask it
    dt->minute = bcdToDec(Wire.read());
    dt->hour = bcdToDec(Wire.read() & 0x3F);  // 24-hour mode masking
    dt->dayOfWeek = bcdToDec(Wire.read());
    dt->dayOfMonth = bcdToDec(Wire.read());
    dt->month = bcdToDec(Wire.read() & 0x1F);  // Month mask
    dt->year = bcdToDec(Wire.read());
    return true;
  }

  return false;
}

/**
 * Reads registers 0x11 and 0x12 to extract the signed temperature.
 */
float readRTCTemperature() {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);  // Temperature registers start at 0x11
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);

  if (Wire.available() >= 2) {
    int8_t msb = Wire.read();   // MSB is signed integer (2's complement)
    uint8_t lsb = Wire.read();  // LSB fractional bits

    // The upper 2 bits of LSB represent fractional temperature: 0.25 * (lsb >> 6)
    float fractional = (lsb >> 6) * 0.25;
    return (float)msb + fractional;
  }

  return 0.0;
}

// --- TELEMETRY PRINT UTILITIES ---

/**
 * Helper to display formatted time output with leading zeros.
 */
void printDateTime(RTCDateTime dt) {
  // Print date (YYYY-MM-DD)
  Serial.print("20");
  if (dt.year < 10) Serial.print("0");
  Serial.print(dt.year);
  Serial.print("-");
  if (dt.month < 10) Serial.print("0");
  Serial.print(dt.month);
  Serial.print("-");
  if (dt.dayOfMonth < 10) Serial.print("0");
  Serial.print(dt.dayOfMonth);

  Serial.print(" ");

  // Print time (HH:MM:SS)
  if (dt.hour < 10) Serial.print("0");
  Serial.print(dt.hour);
  Serial.print(":");
  if (dt.minute < 10) Serial.print("0");
  Serial.print(dt.minute);
  Serial.print(":");
  if (dt.second < 10) Serial.print("0");
  Serial.print(dt.second);

  // Print day of week string
  Serial.print(" (");
  switch (dt.dayOfWeek) {
    case 1:
      Serial.print("Sunday");
      break;
    case 2:
      Serial.print("Monday");
      break;
    case 3:
      Serial.print("Tuesday");
      break;
    case 4:
      Serial.print("Wednesday");
      break;
    case 5:
      Serial.print("Thursday");
      break;
    case 6:
      Serial.print("Friday");
      break;
    case 7:
      Serial.print("Saturday");
      break;
  }
  Serial.print(")");
}

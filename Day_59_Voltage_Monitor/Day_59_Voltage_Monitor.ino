/*
 * 100 Projects with Arduino - Day 59
 * Project: Precision Voltage & Current Monitor (ADC + INA219 I2C Power Sensor)
 * 
 * DESCRIPTION:
 * This project implements a high-precision power monitoring system using two methods:
 *  1. INA219 I2C Current/Power Sensor: Measures bus voltage, shunt voltage, current,
 *     and power by reading internal 12-bit ADC registers over I2C without any library.
 *  2. Arduino Raw ADC: Monitors an external voltage rail (up to ~50V with resistor
 *     divider) using the 10-bit ADC and an over-voltage alert threshold.
 * 
 * INA219 I2C REGISTER MAP:
 *  0x00 = Configuration Register (12-bit mode, range, averaging)
 *  0x01 = Shunt Voltage Register (signed 16-bit, LSB = 10 uV)
 *  0x02 = Bus Voltage Register   (12-bit result in bits 15:3, LSB = 4 mV)
 *  0x03 = Power Register         (12-bit, calibrated LSB based on Cal register)
 *  0x04 = Current Register       (12-bit signed, calibrated LSB)
 *  0x05 = Calibration Register   (Cal = trunc(0.04096 / (Current_LSB * R_shunt)))
 * 
 * CALIBRATION MATH:
 *  R_shunt     = 0.1 Ohm (typical inline shunt resistor)
 *  Current_LSB = 0.001 A (1 mA resolution)
 *  Cal         = trunc(0.04096 / (0.001 * 0.1)) = 4096 = 0x1000
 *  Current_mA  = Current_Reg * Current_LSB * 1000
 *  Power_mW    = Power_Reg   * 20 * Current_LSB * 1000
 * 
 * RESISTOR DIVIDER (external voltage rail monitoring):
 *  R1 = 47k (top), R2 = 5.1k (bottom to GND)
 *  V_arduino_pin = V_rail * R2 / (R1 + R2) < 5V (safe for ADC)
 *  V_rail = V_arduino_pin * (R1 + R2) / R2
 *  With 10-bit ADC: V_arduino_pin = ADC_count * (5.0 / 1023.0)
 * 
 * WIRING:
 *  INA219 VCC  -> 3.3V or 5V
 *  INA219 GND  -> GND
 *  INA219 SDA  -> A4
 *  INA219 SCL  -> A5
 *  INA219 IN+  -> Positive terminal of load supply rail
 *  INA219 IN-  -> Negative terminal / IN+ side of shunt resistor
 *  External voltage divider middle point -> A0
 */

#include <Wire.h>

// --- INA219 I2C ADDRESS ---
// Default address: 0x40 (A0=GND, A1=GND)
const uint8_t INA219_ADDR = 0x40;

// --- INA219 REGISTER ADDRESSES ---
const uint8_t REG_CONFIG     = 0x00;
const uint8_t REG_SHUNT_V    = 0x01;
const uint8_t REG_BUS_V      = 0x02;
const uint8_t REG_POWER      = 0x03;
const uint8_t REG_CURRENT    = 0x04;
const uint8_t REG_CALIBRATION= 0x05;

// --- CALIBRATION VALUES ---
// R_shunt = 0.1 Ohm, Current_LSB = 1 mA
// Cal = trunc(0.04096 / (0.001 * 0.1)) = 4096
const uint16_t CAL_VALUE     = 4096;
const float    CURRENT_LSB   = 0.001f;  // 1 mA per count

// --- RESISTOR DIVIDER CONSTANTS (for external rail monitoring) ---
const float R1 = 47000.0f;             // 47k top resistor (Ohms)
const float R2 = 5100.0f;              // 5.1k bottom resistor (Ohms)
const float DIVIDER_RATIO = (R1 + R2) / R2; // = (47000+5100)/5100 ≈ 10.22

// --- ALERT THRESHOLDS ---
const float OVERVOLTAGE_THRESHOLD = 30.0f; // Volts — alert if rail exceeds this
const float OVERCURRENT_THRESHOLD = 2.0f;  // Amps — alert if current exceeds this
const int   ALERT_PIN             = 13;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(ALERT_PIN, OUTPUT);
  
  // Write INA219 Configuration Register
  // Config: 32V bus range, ±320mV shunt, 12-bit 128-sample averaging
  // BRNG=1 (32V), PG=11 (±320mV), BADC=1111 (128 avg), SADC=1111 (128 avg), MODE=111 (continuous)
  uint16_t config = 0b0011111111111111;
  writeRegister16(INA219_ADDR, REG_CONFIG, config);
  delay(1); // Let ADC settle after config

  // Write Calibration Register
  writeRegister16(INA219_ADDR, REG_CALIBRATION, CAL_VALUE);

  Serial.println(F("[ INA219 ] Power Monitor initialized."));
  Serial.println(F("[ ADC    ] External voltage rail monitor on A0 (R1=47k, R2=5.1k)."));
  printHeader();
}

void loop() {
  // --- Read INA219 over I2C ---
  // Bus Voltage: bits 15:3 = voltage, bit 1 = CNVR (conversion ready), bit 0 = OVF
  uint16_t rawBus  = readRegister16(INA219_ADDR, REG_BUS_V);
  int16_t  rawShunt = (int16_t)readRegister16(INA219_ADDR, REG_SHUNT_V);
  int16_t  rawCurr  = (int16_t)readRegister16(INA219_ADDR, REG_CURRENT);
  uint16_t rawPow   = readRegister16(INA219_ADDR, REG_POWER);

  // Convert to physical units
  float busVoltage_V  = (float)(rawBus >> 3) * 0.004f;       // LSB = 4 mV
  float shuntVoltage_mV = (float)rawShunt * 0.01f;            // LSB = 10 uV = 0.01 mV
  float current_mA     = (float)rawCurr * CURRENT_LSB * 1000; // mA
  float power_mW       = (float)rawPow  * 20.0f * CURRENT_LSB * 1000;
  float loadVoltage_V  = busVoltage_V + (shuntVoltage_mV / 1000.0f);

  // --- Read external rail voltage via raw ADC ---
  int   rawADC = analogRead(A0);
  float pinVoltage  = (float)rawADC * (5.0f / 1023.0f);
  float railVoltage = pinVoltage * DIVIDER_RATIO;

  // --- Alert logic ---
  bool overV = (railVoltage > OVERVOLTAGE_THRESHOLD);
  bool overI = (current_mA / 1000.0f > OVERCURRENT_THRESHOLD);
  digitalWrite(ALERT_PIN, (overV || overI) ? HIGH : LOW);

  // --- Print readings ---
  Serial.print(F("BusV: ")); Serial.print(busVoltage_V, 3);
  Serial.print(F("V | LoadV: ")); Serial.print(loadVoltage_V, 3);
  Serial.print(F("V | Shunt: ")); Serial.print(shuntVoltage_mV, 2);
  Serial.print(F("mV | Current: ")); Serial.print(current_mA, 1);
  Serial.print(F("mA | Power: ")); Serial.print(power_mW, 1);
  Serial.print(F("mW | Rail: ")); Serial.print(railVoltage, 2);
  Serial.print(F("V"));
  if (overV) Serial.print(F(" [OVER-VOLTAGE!]"));
  if (overI) Serial.print(F(" [OVER-CURRENT!]"));
  Serial.println();

  delay(500);
}

// --- I2C HELPER: Write 16-bit value to a register ---
void writeRegister16(uint8_t addr, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF); // MSB first
  Wire.write(value & 0xFF);         // LSB second
  Wire.endTransmission();
}

// --- I2C HELPER: Read 16-bit value from a register ---
uint16_t readRegister16(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false); // Repeated START for read
  Wire.requestFrom(addr, (uint8_t)2);
  uint16_t result = 0;
  if (Wire.available() >= 2) {
    result  = (uint16_t)Wire.read() << 8; // MSB
    result |= (uint16_t)Wire.read();       // LSB
  }
  return result;
}

// --- Print table header ---
void printHeader() {
  Serial.println(F("----------------------------------------------------------------------"));
  Serial.println(F("BusV | LoadV | Shunt(mV) | Current(mA) | Power(mW) | Rail(V)"));
  Serial.println(F("----------------------------------------------------------------------"));
}

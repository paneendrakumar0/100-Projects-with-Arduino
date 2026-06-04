/*
 * 100 Projects with Arduino - Day 76
 * Project: IMU-Based Fall Detection System (MPU6050 Direct Registers)
 * 
 * DESCRIPTION:
 * This project implements a high-reliability Fall Detection System using the MPU6050 accelerometer
 * read directly via low-level I2C commands. To align with professional medical alert and robotic
 * safety systems, it implements a 4-phase sequential Finite State Machine (FSM):
 * 1. Free-Fall Phase: Detects when the total acceleration magnitude ($a_{tot}$) drops below 0.4g
 *    and remains low for at least 80ms, indicating a descent.
 * 2. Impact Phase: Detects a massive acceleration spike ($a_{tot} > 3.0g$) within 500ms of free-fall,
 *    indicating collision with the ground.
 * 3. Inactivity/Post-Fall Phase: Monitors the sensor for at least 2.0 seconds after the impact.
 *    If the sensor is lying still (variance $< 0.15g$) and in a tilted orientation (tilt angle $> 45^\circ$),
 *    it confirms the subject is down.
 * 4. Alarm Phase: Activates a warning buzzer and sends an alert over the serial console.
 * 
 * FALL DETECTION MATHEMATICS & FSM:
 * - Acceleration Vector Magnitude ($a_{tot}$):
 *   $$a_{tot} = \sqrt{a_x^2 + a_y^2 + a_z^2}$$
 * - Tilt Angle Calculation (from Z-axis proper gravity):
 *   $$\theta_{tilt} = \arccos(a_z / a_{tot}) \cdot \frac{180}{\pi}$$
 *   When standing upright, $a_z \approx 1.0g \rightarrow \theta_{tilt} \approx 0^\circ$.
 *   When lying down, $a_z \approx 0.0g \rightarrow \theta_{tilt} \approx 90^\circ$.
 * 
 * WIRING:
 * - MPU6050 Pin -> Arduino Uno Pin
 *   - VCC        -> 5V
 *   - GND        -> GND
 *   - SCL        -> Pin A5 (SCL)
 *   - SDA        -> Pin A4 (SDA)
 *   - AD0        -> GND
 * - Indicators:
 *   - Active Buzzer -> Pin 8 (via 220Ω resistor or driver transistor)
 *   - Red Alarm LED -> Pin 6 (via 220Ω resistor to GND)
 */

#include <Wire.h>

// --- MPU6050 CONFIG ---
const uint8_t MPU6050_ADDR     = 0x68;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;
const uint8_t REG_PWR_MGMT_1   = 0x6B;
const float ACCEL_SCALE_FACTOR = 16384.0f; // ±2g sensitivity

// --- PIN DEFINITIONS ---
const int BUZZER_PIN = 8;
const int ALARM_LED  = 6;

// --- FALL DETECTION FSM STATES ---
enum FallState {
  STATE_NORMAL,
  STATE_FREE_FALL,
  STATE_IMPACT_WAIT,
  STATE_INACTIVITY_CHECK,
  STATE_ALARM
};
FallState currentState = STATE_NORMAL;

// --- CRITICAL THRESHOLDS ---
const float THRES_FREE_FALL     = 0.40f;  // Free-fall limit (g)
const float THRES_IMPACT        = 2.80f;  // Impact spike limit (g)
const float THRES_STILL         = 0.15f;  // Max deviation to be considered "still"
const float THRES_TILT          = 45.0f;  // Tilt angle limit (degrees)

// --- TIMER LIMITS (ms) ---
const unsigned long TIME_FREE_FALL_MIN   = 80;   // Min duration in free-fall to trigger
const unsigned long TIME_IMPACT_TIMEOUT  = 500;  // Max time to wait for impact after free-fall
const unsigned long TIME_INACTIVITY_REQ   = 2000; // Duration sensor must be still to confirm fall
const unsigned long TIME_ALARM_RESET     = 10000;// Automatically reset alarm after 10 seconds

// --- STATE TIMERS ---
unsigned long freeFallStartTime = 0;
unsigned long impactTime = 0;
unsigned long inactivityStartTime = 0;
unsigned long alarmStartTime = 0;

// Simulation variables (allows testing without hardware)
bool simulationMode = false;
float simAx = 0.0f, simAy = 0.0f, simAz = 1.0f;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ALARM_LED, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(ALARM_LED, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F("Day 76: MPU6050 Fall Detection System"));
  Serial.println(F("=================================================="));

  if (!initMPU6050()) {
    Serial.println(F("[SYSTEM] MPU6050 not found. Entering Simulation Mode automatically."));
    simulationMode = true;
  } else {
    Serial.println(F("[SYSTEM] MPU6050 initialized successfully."));
  }

  printMenu();
}

void loop() {
  float ax, ay, az;

  // Get data (either from sensor or serial simulation)
  if (simulationMode) {
    ax = simAx;
    ay = simAy;
    az = simAz;
  } else {
    int16_t rawX, rawY, rawZ;
    if (readAccelRaw(rawX, rawY, rawZ)) {
      ax = rawX / ACCEL_SCALE_FACTOR;
      ay = rawY / ACCEL_SCALE_FACTOR;
      az = rawZ / ACCEL_SCALE_FACTOR;
    } else {
      Serial.println(F("[ERROR] Read failed!"));
      return;
    }
  }

  // Calculate Vector Magnitude (Total Acceleration G-Force)
  float aTot = sqrt(ax * ax + ay * ay + az * az);
  
  // Calculate Tilt Angle relative to vertical (Z-axis)
  // Clamp value to prevent NaN in acos
  float zRatio = az / aTot;
  if (zRatio > 1.0f) zRatio = 1.0f;
  if (zRatio < -1.0f) zRatio = -1.0f;
  float tilt = acos(zRatio) * (180.0f / PI);

  // Run Fall Detection Finite State Machine
  updateFallFSM(aTot, tilt);

  // Check for serial console inputs
  pollSerialCommands();

  // Print system status at 5 Hz
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    printStatus(aTot, tilt);
  }

  delay(20); // 50 Hz loop frequency
}

// =============================================================
//  FALL DETECTION FINITE STATE MACHINE (FSM)
// =============================================================
void updateFallFSM(float aTot, float tilt) {
  unsigned long now = millis();

  switch (currentState) {
    case STATE_NORMAL:
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(ALARM_LED, LOW);
      
      // If acceleration drops below free-fall threshold
      if (aTot < THRES_FREE_FALL) {
        currentState = STATE_FREE_FALL;
        freeFallStartTime = now;
        Serial.println(F("[FSM] -> FREE-FALL DETECTED"));
      }
      break;

    case STATE_FREE_FALL:
      // If we exit free-fall too quickly (temporary noise)
      if (aTot >= THRES_FREE_FALL) {
        if (now - freeFallStartTime >= TIME_FREE_FALL_MIN) {
          // Valid free-fall occurred, now wait for impact
          currentState = STATE_IMPACT_WAIT;
          impactTime = now;
          Serial.println(F("[FSM] -> WAITING FOR IMPACT SPIKE"));
        } else {
          // Fake free-fall (too short)
          currentState = STATE_NORMAL;
          Serial.println(F("[FSM] -> FALSE ALARM (Free-fall duration too short)"));
        }
      }
      break;

    case STATE_IMPACT_WAIT:
      // Check if we hit the ground (impact spike)
      if (aTot > THRES_IMPACT) {
        currentState = STATE_INACTIVITY_CHECK;
        inactivityStartTime = now;
        Serial.println(F("[FSM] -> IMPACT DETECTED! Checking for post-fall inactivity..."));
      }
      // Timeout waiting for impact
      else if (now - impactTime > TIME_IMPACT_TIMEOUT) {
        currentState = STATE_NORMAL;
        Serial.println(F("[FSM] -> FALSE ALARM (Impact timeout expired)"));
      }
      break;

    case STATE_INACTIVITY_CHECK:
      // If there is sudden movement (subject stood up / recovery)
      if (abs(aTot - 1.0f) > THRES_STILL) {
        // Reset inactivity check timer
        inactivityStartTime = now;
      }
      
      // If the sensor has remained quiet for the required duration
      if (now - inactivityStartTime >= TIME_INACTIVITY_REQ) {
        // Confirm the sensor is also tilted (lying down)
        if (tilt > THRES_TILT) {
          currentState = STATE_ALARM;
          alarmStartTime = now;
          Serial.println(F("[WARNING] !!! FALL DETECTED !!! ALARM TRIGGERED"));
        } else {
          // Sensor sits still but upright (e.g. placed flat on a table gently)
          currentState = STATE_NORMAL;
          Serial.println(F("[FSM] -> FALSE ALARM (Sensor is stationary but upright)"));
        }
      }
      
      // Safety timeout: if checking takes more than 5 seconds, reset
      if (now - impactTime > 5000) {
        currentState = STATE_NORMAL;
        Serial.println(F("[FSM] -> FALSE ALARM (Inactivity check timed out)"));
      }
      break;

    case STATE_ALARM:
      // Sound alarm: pulse buzzer and turn on LED
      digitalWrite(ALARM_LED, HIGH);
      // Generate pulsing tone
      if ((now / 200) % 2 == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }

      // Auto-reset alarm after timeout
      if (now - alarmStartTime > TIME_ALARM_RESET) {
        currentState = STATE_NORMAL;
        Serial.println(F("[SYSTEM] Alarm automatically reset. Returning to normal monitoring."));
      }
      break;
  }
}

// =============================================================
//  LOW-LEVEL MPU6050 REGISTER ACCESS
// =============================================================
bool initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75); // WHO_AM_I
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available()) {
    if (Wire.read() != 0x68) return false;
  }

  // Wake sensor
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00); // Wake
  if (Wire.endTransmission() != 0) return false;

  return true;
}

bool readAccelRaw(int16_t& x, int16_t& y, int16_t& z) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)6);
  if (Wire.available() >= 6) {
    x = (Wire.read() << 8) | Wire.read();
    y = (Wire.read() << 8) | Wire.read();
    z = (Wire.read() << 8) | Wire.read();
    return true;
  }
  return false;
}

// =============================================================
//  SERIAL CONSOLE SIMULATOR & SHELL
// =============================================================
void pollSerialCommands() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'm':
      case 'M':
        simulationMode = !simulationMode;
        Serial.print(F("[SYSTEM] Simulation Mode: "));
        Serial.println(simulationMode ? F("ON") : F("OFF"));
        break;

      case 's':
      case 'S':
        parseSimulatedData();
        break;

      case 'r':
      case 'R':
        currentState = STATE_NORMAL;
        Serial.println(F("[SYSTEM] Fall detector FSM reset to STATE_NORMAL."));
        break;

      case 'h':
      case 'H':
        printMenu();
        break;

      default:
        break;
    }
  }
}

void parseSimulatedData() {
  // Read simulation vectors, format: s ax ay az
  String line = Serial.readStringUntil('\n');
  line.trim();

  int firstSpace = line.indexOf(' ');
  int secondSpace = line.indexOf(' ', firstSpace + 1);

  if (firstSpace == -1 || secondSpace == -1) {
    Serial.println(F("[SHELL] Usage: s [ax] [ay] [az] (e.g. s 0.0 0.0 0.1)"));
    return;
  }

  String xStr = line.substring(0, firstSpace);
  String yStr = line.substring(firstSpace + 1, secondSpace);
  String zStr = line.substring(secondSpace + 1);

  simAx = xStr.toFloat();
  simAy = yStr.toFloat();
  simAz = zStr.toFloat();

  // If simulation mode wasn't on, activate it
  if (!simulationMode) {
    simulationMode = true;
    Serial.println(F("[SYSTEM] Simulation mode turned ON."));
  }

  Serial.print(F("[SIMULATION] Injected acceleration -> X: "));
  Serial.print(simAx, 2);
  Serial.print(F("g | Y: "));
  Serial.print(simAy, 2);
  Serial.print(F("g | Z: "));
  Serial.print(simAz, 2);
  Serial.println(F("g"));
}

void printStatus(float aTot, float tilt) {
  Serial.print(F("[STATUS] FSM: "));
  switch (currentState) {
    case STATE_NORMAL:           Serial.print(F("NORMAL          ")); break;
    case STATE_FREE_FALL:        Serial.print(F("FREE_FALL       ")); break;
    case STATE_IMPACT_WAIT:      Serial.print(F("IMPACT_WAIT     ")); break;
    case STATE_INACTIVITY_CHECK: Serial.print(F("INACTIVITY_CHECK")); break;
    case STATE_ALARM:            Serial.print(F("ALARM DETECTED  ")); break;
  }
  Serial.print(F(" | Accel: "));
  Serial.print(aTot, 3);
  Serial.print(F("g | Tilt: "));
  Serial.print(tilt, 1);
  Serial.println(F(" deg"));
}

void printMenu() {
  Serial.println(F("\n--- FALL DETECTOR COMMAND SHELL ---"));
  Serial.println(F(" 'm' : Toggle Simulation Mode (ON/OFF)"));
  Serial.println(F(" 's' : Inject simulated vector (Usage: s ax ay az)"));
  Serial.println(F("       * Step 1 (Free-Fall): s 0.1 0.1 0.1"));
  Serial.println(F("       * Step 2 (Impact):    s 2.0 2.0 2.0"));
  Serial.println(F("       * Step 3 (Lying flat):s 0.8 0.5 0.2"));
  Serial.println(F(" 'r' : Reset FSM state machine"));
  Serial.println(F(" 'h' : Display this command menu"));
  Serial.println(F("------------------------------------"));
}

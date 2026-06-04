/*
 * 100 Projects with Arduino - Day 18
 * Project: Corrosion-Mitigated Water Level Detector (Analog Power Gating)
 * 
 * DESCRIPTION:
 * This project interfaces a resistive water level sensor with the Arduino to measure water depth.
 * To implement professional mechatronics standards, this code features:
 * 1. Digital Power Gating: Instead of connecting the sensor VCC to the 5V line permanently, we
 *    power the sensor from a digital I/O pin (Pin 7). We pull this pin HIGH for 10ms to take a reading,
 *    then pull it LOW. This cuts power for 99% of the time, completely preventing electrochemical
 *    electrolysis corrosion of the sensor traces.
 * 2. Non-blocking timing: Measurements are scheduled once every 1 second using millis().
 * 3. Calibration Mapping: Raw 10-bit analog values are translated into descriptive water levels
 *    (Dry, Shallow, Half-Full, Full) based on calibrated thresholds.
 * 
 * WIRING:
 * - Sensor VCC (Power) -> Arduino Digital Pin 7 (Gated power supply)
 * - Sensor GND         -> Arduino GND
 * - Sensor Signal (S)  -> Arduino Analog Pin A0
 */

// --- PIN DEFINITIONS ---
const int SENSOR_POWER_PIN = 7; // Digital output pin supplying gated power to the sensor
const int SENSOR_SIGNAL_PIN = A0; // Analog input pin connected to the sensor signal trace

// --- CALIBRATION THRESHOLDS ---
// These values vary depending on water conductivity. Calibrate them for your tap water!
const int THRESHOLD_DRY = 50;     // Values below 50 indicate sensor is dry
const int THRESHOLD_LOW = 250;    // Threshold for "Shallow" level
const int THRESHOLD_MEDIUM = 500; // Threshold for "Half-Full" level
const int THRESHOLD_HIGH = 700;   // Threshold for "Full" level

// --- SCHEDULER VARIABLES ---
unsigned long lastMeasureTime = 0;       // Stores the last measurement timestamp
const unsigned long measureInterval = 1000; // Sample water level once per second (1 Hz)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure the sensor power pin as digital output
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  
  // Start with sensor power OFF (LOW) to prevent early corrosion
  digitalWrite(SENSOR_POWER_PIN, LOW);
  
  Serial.println("==================================================");
  Serial.println("Day 18: Corrosion-Mitigated Water Level Detector");
  Serial.println("==================================================");
  Serial.println("System armed. Monitoring depth...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Read the sensor at scheduled non-blocking interval (every 1 second)
  if (currentTime - lastMeasureTime >= measureInterval) {
    lastMeasureTime = currentTime;
    
    // Step 2: Power Gating - Turn the sensor ON
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    
    // Wait 10 milliseconds for the voltage divider and current lines to stabilize.
    // This short blocking delay is acceptable as it happens once a second and is microscopic.
    delay(10); 
    
    // Step 3: Read the analog voltage from the sensor (10-bit ADC)
    int sensorValue = analogRead(SENSOR_SIGNAL_PIN);
    
    // Step 4: Power Gating - Turn the sensor OFF immediately after reading
    digitalWrite(SENSOR_POWER_PIN, LOW);
    
    // Step 5: Translate ADC value into descriptive levels based on calibration
    String statusString = "";
    if (sensorValue < THRESHOLD_DRY) {
      statusString = "DRY (Empty)";
    } else if (sensorValue < THRESHOLD_LOW) {
      statusString = "LOW (Shallow)";
    } else if (sensorValue < THRESHOLD_MEDIUM) {
      statusString = "MEDIUM (Half-Full)";
    } else {
      statusString = "HIGH (Full / Alert)";
    }
    
    // Calculate approximate voltage representation (for display)
    float voltage = sensorValue * (5.0 / 1023.0);
    
    // Step 6: Log telemetry
    Serial.print("Raw ADC: ");
    Serial.print(sensorValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V | Water Level: ");
    Serial.println(statusString);
  }
  
  // Loop is non-blocking. Additional mechatronic control loops can execute here
}

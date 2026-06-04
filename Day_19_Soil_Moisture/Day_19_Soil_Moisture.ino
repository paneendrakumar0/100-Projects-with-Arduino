/*
 * 100 Projects with Arduino - Day 19
 * Project: Soil Moisture Monitor with Anti-Corrosion Gating
 * 
 * DESCRIPTION:
 * This project interfaces a resistive soil moisture sensor probe with the Arduino to measure
 * soil water content. Just like the water level sensor (Day 18), resistive soil probes corrode
 * rapidly if powered continuously due to galvanic electrolysis.
 * 
 * To meet professional mechatronics standards, this sketch implements:
 * 1. Gated Digital Power: Sensor is powered from Digital Pin 7 only during a brief 10ms window
 *    needed for sampling, once every 2 seconds.
 * 2. Calibration Zones: Mapped outputs classifying the soil into Dry, Moist, and Wet zones.
 * 
 * WIRING:
 * - Probe Module VCC  -> Arduino Digital Pin 7 (Gated power)
 * - Probe Module GND  -> Arduino GND
 * - Probe Module AO (Analog Out) -> Arduino Analog Pin A0
 */

// --- PIN DEFINITIONS ---
const int PROBE_POWER_PIN = 7;  // Digital output pin supplying gated power to the probe
const int PROBE_SIGNAL_PIN = A0; // Analog input pin connected to probe analog output

// --- CALIBRATION LIMITS ---
// Standard resistive soil probes (e.g. YL-69) output LOW voltage (0V) when wet,
// and HIGH voltage (5V) when completely dry. Calibrate these for your soil!
const int VAL_DRY = 750;  // Readings ABOVE this value mean soil is dry/needs watering
const int VAL_WET = 350;  // Readings BELOW this value mean soil is saturated/wet

// --- SCHEDULER VARIABLES ---
unsigned long lastMeasureTime = 0;       // Stores the last measurement timestamp
const unsigned long measureInterval = 2000; // Sample soil moisture once every 2 seconds (0.5 Hz)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure the sensor power pin as digital output
  pinMode(PROBE_POWER_PIN, OUTPUT);
  
  // Keep power OFF initially to prevent probe degradation
  digitalWrite(PROBE_POWER_PIN, LOW);
  
  Serial.println("==================================================");
  Serial.println("Day 19: Soil Moisture Monitor (Corrosion Gated)");
  Serial.println("==================================================");
  Serial.println("System armed. Monitoring soil moisture...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Read the sensor at scheduled non-blocking interval (every 2 seconds)
  if (currentTime - lastMeasureTime >= measureInterval) {
    lastMeasureTime = currentTime;
    
    // Step 2: Power Gating - Turn the sensor ON
    digitalWrite(PROBE_POWER_PIN, HIGH);
    
    // Wait 10ms for current conduction and analog lines to stabilize
    delay(10);
    
    // Step 3: Read the analog voltage from the probe
    int sensorValue = analogRead(PROBE_SIGNAL_PIN);
    
    // Step 4: Power Gating - Turn the sensor OFF immediately
    digitalWrite(PROBE_POWER_PIN, LOW);
    
    // Step 5: Classify moisture level
    // High ADC = High resistance = Dry soil
    // Low ADC = Low resistance = Saturated wet soil
    String soilStatus = "";
    if (sensorValue > VAL_DRY) {
      soilStatus = "DRY (Needs Watering!)";
    } else if (sensorValue < VAL_WET) {
      soilStatus = "WET (Saturated)";
    } else {
      soilStatus = "MOIST (Ideal)";
    }
    
    // Calculate volumetric moisture percentage (for visual reference)
    // 1023 (dry) mapped to 0%, 0 (wet) mapped to 100%
    float moisturePercent = map(sensorValue, 1023, 0, 0, 100);
    // Constrain to 0-100% boundaries to prevent clipping errors
    moisturePercent = constrain(moisturePercent, 0.0, 100.0);
    
    // Step 6: Log telemetry
    Serial.print("Raw ADC: ");
    Serial.print(sensorValue);
    Serial.print(" | Est. Moisture: ");
    Serial.print(moisturePercent, 1);
    Serial.print("% | Status: ");
    Serial.println(soilStatus);
  }
  
  // Non-blocking loop - background processes can run here
}

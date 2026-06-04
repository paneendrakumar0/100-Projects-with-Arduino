/*
 * 100 Projects with Arduino - Day 9
 * Project: Temperature & Humidity Logger (DHT11 Sensor Interfacing)
 * 
 * DESCRIPTION:
 * This project interfaces the DHT11 digital temperature and humidity sensor with the Arduino
 * to read environmental data. It prints the logs to the Serial Monitor at a regular interval.
 * To maintain professional coding standards:
 * 1. The reading frequency is scheduled non-blockingly using millis() to fire once every 2 seconds.
 *    (DHT11 sensors are slow; reading them faster than once every 2 seconds will cause read errors
 *    or self-heating issues).
 * 2. It implements validation checks (`isnan()`) to handle sensor disconnects gracefully.
 * 
 * THEORY OF OPERATION:
 * 1. The DHT11 contains two sensor elements: a capacitive humidity sensor and a thermistor.
 *    - Capacitive Humidity Sensor: Has two electrodes with a moisture-holding substrate between them.
 *      As water vapor is absorbed by the substrate, its dielectric constant changes, altering the
 *      electrical capacitance, which is converted to relative humidity (%RH).
 *    - Thermistor (NTC): A temperature-sensitive resistor whose resistance decreases as temperature
 *      increases.
 * 2. Built-in 8-bit Microcontroller: The DHT11 has a tiny internal chip that measures these analog values,
 *      converts them, and transmits them to the Arduino using a custom **40-bit Single-Bus Serial Protocol**.
 * 3. The 40-bit protocol transmission consists of:
 *    - 8-bit integral RH data + 8-bit decimal RH data
 *    - 8-bit integral Temp data + 8-bit decimal Temp data
 *    - 8-bit checksum (used to verify that no bits were corrupted during transmission).
 * 
 * WIRING:
 * - DHT11 VCC -> Arduino 5V
 * - DHT11 GND -> Arduino GND
 * - DHT11 DATA -> Arduino Pin 2 (Digital input)
 * Note: If using a raw 4-pin DHT11 sensor (not mounted on a 3-pin breakout board), you must
 * connect a 10k Ohm pull-up resistor between the DATA pin and VCC. Breakout modules usually
 * have this resistor pre-soldered on the board.
 * 
 * LIBRARY REQUIREMENT:
 * This code requires the "DHT sensor library" by Adafruit. 
 * Install it via the Arduino IDE Library Manager (Sketch -> Include Library -> Manage Libraries).
 */

#include "DHT.h" // Include the Adafruit DHT Sensor Library

// --- PIN DEFINITIONS ---
const int DHT_PIN = 2; // Digital pin connected to the DHT11 data pin

// --- SENSOR CONFIGURATION ---
// Define the sensor type. The library supports DHT11, DHT22 (AM2302), and DHT21 (AM2301).
#define DHTTYPE DHT11 

// Initialize the DHT sensor object
DHT dht(DHT_PIN, DHTTYPE);

// --- SCHEDULER VARIABLES ---
unsigned long lastMeasureTime = 0;       // Stores the last time we read the sensor
const unsigned long measureInterval = 2000; // Reading interval in ms (2 seconds minimum for DHT11)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 9: Temperature & Humidity Logger (DHT11)");
  Serial.println("==================================================");
  Serial.println("System Initialized. Starting sensor communication...");
  
  // Start the DHT sensor protocol
  dht.begin();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Step 1: Read the sensor at scheduled non-blocking interval (every 2 seconds)
  if (currentTime - lastMeasureTime >= measureInterval) {
    lastMeasureTime = currentTime;
    
    // Step 2: Read temperature and humidity.
    // Reading values takes about 250 milliseconds.
    float humidity = dht.readHumidity();
    
    // Read temperature as Celsius (the default)
    float tempC = dht.readTemperature();
    
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float tempF = dht.readTemperature(true);
    
    // Step 3: Validate readings.
    // If the sensor is unplugged or timing fails, read functions return "NaN" (Not a Number).
    // We must check for this to prevent mathematical errors in downstream code.
    if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
      Serial.println("[ERROR] Failed to read from DHT sensor! Check connection.");
      return; // Exit loop early to wait for next reading
    }
    
    // Step 4: Calculate Heat Index (apparent temperature reflecting humidity impact)
    float heatIndexC = dht.computeHeatIndex(tempC, humidity, false);
    float heatIndexF = dht.computeHeatIndex(tempF, humidity);
    
    // Step 5: Log telemetry
    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.print("% | Temp: ");
    Serial.print(tempC, 1);
    Serial.print(" °C (");
    Serial.print(tempF, 1);
    Serial.print(" °F) | Heat Index: ");
    Serial.print(heatIndexC, 1);
    Serial.println(" °C");
  }
  
  // Non-blocking architecture allows adding other background tasks here
}

/*
 * 100 Projects with Arduino - Day 4
 * Project: Potentiometer to LED Fade (Analog Input to PWM Output)
 *
 * DESCRIPTION:
 * This project demonstrates how to read an analog voltage from a rotary potentiometer
 * (Analog Input) and translate it to control the brightness of an LED (PWM Output).
 * To follow professional coding standards, the program does NOT use delay() to control
 * serial print speed; instead, it uses a non-blocking millis() timer for telemetry logging.
 *
 * THEORY OF OPERATION:
 * 1. The potentiometer acts as a variable voltage divider. By rotating the shaft, we adjust
 *    the voltage on the wiper pin (middle pin) between 0V and 5V.
 * 2. The Arduino's Analog-to-Digital Converter (ADC) reads this voltage on pin A0 and
 *    converts it to a 10-bit digital number: 0 (for 0V) to 1023 (for 5V).
 * 3. An LED's brightness cannot be dimmed by simply lowering DC voltage directly from the
 *    microcontroller. Instead, we use Pulse Width Modulation (PWM). By turning the LED ON
 *    and OFF rapidly (approx. 490 Hz), we control the average power delivered to it.
 *    The ratio of ON time to total period is the "Duty Cycle" (represented as 0 to 255).
 * 4. We use the map() function to scale the 10-bit ADC reading (0-1023) to an 8-bit PWM
 *    duty cycle (0-255).
 *
 * WIRING:
 * - Potentiometer Pin 1 (Outer left)  -> Arduino 5V
 * - Potentiometer Pin 2 (Middle wiper)-> Arduino Analog A0
 * - Potentiometer Pin 3 (Outer right) -> Arduino GND
 * - LED Anode (+)                     -> 220 Ohm Resistor -> Arduino Pin 9 (PWM pin)
 * - LED Cathode (-)                   -> Arduino GND
 */

// --- PIN DEFINITIONS ---
const int POT_PIN = A0;  // Analog input pin connected to the potentiometer wiper
const int LED_PIN = 9;   // Digital PWM output pin connected to the LED anode

// --- TIMING VARIABLES ---
unsigned long lastLogTime = 0;  // Stores the last time we printed to the Serial Monitor
const unsigned long logInterval =
    100;  // Telemetry logging interval in milliseconds (10 readings/sec)

void setup() {
  // Initialize Serial Monitor at 9600 bps
  Serial.begin(9600);

  // Set LED pin as output.
  // Note: We don't need to configure A0 as input; analogRead() handles this automatically.
  pinMode(LED_PIN, OUTPUT);

  Serial.println("==================================================");
  Serial.println("Day 4: Potentiometer to LED Fade (Analog Input/PWM)");
  Serial.println("==================================================");
  Serial.println("System Initialized. Turn the potentiometer shaft.");
}

void loop() {
  // Step 1: Read the raw 10-bit analog voltage from the potentiometer
  // This takes about 100 microseconds (0.0001 seconds)
  int adcValue = analogRead(POT_PIN);

  // Step 2: Map the raw ADC value (0-1023) to PWM duty cycle (0-255)
  // map(value, fromLow, fromHigh, toLow, toHigh)
  int pwmValue = map(adcValue, 0, 1023, 0, 255);

  // Step 3: Output the PWM duty cycle to the LED pin
  // This generates a square wave on Pin 9.
  analogWrite(LED_PIN, pwmValue);

  // Step 4: Non-blocking telemetry output using millis()
  // This prevents the Serial Monitor from being flooded and avoids freezing the CPU.
  unsigned long currentTime = millis();
  if (currentTime - lastLogTime >= logInterval) {
    lastLogTime = currentTime;

    // Calculate the actual voltage represented by the ADC reading (for display)
    float voltage = adcValue * (5.0 / 1023.0);

    // Calculate the duty cycle percentage
    float dutyPercent = (pwmValue / 255.0) * 100.0;

    // Print telemetry log
    Serial.print("Raw ADC: ");
    Serial.print(adcValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V | PWM Duty: ");
    Serial.print(pwmValue);
    Serial.print(" (");
    Serial.print(dutyPercent, 1);
    Serial.println("%)");
  }

  // The Arduino can run other tasks here (e.g. state machines or button checks)
  // while the LED brightness remains perfectly responsive!
}

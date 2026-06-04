/*
 * 100 Projects with Arduino - Day 89
 * Project: Precision 100 Hz Datalogging using AVR Timer1 CTC Interrupts
 * 
 * DESCRIPTION:
 * This project demonstrates how to configure and use the 16-bit hardware Timer1 on the
 * ATmega328P (Arduino Uno) in Clear Timer on Compare Match (CTC) mode. Timer interrupts
 * are critical in mechatronics for digital control loops, signal processing (DSP), 
 * and sensor datalogging where sampling jitter must be eliminated.
 * 
 * While polling with millis() or micros() has timing jitter caused by other code in loop(),
 * a hardware Timer interrupt fires with sub-microsecond precision, bypassing the main program.
 * 
 * THE MATHEMATICS OF TIMER SELECTION:
 * Clock speed (f_CPU) = 16,000,000 Hz.
 * Target sampling rate (f_target) = 100 Hz (period = 10ms).
 * Timer1 is a 16-bit counter, meaning it can count from 0 up to 65,535.
 * 
 * The formula to calculate the Compare Match Register value (OCR1A) is:
 *   OCR1A = [ f_CPU / (Prescaler * f_target) ] - 1
 * 
 * Let's evaluate potential Prescalers:
 * 1. Prescaler = 1: OCR1A = [16M / (1 * 100)] - 1 = 159,999 (Overflows 16-bit limit of 65,535)
 * 2. Prescaler = 8: OCR1A = [16M / (8 * 100)] - 1 = 19,999 (Fits perfectly in 16-bit!)
 * 
 * Therefore, we set Prescaler = 8 and OCR1A = 19,999.
 * 
 * WIRING:
 * - A0: Connect to a sensor (e.g. potentiometer or photodiode) to sample real voltage.
 * - If no sensor is connected, the sketch generates a simulated sensor signal inside the ISR.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

// --- VOLATILE VARIABLES (Shared between ISR and main loop) ---
volatile int sampledValue = 0;              // Stores the raw ADC reading
volatile unsigned long sampleTimestampUs = 0; // Timestamp of the sample in microseconds
volatile bool newSampleAvailable = false;   // Flag to signal loop() that data is ready
volatile unsigned long timerCounter = 0;    // Increments on every interrupt

// --- SIMULATION PARAMETERS ---
bool outputActive = true;
float simulatedAngle = 0.0f;

void setup() {
  Serial.begin(115200); // Use 115200 baud for fast telemetry output
  
  pinMode(A0, INPUT); // Configure analog channel
  
  Serial.println(F("=================================================="));
  Serial.println(F("Day 89: Precision 100 Hz Sampling via Timer1 CTC"));
  Serial.println(F("=================================================="));
  
  // Initialize and start Timer1
  configureTimer1_100Hz();
  
  printMenu();
}

void loop() {
  // Check if a new sample has been acquired by the Timer ISR
  if (newSampleAvailable) {
    // Copy volatile variables atomically to avoid data corruption
    int currentSample;
    unsigned long currentTimestamp;
    
    noInterrupts(); // Disable interrupts
    currentSample = sampledValue;
    currentTimestamp = sampleTimestampUs;
    newSampleAvailable = false;
    interrupts();   // Re-enable interrupts
    
    if (outputActive) {
      // Print formatted output for Serial Plotter
      // Displays Time (us), Interval (us), and Value
      static unsigned long lastTimestamp = 0;
      unsigned long interval = currentTimestamp - lastTimestamp;
      lastTimestamp = currentTimestamp;
      
      Serial.print(F("TimeUs:"));  Serial.print(currentTimestamp);
      Serial.print(F(",IntervalUs:")); Serial.print(interval);
      Serial.print(F(",SampleValue:")); Serial.println(currentSample);
    }
  }

  // Poll Serial CLI Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'p':
      case 'P':
        outputActive = !outputActive;
        Serial.print(F("[SYSTEM] Telemetry output: ")); 
        Serial.println(outputActive ? F("ON") : F("OFF"));
        break;
      case '1':
        setTimer1Frequency(50); // Change to 50 Hz
        break;
      case '2':
        setTimer1Frequency(100); // Change to 100 Hz
        break;
      case '3':
        setTimer1Frequency(200); // Change to 200 Hz
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

// =============================================================
//  AVR TIMER1 CONFIGURATION
// =============================================================

/**
 * Configures Timer1 in CTC mode to fire interrupts at exactly 100 Hz.
 */
void configureTimer1_100Hz() {
  noInterrupts(); // Disable interrupts during configuration
  
  // Reset Timer1 Control Registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; // Initialize counter value to 0
  
  // Set Compare Match Value (OCR1A)
  // OCR1A = (16,000,000 / (8 * 100)) - 1 = 19,999
  OCR1A = 19999;
  
  // Turn on CTC (Clear Timer on Compare Match) mode
  TCCR1B |= (1 << WGM12);
  
  // Set Prescaler to 8 (CS11 bit)
  TCCR1B |= (1 << CS11);
  
  // Enable Timer Compare Match A Interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  interrupts(); // Re-enable interrupts
  Serial.println(F("[TIMER] Timer1 configured at 100 Hz (Prescaler=8, OCR1A=19999)"));
}

/**
 * Dynamically changes the Timer1 CTC frequency.
 * Supported rates: 50 Hz, 100 Hz, 200 Hz (all using Prescaler = 8).
 */
void setTimer1Frequency(int freq) {
  noInterrupts();
  TCNT1 = 0; // Clear counter
  
  // Calculate compare register value
  // OCR1A = (16,000,000 / (8 * freq)) - 1
  uint16_t compareValue = (16000000UL / (8UL * freq)) - 1;
  OCR1A = compareValue;
  
  interrupts();
  
  Serial.print(F("[TIMER] Frequency changed to "));
  Serial.print(freq);
  Serial.print(F(" Hz. OCR1A set to "));
  Serial.println(compareValue);
}

// =============================================================
//  TIMER1 COMPARE MATCH ISR
// =============================================================
/**
 * ISR executed every time Timer1 counter matches OCR1A.
 * Fired at exactly the configured frequency (e.g. 100 Hz).
 */
ISR(TIMER1_COMPA_vect) {
  timerCounter++;
  
  // Capture timestamp of this trigger
  sampleTimestampUs = micros();
  
  // Read analog channel A0
  int rawADC = analogRead(A0);
  
  // If the A0 pin is floating, we inject a synthetic sine wave to make the 
  // datalogger output visually clear on the Serial Plotter.
  if (rawADC < 10 || rawADC > 1010) {
    // Generate a simulated sine wave + noise
    simulatedAngle += 0.05f;
    if (simulatedAngle >= 2.0f * PI) simulatedAngle -= 2.0f * PI;
    
    // Scale sine wave to 0-1023 analog range (centered at 512)
    float baseSignal = 512.0f + 300.0f * sin(simulatedAngle);
    
    // Simple deterministic pseudo-noise based on timer ticks
    float noise = (float)(timerCounter % 10) - 5.0f;
    
    sampledValue = (int)(baseSignal + noise);
  } else {
    sampledValue = rawADC;
  }
  
  // Flag that new sample data is ready for the loop() to process
  newSampleAvailable = true;
}

void printMenu() {
  Serial.println(F("\n--- PRECISION SAMPLING CLI ---"));
  Serial.println(F(" 'p' : Toggle printing of sample data"));
  Serial.println(F(" '1' : Change Timer1 frequency to 50 Hz  (20ms interval)"));
  Serial.println(F(" '2' : Change Timer1 frequency to 100 Hz (10ms interval)"));
  Serial.println(F(" '3' : Change Timer1 frequency to 200 Hz (5ms interval)"));
  Serial.println(F(" 'h' : Print this command list"));
  Serial.println(F("------------------------------\n"));
}

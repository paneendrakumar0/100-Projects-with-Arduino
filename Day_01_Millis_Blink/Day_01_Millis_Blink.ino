/*
 * 100 Projects with Arduino - Day 1
 * Project: Non-Blocking Blink using millis()
 *
 * INSTRUCTIONS:
 * 1. Connect your Arduino to your computer.
 * 2. Select your board and COM port in the IDE.
 * 3. Hit Upload.
 * 4. Open the Serial Monitor (magnifying glass icon in top right)
 *    and set the baud rate to 9600 to see the output!
 */

// We define our LED pin here. Pin 13 is standard for the onboard LED.
const int ledPin = 13;

// --- TIMING VARIABLES ---
// We use 'unsigned long' instead of 'int' because millis() gets very large.
// An 'int' would run out of space and crash after just 32 seconds!
unsigned long previousMillis = 0; // This remembers the last time we changed the LED
const long interval = 1000;       // The time we want to wait (1000 milliseconds = 1 second)

// This remembers if the LED is currently ON (HIGH) or OFF (LOW)
bool ledState = LOW;

void setup()
{
    // Tell the Arduino that Pin 13 is going to push electricity OUT
    pinMode(ledPin, OUTPUT);

    // Start up the Serial Monitor so the Arduino can talk to our computer
    Serial.begin(9600);
    Serial.println("Welcome to Day 1! Initializing Non-Blocking Blink...");
}

void loop()
{
    // Look at the stopwatch. What time is it right now?
    unsigned long currentMillis = millis();

    // The Logic: Current Time - Last Time We Acted >= 1 Second?
    if (currentMillis - previousMillis >= interval)
    {

        // It's time! First, reset our memory to the current time.
        previousMillis = currentMillis;

        // Flip the state. The '!' means "NOT".
        // If it is LOW, make it NOT LOW (which is HIGH).
        ledState = !ledState;

        // Actually turn the LED on or off based on the new state
        digitalWrite(ledPin, ledState);

        // Tell the user what just happened via the Serial Monitor
        if (ledState == HIGH)
        {
            Serial.println("LED is ON");
        }
        else
        {
            Serial.println("LED is OFF");
        }
    }

    // Because we didn't use delay(), the Arduino blasts right past the 'if' statement
    // and reaches this point thousands of times a second.
    // In the future, this is where we will add code to read sensors or drive motors
    // while the LED blinks perfectly in the background!
}
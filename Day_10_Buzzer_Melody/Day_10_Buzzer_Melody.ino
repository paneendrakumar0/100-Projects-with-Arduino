/*
 * 100 Projects with Arduino - Day 10
 * Project: Active vs. Passive Buzzer Melody & Rhythm Player
 * 
 * DESCRIPTION:
 * This project demonstrates how to interface buzzers with the Arduino. Because there is a
 * critical hardware difference between Active and Passive buzzers, this project is designed
 * with a Dual-Mode configuration flag:
 * 1. Active Buzzer Mode: Outputs a non-blocking rhythmic alarm cadence (using digitalWrite).
 * 2. Passive Buzzer Mode: Outputs a real frequency-pitched melody (using the tone() function).
 * 
 * HARDWARE THEORY:
 * - Active Buzzer: Contains an internal oscillator. When given a steady 5V DC, it immediately
 *   vibrates at a pre-set fixed frequency (approx 2.5 kHz). You can only control its timing
 *   (ON/OFF rhythm), not its pitch.
 * - Passive Buzzer: Does not contain an internal oscillator. It acts like a tiny speaker.
 *   It requires an AC square wave from the Arduino to flex its piezoelectric disc. By changing the
 *   frequency of the square wave, we control the pitch of the note.
 * 
 * WIRING:
 * - Buzzer Positive (+) -> 100 Ohm Resistor -> Arduino Pin 8
 * - Buzzer Negative (-) -> Arduino GND
 * Note: A 100 Ohm resistor in series with the buzzer reduces current consumption and lowers the
 * volume to a comfortable testing level.
 */

// --- CONFIGURATION: SELECT YOUR BUZZER TYPE ---
// Set to 'false' if you have an Active Buzzer (fixed pitch alarm rhythm).
// Set to 'true' if you have a Passive Buzzer (multi-tone musical melody).
#define IS_PASSIVE_BUZZER true

// --- PIN DEFINITIONS ---
const int BUZZER_PIN = 8; // Digital output pin driving the buzzer

// --- PASSIVE BUZZER MELODY DATA ---
#if IS_PASSIVE_BUZZER
// Musical Note Frequencies in Hz
#define NOTE_C4  262
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_C5  523

// Note frequencies sequence (classic "Success!" game sound)
const int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// Note durations: 4 = quarter note, 8 = eighth note, etc.
const int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

const int totalNotes = sizeof(melody) / sizeof(melody[0]);
#endif

// --- STATE VARIABLES ---
int currentStep = 0;                  // Tracks the current note or rhythm step
unsigned long stepStartTime = 0;       // Timestamp when current step started
unsigned long currentStepDuration = 0; // Calculated duration of current step

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  pinMode(BUZZER_PIN, OUTPUT);
  
  Serial.println("==================================================");
  Serial.println("Day 10: Active/Passive Buzzer Melody & Rhythm");
  Serial.println("==================================================");
  
  #if IS_PASSIVE_BUZZER
    Serial.println("Mode: PASSIVE BUZZER (Playing Pitch Melody)");
  #else
    Serial.println("Mode: ACTIVE BUZZER (Playing Rhythmic Cadence)");
  #endif
  
  // Start the first step
  startStep(0);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if the current note/rhythm duration has elapsed
  if (currentTime - stepStartTime >= currentStepDuration) {
    currentStep++;
    
    // Determine total steps based on active mode
    int maxSteps = 0;
    #if IS_PASSIVE_BUZZER
      maxSteps = totalNotes + 1; // +1 to add a pause at the end of the song
    #else
      maxSteps = 6; // Rhythmic alarm pattern has 6 steps
    #endif
    
    if (currentStep >= maxSteps) {
      currentStep = 0;
      Serial.println("--- Restarting Sequence ---");
    }
    
    startStep(currentStep);
  }
  
  // Completely non-blocking! You can read sensors or run motor control loops here.
}

/**
 * Starts a specific note or rhythmic pulse in our sequence.
 */
void startStep(int stepIndex) {
  stepStartTime = millis();
  
  #if IS_PASSIVE_BUZZER
  // --- PASSIVE BUZZER MELODY LOGIC ---
  if (stepIndex < totalNotes) {
    int note = melody[stepIndex];
    // Calculate note duration: 1000ms divided by the note type (e.g. quarter = 1000/4 = 250ms)
    currentStepDuration = 1000 / noteDurations[stepIndex];
    
    if (note == 0) {
      noTone(BUZZER_PIN);
      Serial.println("[SILENCE] Pause");
    } else {
      // tone(pin, frequency, duration)
      tone(BUZZER_PIN, note, currentStepDuration);
      Serial.print("[NOTE] Freq: ");
      Serial.print(note);
      Serial.print(" Hz | Duration: ");
      Serial.print(currentStepDuration);
      Serial.println(" ms");
    }
    // Add a 30% gap between notes to distinguish them clearly
    currentStepDuration = currentStepDuration * 1.30; 
  } 
  else {
    // Song is done. Pause for 3 seconds before looping
    noTone(BUZZER_PIN);
    currentStepDuration = 3000;
    Serial.println("[PAUSE] Ending Song Rest...");
  }
  
  #else
  // --- ACTIVE BUZZER RHYTHMIC LOGIC (Dual tone alarm beep) ---
  // We alternate ON/OFF states with different duration to create a custom alarm
  switch (stepIndex) {
    case 0:
      digitalWrite(BUZZER_PIN, HIGH);
      currentStepDuration = 150; // Quick beep
      Serial.println("[BEEP] High pulse - 150ms");
      break;
    case 1:
      digitalWrite(BUZZER_PIN, LOW);
      currentStepDuration = 100; // Silence
      break;
    case 2:
      digitalWrite(BUZZER_PIN, HIGH);
      currentStepDuration = 150; // Quick beep
      Serial.println("[BEEP] High pulse - 150ms");
      break;
    case 3:
      digitalWrite(BUZZER_PIN, LOW);
      currentStepDuration = 400; // Longer silence
      break;
    case 4:
      digitalWrite(BUZZER_PIN, HIGH);
      currentStepDuration = 600; // Long alert blast
      Serial.println("[BLAST] High pulse - 600ms");
      break;
    case 5:
      digitalWrite(BUZZER_PIN, LOW);
      currentStepDuration = 2000; // Rest before repeating
      Serial.println("[REST] Silent pause - 2000ms");
      break;
  }
  #endif
}

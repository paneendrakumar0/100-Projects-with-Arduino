/*
 * 100 Projects with Arduino - Day 48
 * Project: Maze Solving Robot (Left-Hand-On-Wall Rule & Path Optimization)
 *
 * DESCRIPTION:
 * This project implements a fully autonomous maze-solving robot using a 5-sensor IR array
 * and a 2WD differential chassis.
 * The system executes in two phases:
 * 1. Exploration Phase: The robot navigates an unknown line-based grid maze using the
 *    "Left-Hand-On-Wall" (LHR) heuristic. It detects all intersections (L-turns, R-turns,
 * T-junctions, Cross-roads, Dead-ends, and the End-of-Maze target) and records every turn decision
 * into memory.
 * 2. Optimization Phase: Upon reaching the end of the maze, the robot processes the logged path
 *    to simplify U-turns and output the shortest, most direct route to the destination.
 *
 * MAZE DECISION PRIORITY (Left-Hand Rule):
 * 1. Turn Left (if a left path is available)
 * 2. Go Straight (if no left path is available, but a straight path is)
 * 3. Turn Right (if no left or straight path is available)
 * 4. Turn Back / U-Turn (if it is a dead end)
 *
 * PATH OPTIMIZATION MATH:
 * When the robot makes a U-turn (represented by 'U'), it implies that the previous turn, the
 * U-turn, and the next turn can be combined into a single, direct path. The optimization algorithm
 * searches the path array and replaces 3-turn sequences as follows:
 *   - L U L -> S
 *   - L U S -> R
 *   - R U L -> U
 *   - S U L -> R
 *   - S U S -> U
 *   - L U R -> U
 *
 * STATE MACHINE:
 * - STATE_CALIBRATING: Pivots in place to calibrate the IR sensor array.
 * - STATE_FOLLOWING: Executes line tracking.
 * - STATE_INTERSECTION: Inches forward to analyze the junction, makes a decision, and transitions.
 * - STATE_TURNING: Pivots left, right, or around until the line is re-acquired.
 * - STATE_COMPLETED: Reaches the endpoint, halts, optimizes the path, and logs telemetry.
 *
 * WIRING:
 * - 5-Channel IR Sensor Array -> Arduino Uno
 *   - OUT1 to OUT5 -> Pins A0 to A4
 *   - VCC, GND     -> 5V, GND
 * - L298N Motor Driver -> Arduino Uno
 *   - ENA (Left PWM)   -> Pin 5
 *   - IN1, IN2         -> Pins 4, 3
 *   - ENB (Right PWM)  -> Pin 6
 *   - IN3, IN4         -> Pins 7, 8
 */

// --- PIN DEFINITIONS ---
const int NUM_SENSORS = 5;
const int SENSOR_PINS[NUM_SENSORS] = {A0, A1, A2, A3, A4};

const int L_ENA_PIN = 5;
const int L_IN1_PIN = 4;
const int L_IN2_PIN = 3;

const int R_ENB_PIN = 6;
const int R_IN3_PIN = 7;
const int R_IN4_PIN = 8;

const int LED_INDICATOR_PIN = 13;

// --- ROBOT STATE MACHINE ---
enum RobotState {
  STATE_CALIBRATING,
  STATE_FOLLOWING,
  STATE_INTERSECTION,
  STATE_TURNING,
  STATE_COMPLETED
};

RobotState currentState = STATE_CALIBRATING;

// --- SENSOR CALIBRATION LIMITS ---
int sensorMinValues[NUM_SENSORS];
int sensorMaxValues[NUM_SENSORS];

// --- PATH LOGGING & OPTIMIZATION VARIABLES ---
const int MAX_PATH_LENGTH = 100;
char pathLog[MAX_PATH_LENGTH];
int pathLength = 0;

// Turn directions
enum TurnDirection { TURN_LEFT, TURN_RIGHT, TURN_BACK, GO_STRAIGHT };
TurnDirection activeTurn = GO_STRAIGHT;

// --- TIMING & SPEED PARAMS ---
unsigned long stateTimerStart = 0;
const int BASE_CRUISE_SPEED = 120;
const int TURN_SPEED = 110;

void setup() {
  Serial.begin(9600);

  pinMode(LED_INDICATOR_PIN, OUTPUT);
  pinMode(L_ENA_PIN, OUTPUT);
  pinMode(L_IN1_PIN, OUTPUT);
  pinMode(L_IN2_PIN, OUTPUT);
  pinMode(R_ENB_PIN, OUTPUT);
  pinMode(R_IN3_PIN, OUTPUT);
  pinMode(R_IN4_PIN, OUTPUT);

  haltRobot();

  // Initialize calibration limits
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensorMinValues[i] = 1023;
    sensorMaxValues[i] = 0;
  }

  runAutoCalibration();

  currentState = STATE_FOLLOWING;
  Serial.println("[MAZE] Exploration started. Following line...");
}

void loop() {
  // Read and normalize all sensor values
  int weights[NUM_SENSORS];
  long totalWeight = 0;
  float centroidSum = 0.0;

  for (int i = 0; i < NUM_SENSORS; i++) {
    int raw = analogRead(SENSOR_PINS[i]);
    int norm = map(raw, sensorMinValues[i], sensorMaxValues[i], 0, 1000);
    norm = constrain(norm, 0, 1000);
    weights[i] = norm;
    totalWeight += norm;
    centroidSum += ((float)norm * (i - 2));  // Coordinates: -2, -1, 0, 1, 2
  }

  // Boolean flags for line detection on parts of the array
  bool leftSensorActive = (weights[0] > 600);
  bool centerSensorActive = (weights[2] > 600);
  bool rightSensorActive = (weights[4] > 600);
  bool lineDetected = (totalWeight > 400);

  switch (currentState) {
    case STATE_FOLLOWING: {
      // Check if we hit an intersection (detectable by wide lateral lines)
      if (leftSensorActive || rightSensorActive) {
        // Transition to intersection inspection
        currentState = STATE_INTERSECTION;
        stateTimerStart = millis();
        // Drive forward slightly to position the wheels over the pivot point
        driveMotors(BASE_CRUISE_SPEED, BASE_CRUISE_SPEED);
      } else if (!lineDetected) {
        // No line detected -> We hit a Dead End!
        logDecision('U');
        activeTurn = TURN_BACK;
        currentState = STATE_TURNING;
        drivePivot(TURN_LEFT);  // Always pivot left for U-turn in LHR
      } else {
        // Standard Line Following (Proportional correction)
        float offset = centroidSum / totalWeight;  // Range: -2.0 to +2.0
        int correction = (int)(offset * 45.0);
        driveMotors(BASE_CRUISE_SPEED + correction, BASE_CRUISE_SPEED - correction);
      }
      break;
    }

    case STATE_INTERSECTION: {
      // Inch forward for 120ms to check what lies ahead (beyond the initial branch detection)
      if (millis() - stateTimerStart >= 120) {
        haltRobot();

        // Re-read sensors to identify intersection type
        long checkTotalWeight = 0;
        int checkWeights[NUM_SENSORS];
        for (int i = 0; i < NUM_SENSORS; i++) {
          int raw = analogRead(SENSOR_PINS[i]);
          int norm = map(raw, sensorMinValues[i], sensorMaxValues[i], 0, 1000);
          checkWeights[i] = constrain(norm, 0, 1000);
          checkTotalWeight += checkWeights[i];
        }

        bool pathAhead = (checkWeights[2] > 500 || checkWeights[1] > 500 || checkWeights[3] > 500);
        bool pathLeft = leftSensorActive;  // From cached readings when we entered
        bool pathRight = rightSensorActive;

        // Check for End-of-Maze (e.g. solid black block, all sensors strongly triggered)
        bool allSensorsTriggered = true;
        for (int i = 0; i < NUM_SENSORS; i++) {
          if (checkWeights[i] < 700) allSensorsTriggered = false;
        }

        if (allSensorsTriggered) {
          currentState = STATE_COMPLETED;
          haltRobot();
          break;
        }

        // Apply Left-Hand Rule Priority
        if (pathLeft) {
          logDecision('L');
          activeTurn = TURN_LEFT;
          currentState = STATE_TURNING;
          drivePivot(TURN_LEFT);
        } else if (pathAhead) {
          logDecision('S');
          currentState = STATE_FOLLOWING;  // Just drive straight forward
        } else if (pathRight) {
          logDecision('R');
          activeTurn = TURN_RIGHT;
          currentState = STATE_TURNING;
          drivePivot(TURN_RIGHT);
        } else {
          // Fallback: Should not be reached unless track is corrupted, turn back
          logDecision('U');
          activeTurn = TURN_BACK;
          currentState = STATE_TURNING;
          drivePivot(TURN_LEFT);
        }
      }
      break;
    }

    case STATE_TURNING: {
      // In turning state, we pivot until the center sensor re-acquires the black line.
      // To prevent stopping immediately before the robot has rotated off the current line,
      // we ignore center sensor re-acquisition for the first 150ms of the turn.
      if (millis() - stateTimerStart > 180) {
        if (centerSensorActive) {
          haltRobot();
          currentState = STATE_FOLLOWING;
        }
      }
      break;
    }

    case STATE_COMPLETED: {
      haltRobot();
      digitalWrite(LED_INDICATOR_PIN, HIGH);
      Serial.println("[MAZE] Endpoint Reached!");
      printPath("Explored Path");

      optimizePath();
      printPath("Optimized Path");

      // Infinite halt loop
      for (;;)
        ;
    }

    default:
      break;
  }
}

// --- UTILITY PATH MANAGEMENT LOGIC ---

void logDecision(char turn) {
  if (pathLength < MAX_PATH_LENGTH) {
    pathLog[pathLength] = turn;
    pathLength++;
    Serial.print("[LOGGED]: ");
    Serial.println(turn);
  }
}

void printPath(const char* label) {
  Serial.print("[PATH] ");
  Serial.print(label);
  Serial.print(": ");
  for (int i = 0; i < pathLength; i++) {
    Serial.print(pathLog[i]);
    Serial.print(" ");
  }
  Serial.println();
}

/*
 * Optimizes the explored path list by substituting redundant U-turns.
 * Loops through the path, replacing 3-turn arrays matching:
 * Turn(i-1) + 'U' + Turn(i+1) with a single equivalent turn.
 */
void optimizePath() {
  if (pathLength < 3) return;

  bool simplified = true;
  while (simplified) {
    simplified = false;
    for (int i = 1; i < pathLength - 1; i++) {
      if (pathLog[i] == 'U') {
        char prev = pathLog[i - 1];
        char next = pathLog[i + 1];
        char replacement = ' ';

        // Rules based on coordinate turn math
        if (prev == 'L' && next == 'L')
          replacement = 'S';
        else if (prev == 'L' && next == 'S')
          replacement = 'R';
        else if (prev == 'R' && next == 'L')
          replacement = 'U';
        else if (prev == 'S' && next == 'L')
          replacement = 'R';
        else if (prev == 'S' && next == 'S')
          replacement = 'U';
        else if (prev == 'L' && next == 'R')
          replacement = 'U';

        if (replacement != ' ') {
          // Perform shift operation: replace 3 elements with 1
          pathLog[i - 1] = replacement;
          for (int j = i; j < pathLength - 2; j++) {
            pathLog[j] = pathLog[j + 2];
          }
          pathLength -= 2;
          simplified = true;
          break;  // Restart loop to find any remaining replacements
        }
      }
    }
  }
}

// --- HARDWARE INTERFACES ---

void runAutoCalibration() {
  Serial.println("[CALIBRATION] Starting. Place robot on line...");
  digitalWrite(LED_INDICATOR_PIN, HIGH);

  unsigned long start = millis();
  unsigned long lastToggle = millis();
  int pivotDirection = 1;

  // Slowly swing back and forth
  drivePivot(pivotDirection);

  while (millis() - start < 5000) {
    if (millis() - lastToggle >= 700) {
      lastToggle = millis();
      pivotDirection = -pivotDirection;
      drivePivot(pivotDirection == 1 ? TURN_RIGHT : TURN_LEFT);
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
      int val = analogRead(SENSOR_PINS[i]);
      if (val < sensorMinValues[i]) sensorMinValues[i] = val;
      if (val > sensorMaxValues[i]) sensorMaxValues[i] = val;
    }
    delay(5);
  }

  haltRobot();
  digitalWrite(LED_INDICATOR_PIN, LOW);
  Serial.println("[CALIBRATION] Load Limits Successful.");
  delay(1500);
}

void driveMotors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  if (leftSpeed >= 0) {
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, leftSpeed);
  } else {
    digitalWrite(L_IN1_PIN, LOW);
    digitalWrite(L_IN2_PIN, HIGH);
    analogWrite(L_ENA_PIN, -leftSpeed);
  }

  if (rightSpeed >= 0) {
    digitalWrite(R_IN3_PIN, HIGH);
    digitalWrite(R_IN4_PIN, LOW);
    analogWrite(R_ENB_PIN, rightSpeed);
  } else {
    digitalWrite(R_IN3_PIN, LOW);
    digitalWrite(R_IN4_PIN, HIGH);
    analogWrite(R_ENB_PIN, -rightSpeed);
  }
}

void drivePivot(TurnDirection dir) {
  if (dir == TURN_LEFT) {
    digitalWrite(L_IN1_PIN, LOW);
    digitalWrite(L_IN2_PIN, HIGH);
    analogWrite(L_ENA_PIN, TURN_SPEED);
    digitalWrite(R_IN3_PIN, HIGH);
    digitalWrite(R_IN4_PIN, LOW);
    analogWrite(R_ENB_PIN, TURN_SPEED);
  } else {  // TURN_RIGHT
    digitalWrite(L_IN1_PIN, HIGH);
    digitalWrite(L_IN2_PIN, LOW);
    analogWrite(L_ENA_PIN, TURN_SPEED);
    digitalWrite(R_IN3_PIN, LOW);
    digitalWrite(R_IN4_PIN, HIGH);
    analogWrite(R_ENB_PIN, TURN_SPEED);
  }
  stateTimerStart = millis();  // Reset stateTimer for turn timeout ignores
}

void haltRobot() {
  digitalWrite(L_IN1_PIN, LOW);
  digitalWrite(L_IN2_PIN, LOW);
  analogWrite(L_ENA_PIN, 0);

  digitalWrite(R_IN3_PIN, LOW);
  digitalWrite(R_IN4_PIN, LOW);
  analogWrite(R_ENB_PIN, 0);
}

/*
 * 100 Projects with Arduino - Day 27
 * Project: OLED Bouncing Ball Animation (2D Vector Physics & Frame Timing)
 * 
 * DESCRIPTION:
 * This project implements a real-time, 2D physics simulation of a bouncing ball on a 128x64
 * SSD1306 OLED display. To satisfy professional mechatronics and game-loop engineering standards:
 * 1. Physics Engine: Uses vector kinematics (position and velocity vectors) with float-precision.
 * 2. Collision Detection: Implements boundary checking with automatic coordinate repositioning
 *    (penetration resolution) to prevent the ball from escaping or getting stuck in borders.
 * 3. Non-Blocking Frame Rate Controller: Standardizes rendering at a target frame rate (e.g., 30 FPS)
 *    using millis() timing.
 * 4. Diagnostics Overlay: Displays dynamic coordinates and actual calculated FPS (Frames Per Second)
 *    in a dedicated UI header bar, separated from the bounce container.
 * 
 * PHYSICS THEORY:
 * - Position Vector (P) and Velocity Vector (V):
 *   At each frame, the ball's position updates by adding the velocity vector:
 *     X_new = X_old + (Vx * dt)
 *     Y_new = Y_old + (Vy * dt)
 *   In this sketch, we assume a constant time step (dt = 1 frame time), so:
 *     X += Vx
 *     Y += Vy
 * - Elastic Collision (Wall Bounce):
 *   When the ball hits a vertical wall (left/right bounds), its horizontal velocity is reversed:
 *     Vx = -Vx
 *   When it hits a horizontal wall (top/bottom bounds), its vertical velocity is reversed:
 *     Vy = -Vy
 * - Penetration Resolution:
 *   Due to discrete time steps, the ball may overlap with the wall in the frame it collides. 
 *   We reposition the ball exactly at the boundary tangent point to avoid visual clipping or infinite collision loops.
 * 
 * WIRING:
 * - OLED VCC -> Arduino 5V (or 3.3V)
 * - OLED GND -> Arduino GND
 * - OLED SDA -> Arduino SDA (A4 on Uno)
 * - OLED SCL -> Arduino SCL (A5 on Uno)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED DISPLAY DEFINITIONS ---
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- RENDERING CONFIGURATION ---
const int TARGET_FPS = 30;
const unsigned long FRAME_DELAY_MS = 1000 / TARGET_FPS; // ~33ms per frame
unsigned long lastFrameTime = 0;

// --- DIAGNOSTICS & PERFORMANCE VARIABLES ---
unsigned long fpsCounterStart = 0;
int frameCount = 0;
float calculatedFPS = 0.0;

// --- 2D PHYSICS SIMULATION VARIABLES ---
// Define boundaries for the bounce container (to leave space for the top header)
const int PLAY_X_MIN = 0;
const int PLAY_X_MAX = 127;
const int PLAY_Y_MIN = 12; // Top 12 pixels reserved for header
const int PLAY_Y_MAX = 63;

// Ball state variables
float ballX = 64.0;       // Horizontal position (center of ball)
float ballY = 36.0;       // Vertical position (center of ball)
float ballVx = 2.2;       // Horizontal velocity component (pixels per frame)
float ballVy = 1.6;       // Vertical velocity component (pixels per frame)
const int ballRadius = 3; // Ball radius in pixels

void setup() {
  Serial.begin(9600);
  
  Serial.println("==================================================");
  Serial.println("Day 27: OLED 2D Bouncing Ball Physics Simulation");
  Serial.println("==================================================");

  // Initialize SSD1306 OLED screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("[ERROR] SSD1306 allocation failed. Check connections.");
    for (;;); // Halt execution
  }

  display.clearDisplay();
  display.display();
  
  lastFrameTime = millis();
  fpsCounterStart = millis();
  Serial.println("Simulation running. Outputting frames to OLED...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Non-blocking frame timing loop
  if (currentTime - lastFrameTime >= FRAME_DELAY_MS) {
    lastFrameTime = currentTime;
    
    // 1. Update physics states (Kinematics & Collision resolution)
    updatePhysics();
    
    // 2. Clear local buffer and redraw screen elements
    display.clearDisplay();
    drawUIHeader();
    drawBounceBoundary();
    drawBall();
    
    // 3. Push framebuffer to physical OLED panel
    display.display();
    
    // 4. Calculate actual FPS (Frames Per Second)
    frameCount++;
    unsigned long elapsed = currentTime - fpsCounterStart;
    if (elapsed >= 1000) { // Update FPS calculation once per second
      calculatedFPS = (float)frameCount * 1000.0 / elapsed;
      frameCount = 0;
      fpsCounterStart = currentTime;
      
      // Print diagnostics to Serial Monitor
      Serial.print("[PHYSICS] PosX: ");
      Serial.print(ballX, 1);
      Serial.print(" | PosY: ");
      Serial.print(ballY, 1);
      Serial.print(" | FPS: ");
      Serial.println(calculatedFPS, 1);
    }
  }
  
  // Outer loop remains non-blocking for other microcontroller background tasks
}

/**
 * Updates position vectors and resolves elastic wall collisions.
 */
void updatePhysics() {
  // Step 1: Apply translation vector (X = X + Vx, Y = Y + Vy)
  ballX += ballVx;
  ballY += ballVy;
  
  // Step 2: Resolve Horizontal Collisions (Left & Right boundaries)
  // Left wall boundary
  if (ballX - ballRadius <= PLAY_X_MIN) {
    ballVx = -ballVx;                 // Reverse direction vector
    ballX = PLAY_X_MIN + ballRadius;  // Penetration resolution (position clamp)
  } 
  // Right wall boundary
  else if (ballX + ballRadius >= PLAY_X_MAX) {
    ballVx = -ballVx;
    ballX = PLAY_X_MAX - ballRadius;
  }
  
  // Step 3: Resolve Vertical Collisions (Top & Bottom boundaries of play area)
  // Top boundary
  if (ballY - ballRadius <= PLAY_Y_MIN) {
    ballVy = -ballVy;
    ballY = PLAY_Y_MIN + ballRadius;
  } 
  // Bottom boundary
  else if (ballY + ballRadius >= PLAY_Y_MAX) {
    ballVy = -ballVy;
    ballY = PLAY_Y_MAX - ballRadius;
  }
}

/**
 * Draws the top dashboard header containing system stats.
 */
void drawUIHeader() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Display FPS
  display.setCursor(4, 2);
  display.print("FPS:");
  display.print((int)calculatedFPS);
  
  // Display Position
  display.setCursor(55, 2);
  display.print("X:");
  display.print((int)ballX);
  display.print(" Y:");
  display.print((int)ballY);
  
  // Draw a solid divider line separating header from bounce area
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

/**
 * Draws the boundary box of the bounce container.
 */
void drawBounceBoundary() {
  // Outer rectangle for the play field (pixels 11 to 63)
  display.drawRect(PLAY_X_MIN, PLAY_Y_MIN - 1, SCREEN_WIDTH, PLAY_Y_MAX - PLAY_Y_MIN + 2, SSD1306_WHITE);
}

/**
 * Renders the ball vector onto the display.
 */
void drawBall() {
  display.fillCircle((int)ballX, (int)ballY, ballRadius, SSD1306_WHITE);
}

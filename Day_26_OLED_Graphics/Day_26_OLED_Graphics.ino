/*
 * 100 Projects with Arduino - Day 26
 * Project: SSD1306 OLED Display Graphics (Vector Drawing & Framebuffers)
 *
 * DESCRIPTION:
 * This project interfaces a 0.96" 128x64 pixel OLED display driven by the SSD1306 chip.
 * To meet professional mechatronics standards, this sketch:
 * 1. Implements a non-blocking Scene Scheduler using millis() to cycle through three distinct
 *    visual layouts (Text & Frame, Geometric Vectors, Coordinate Grid) every 3 seconds.
 * 2. Demonstrates the coordinate math and API calls of the industry-standard Adafruit GFX engine.
 * 3. Incorporates clean buffer flushing logic (`display.display()`) to optimize serial bus writes.
 *
 * THEORY OF OPERATION:
 * 1. OLED (Organic Light Emitting Diode): Unlike LCDs (Day 21) which require a separate backlight
 *    shining through liquid crystal gates, OLED pixels are self-luminous. Each individual pixel
 *    (an organic compound film) glows when given current. This yields absolute black levels
 * (infinite contrast), ultra-wide viewing angles, and extremely low power consumption.
 * 2. SSD1306 Driver and I2C Framebuffer:
 *    The OLED has a resolution of 128x64 = 8192 pixels. To control them, the SSD1306 chip uses an
 * internal graphic RAM. Because microcontrollers have limited RAM, the Adafruit library allocates a
 * local 1024-byte framebuffer in the Arduino's SRAM (128x64 bits = 8192 bits = 1024 bytes). All
 * drawing functions (drawCircle, print, etc.) write to this local SRAM buffer. The
 * `display.display()` call then transmits the entire 1kB buffer over the I2C bus at high speed,
 * updating the screen.
 *
 * WIRING:
 * - OLED VCC -> Arduino 5V (or 3.3V, check your module!)
 * - OLED GND -> Arduino GND
 * - OLED SDA -> Arduino SDA (A4 on Uno)
 * - OLED SCL -> Arduino SCL (A5 on Uno)
 *
 * LIBRARY REQUIREMENT:
 * This code requires these libraries by Adafruit:
 * 1. "Adafruit SSD1306" (display driver)
 * 2. "Adafruit GFX Library" (core graphics engine)
 * Install both via the Arduino Library Manager.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// --- OLED DISPLAY DEFINITIONS ---
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin (set to -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // Standard I2C address for 0.96" OLEDs (0x3C or 0x3D)

// Instantiate the display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- SCENE SCHEDULER VARIABLES ---
int currentScene = 0;                      // Index of the active graphics scene (0 - 2)
unsigned long lastSceneChange = 0;         // Timestamp of the last scene transition
const unsigned long sceneDuration = 3000;  // Duration of each scene in ms (3 seconds)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  Serial.println("==================================================");
  Serial.println("Day 26: SSD1306 OLED Display Vector Graphics");
  Serial.println("==================================================");

  // Initialize the OLED screen. SSD1306_SWITCHCAPVCC generates 7.5V-9V internally for OLED drive.
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(("[ERROR] SSD1306 allocation failed. Check wiring!"));
    for (;;);  // Loop forever if display initialization fails
  }

  // Clear the library's default startup splash screen buffer
  display.clearDisplay();
  display.display();

  Serial.println("OLED Initialized successfully. Cycling scenes...");
}

void loop() {
  unsigned long currentTime = millis();

  // Step 1: Check scene timer and cycle scenes non-blockingly
  if (currentTime - lastSceneChange >= sceneDuration) {
    lastSceneChange = currentTime;

    currentScene++;
    if (currentScene > 2) {
      currentScene = 0;
    }

    // Clear buffer for the new scene
    display.clearDisplay();

    // Render the selected scene
    switch (currentScene) {
      case 0:
        drawTextScene();
        break;
      case 1:
        drawVectorGeometryScene();
        break;
      case 2:
        drawCoordinateGridScene();
        break;
    }

    // Step 2: Flush local SRAM framebuffer to the physical OLED panel over I2C
    display.display();

    Serial.print("[OLED] Transitioned to Scene: ");
    Serial.println(currentScene);
  }

  // Loop is non-blocking. Additional mechatronics loops can execute here.
}

/**
 * SCENE 0: Renders formatted text and border frames.
 */
void drawTextScene() {
  // Draw an outer boundary rectangle
  // drawRect(x, y, width, height, color)
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  // Draw a second inner boundary rectangle
  display.drawRect(3, 3, SCREEN_WIDTH - 6, SCREEN_HEIGHT - 6, SSD1306_WHITE);

  // Set text size (1 = 5x7 pixel font, 2 = 10x14, etc.)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // setCursor(x, y) - coordinate (0,0) is top-left corner
  display.setCursor(15, 12);
  display.print("SSD1306 OLED DRV");

  display.setTextSize(2);
  display.setCursor(15, 26);
  display.print("DAY 26");

  display.setTextSize(1);
  display.setCursor(15, 46);
  display.print("Vector Graphics");
}

/**
 * SCENE 1: Renders geometric vector shapes (lines, circles, triangles, filled blocks).
 */
void drawVectorGeometryScene() {
  // Draw overlapping geometric shapes
  // 1. Draw a circle: drawCircle(centerX, centerY, radius, color)
  display.drawCircle(32, 32, 20, SSD1306_WHITE);

  // 2. Draw a filled circle: fillCircle(centerX, centerY, radius, color)
  display.fillCircle(32, 32, 8, SSD1306_WHITE);

  // 3. Draw a triangle: drawTriangle(x0, y0, x1, y1, x2, y2, color)
  display.drawTriangle(80, 12, 110, 52, 70, 52, SSD1306_WHITE);

  // 4. Draw crossing diagonal lines
  display.drawLine(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, 0, SSD1306_WHITE);
}

/**
 * SCENE 2: Renders a coordinate system grid (useful for mapping sensor logs or trajectories).
 */
void drawCoordinateGridScene() {
  // Draw vertical axis (X = 64)
  display.drawLine(64, 0, 64, SCREEN_HEIGHT, SSD1306_WHITE);

  // Draw horizontal axis (Y = 32)
  display.drawLine(0, 32, SCREEN_WIDTH, 32, SSD1306_WHITE);

  // Draw grid ticks
  for (int x = 0; x < SCREEN_WIDTH; x += 16) {
    display.drawLine(x, 30, x, 34, SSD1306_WHITE);  // X-axis ticks
  }
  for (int y = 0; y < SCREEN_HEIGHT; y += 8) {
    display.drawLine(62, y, 66, y, SSD1306_WHITE);  // Y-axis ticks
  }

  // Print axis labels
  display.setTextSize(1);
  display.setCursor(70, 4);
  display.print("+Y");
  display.setCursor(70, 52);
  display.print("-Y");
  display.setCursor(4, 22);
  display.print("-X");
  display.setCursor(110, 22);
  display.print("+X");
}

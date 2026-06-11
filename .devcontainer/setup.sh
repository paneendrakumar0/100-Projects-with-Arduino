#!/bin/bash
set -e

echo "Installing Arduino CLI..."
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sudo BINDIR=/usr/local/bin sh

echo "Initializing Arduino core..."
arduino-cli core update-index

echo "Installing AVR Core..."
arduino-cli core install arduino:avr

echo "Installing required Arduino libraries..."
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" "FreeRTOS" "HID-Project" "IRremote" "Joystick" "Keypad" "LiquidCrystal I2C" "MFRC522" "RF24" "TM1637"

echo "Installing docs-website dependencies..."
if [ -d "docs-website" ]; then
  cd docs-website
  npm install
  cd ..
fi

echo "Setup complete!"

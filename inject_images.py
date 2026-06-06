import os
import glob

# Map of component keyword to image filename
COMPONENTS = {
    "Arduino Uno": "Arduino_Uno.jpg",
    "Arduino Nano": "Arduino_Nano.jpg",
    "Arduino Mega": "Arduino_Mega.jpg",
    "Breadboard": "Breadboard.jpg",
    "Jumper Wires": "Jumper_Wires.jpg",
    "LED": "LED.jpg",
    "Resistor": "Resistor.jpg",
    "Photoresistor": "Photoresistor.jpg",
    "LDR": "Photoresistor.jpg",
    "Ultrasonic Sensor": "Ultrasonic_Sensor.jpg",
    "HC-SR04": "Ultrasonic_Sensor.jpg",
    "PIR Motion Sensor": "PIR_Motion_Sensor.jpg",
    "DHT11": "DHT11.jpg",
    "DHT22": "DHT11.jpg",
    "Servo Motor": "Servo_Motor.jpg",
    "SG90": "Servo_Motor.jpg",
    "Potentiometer": "Potentiometer.jpg",
    "Joystick": "Joystick.jpg",
    "Relay Module": "Relay_Module.jpg",
    "Relay": "Relay_Module.jpg",
    "Soil Moisture": "Soil_Moisture.jpg",
    "LCD 16x2": "LCD_16x2.jpg",
    "7-Segment Display": "7-Segment_Display.jpg",
    "OLED Display": "OLED_Display.jpg",
    "SSD1306": "OLED_Display.jpg",
    "Infrared Remote": "Infrared_Remote.jpg",
    "EEPROM": "EEPROM.jpg",
    "RFID": "RFID.jpg",
    "MFRC522": "RFID.jpg",
    "Rotary Encoder": "Rotary_Encoder.jpg",
    "DC Motor": "DC_Motor.jpg",
    "Stepper Motor": "Stepper_Motor.jpg",
    "Bluetooth Module": "Bluetooth_Module.jpg",
    "HC-05": "Bluetooth_Module.jpg",
    "MPU6050": "MPU6050.jpg",
    "GPS Module": "GPS_Module.jpg",
    "NEO-6M": "GPS_Module.jpg",
    "Modbus RS485": "Modbus_RS485.jpg",
    "CAN Bus": "CAN_Bus.jpg",
    "WS2812B": "WS2812B.jpg",
    "Oscilloscope": "Oscilloscope.jpg",
    "Kalman Filter": "Kalman_Filter.jpg",
    "Robotic Arm": "Robotic_Arm.jpg",
    "Differential Drive": "Differential_Drive.jpg"
}

def inject_gallery_into_readme(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Skip if already injected
    if "📸 Component Visuals" in content:
        return False

    # Determine which components are mentioned
    found_components = set()
    content_upper = content.upper()
    
    for key, filename in COMPONENTS.items():
        # Only inject if the image actually exists in our assets folder
        img_path = os.path.join("assets", "images", "components", filename)
        if os.path.exists(img_path):
            if key.upper() in content_upper:
                found_components.add(filename)

    if not found_components:
        return False

    # Build the HTML gallery
    gallery_html = "\n## 📸 Component Visuals\n\n<p align=\"center\">\n"
    for filename in sorted(found_components):
        # We assume the README is 1 folder deep (e.g. Day_01/README.md)
        rel_path = f"../assets/images/components/{filename}"
        
        # If the file is in root (e.g. Learn_Arduino is 1 deep, ARDUINO_CORE_THEORY is 0 deep)
        if filepath == "ARDUINO_CORE_THEORY.md" or filepath == "README.md":
            rel_path = f"assets/images/components/{filename}"
            
        name_alt = filename.replace('.jpg', '').replace('_', ' ')
        gallery_html += f"  <img src=\"{rel_path}\" alt=\"{name_alt}\" width=\"200\" style=\"margin:10px;\" />\n"
    gallery_html += "</p>\n"

    # Inject the gallery right before the first standard '##' section or just append to end of description
    # Most Day READMEs have "## " as their first section after title.
    lines = content.split('\n')
    new_lines = []
    injected = False
    
    for i, line in enumerate(lines):
        if line.startswith('## ') and not injected:
            # Found the first sub-header. Inject gallery here.
            new_lines.extend(gallery_html.split('\n'))
            injected = True
        new_lines.append(line)

    if not injected:
        # Fallback if no sub-headers exist
        new_lines.extend(gallery_html.split('\n'))

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write('\n'.join(new_lines))
    
    print(f"Injected {len(found_components)} images into {filepath}")
    return True

# 1. Inject into 100 Days READMEs
day_readmes = glob.glob("Day_*/README.md")
for f in day_readmes:
    inject_gallery_into_readme(f)

# 2. Inject into Learn_Arduino Guide
if os.path.exists("Learn_Arduino/README.md"):
    inject_gallery_into_readme("Learn_Arduino/README.md")

# 3. Inject into Theory Guide
if os.path.exists("ARDUINO_CORE_THEORY.md"):
    inject_gallery_into_readme("ARDUINO_CORE_THEORY.md")

print("Image injection complete.")

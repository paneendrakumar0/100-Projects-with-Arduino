import os
import requests
import json
import time

ASSETS_DIR = "assets/images/components"
os.makedirs(ASSETS_DIR, exist_ok=True)

HEADERS = {
    'User-Agent': 'ArduinoLearningBot/1.0 (https://github.com/paneendrakumar0/100-Projects-with-Arduino)'
}

# Curated list of Wikipedia page titles to extract primary images.
COMPONENTS = {
    "Arduino Uno": "Arduino_Uno",
    "Arduino Nano": "Arduino", 
    "Arduino Mega": "Arduino",
    "Breadboard": "Breadboard",
    "Jumper Wires": "Jump_wire",
    "LED": "Light-emitting_diode",
    "Resistor": "Resistor",
    "Photoresistor": "Photoresistor",
    "Ultrasonic Sensor": "Sonar",
    "PIR Motion Sensor": "Passive_infrared_sensor",
    "DHT11": "Thermometer",
    "Servo Motor": "Servomotor",
    "Potentiometer": "Potentiometer",
    "Joystick": "Joystick",
    "Relay Module": "Relay",
    "Soil Moisture": "Soil_moisture_sensor",
    "LCD 16x2": "Liquid-crystal_display",
    "7-Segment Display": "Seven-segment_display",
    "OLED Display": "OLED",
    "Infrared Remote": "Remote_control",
    "EEPROM": "EEPROM",
    "RFID": "Radio-frequency_identification",
    "Rotary Encoder": "Rotary_encoder",
    "DC Motor": "DC_motor",
    "Stepper Motor": "Stepper_motor",
    "Bluetooth Module": "Bluetooth",
    "MPU6050": "Inertial_measurement_unit",
    "GPS Module": "Global_Positioning_System",
    "Modbus RS485": "Modbus",
    "CAN Bus": "CAN_bus",
    "WS2812B": "LED_strip_light",
    "Oscilloscope": "Oscilloscope",
    "Kalman Filter": "Kalman_filter",
    "Robotic Arm": "Robotic_arm",
    "Differential Drive": "Differential_wheeled_robot"
}

def download_image(name, search_title):
    filepath = os.path.join(ASSETS_DIR, f"{name.replace(' ', '_')}.jpg")
    if os.path.exists(filepath):
        print(f"Skipping {name}, already exists.")
        return
        
    url = f"https://en.wikipedia.org/w/api.php?action=query&titles={search_title}&prop=pageimages&format=json&pithumbsize=600"
    
    try:
        response = requests.get(url, headers=HEADERS)
        if response.status_code == 429:
            print("Rate limited. Sleeping 5 seconds...")
            time.sleep(5)
            response = requests.get(url, headers=HEADERS)

        data = response.json()
        pages = data.get('query', {}).get('pages', {})
        
        image_url = None
        for page_id, page_info in pages.items():
            if 'thumbnail' in page_info:
                image_url = page_info['thumbnail']['source']
                break
                
        if image_url:
            print(f"Downloading {name} from {image_url}...")
            img_response = requests.get(image_url, headers=HEADERS)
            if img_response.status_code == 200:
                with open(filepath, 'wb') as f:
                    f.write(img_response.content)
            else:
                print(f"ERROR downloading image file {name}: Status {img_response.status_code}")
        else:
            print(f"WARNING: No image found for {name} ({search_title})")
            
    except Exception as e:
        print(f"ERROR downloading {name}: {e}")

    # Sleep to respect rate limits
    time.sleep(1.0)

print("Starting component image downloads...")
for name, title in COMPONENTS.items():
    download_image(name, title)
print("Downloads complete.")

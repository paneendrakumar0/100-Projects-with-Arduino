import urllib.request
import urllib.parse
import json
import os

images = {
    "Arduino_Uno": "https://upload.wikimedia.org/wikipedia/commons/thumb/a/a6/Arduino_Uno_V3.png/800px-Arduino_Uno_V3.png",
    "Arduino_Nano": "https://upload.wikimedia.org/wikipedia/commons/thumb/c/c2/Arduino_Nano.jpg/800px-Arduino_Nano.jpg",
    "Arduino_Mega": "https://upload.wikimedia.org/wikipedia/commons/thumb/a/ae/Arduino_Mega2560_v3.0.jpg/800px-Arduino_Mega2560_v3.0.jpg",
    "Arduino_Leonardo": "https://upload.wikimedia.org/wikipedia/commons/thumb/b/b8/Arduino_Leonardo.jpg/800px-Arduino_Leonardo.jpg",
    "ESP32": "https://upload.wikimedia.org/wikipedia/commons/thumb/1/1a/ESP32_Devkit_V1.jpg/800px-ESP32_Devkit_V1.jpg",
    "Jumper_Wires": "https://upload.wikimedia.org/wikipedia/commons/thumb/b/ba/Jumper_Wires.jpg/800px-Jumper_Wires.jpg",
    "Potentiometer": "https://upload.wikimedia.org/wikipedia/commons/thumb/b/b5/Potentiometer.jpg/800px-Potentiometer.jpg",
    "Photoresistor": "https://upload.wikimedia.org/wikipedia/commons/thumb/a/a9/LDR_photoresistor.jpg/800px-LDR_photoresistor.jpg",
    "Ultrasonic": "https://upload.wikimedia.org/wikipedia/commons/thumb/7/70/HC-SR04_Ultrasonic_sensor_module.jpg/800px-HC-SR04_Ultrasonic_sensor_module.jpg",
    "Breadboard": "https://upload.wikimedia.org/wikipedia/commons/thumb/b/b3/Breadboard_scheme.svg/800px-Breadboard_scheme.svg.png",
    "LED": "https://upload.wikimedia.org/wikipedia/commons/thumb/3/35/5mm_Red_LED.jpg/800px-5mm_Red_LED.jpg",
    "Resistor": "https://upload.wikimedia.org/wikipedia/commons/thumb/0/05/Resistor_CR25.jpg/800px-Resistor_CR25.jpg"
}

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for name, url in images.items():
    file_path = os.path.join(output_dir, f"{name}.png" if ".png" in url else f"{name}.jpg")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response, open(file_path, 'wb') as out_file:
            out_file.write(response.read())
        print(f"Downloaded: {name}")
    except Exception as e:
        print(f"Failed {name}: {e}")

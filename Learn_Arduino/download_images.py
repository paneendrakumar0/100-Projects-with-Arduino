import urllib.request
import json
import os
import shutil

images_to_download = {
    "Arduino_Uno": "https://upload.wikimedia.org/wikipedia/commons/3/3b/Arduino_Uno_-_R3.jpg",
    "Arduino_Nano": "https://upload.wikimedia.org/wikipedia/commons/thumb/c/c2/Arduino_Nano.jpg/800px-Arduino_Nano.jpg",
    "Arduino_Mega": "https://upload.wikimedia.org/wikipedia/commons/thumb/a/ae/Arduino_Mega2560_v3.0.jpg/800px-Arduino_Mega2560_v3.0.jpg",
    "ESP32": "https://upload.wikimedia.org/wikipedia/commons/thumb/1/1a/ESP32_Devkit_V1.jpg/800px-ESP32_Devkit_V1.jpg",
    "Breadboard": "https://upload.wikimedia.org/wikipedia/commons/thumb/b/b3/Breadboard_scheme.svg/800px-Breadboard_scheme.svg.png",
    "LED": "https://upload.wikimedia.org/wikipedia/commons/thumb/3/35/5mm_Red_LED.jpg/800px-5mm_Red_LED.jpg",
    "Resistor": "https://upload.wikimedia.org/wikipedia/commons/thumb/0/05/Resistor_CR25.jpg/800px-Resistor_CR25.jpg"
}

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for name, url in images_to_download.items():
    ext = url.split('.')[-1]
    if "?" in ext:
        ext = ext.split("?")[0]
    file_path = os.path.join(output_dir, f"{name}.{ext}")
    print(f"Downloading {name} from {url} to {file_path}...")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response, open(file_path, 'wb') as out_file:
            shutil.copyfileobj(response, out_file)
        print(f"Success: {name}")
    except Exception as e:
        print(f"Failed to download {name}: {e}")


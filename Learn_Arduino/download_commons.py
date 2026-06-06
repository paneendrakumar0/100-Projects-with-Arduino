import urllib.request
import urllib.parse
import json
import os
import shutil

files = {
    "Arduino_Uno": "File:Arduino Uno - R3.jpg",
    "Arduino_Nano": "File:Arduino Nano.jpg",
    "Arduino_Mega": "File:Arduino Mega2560 v3.0.jpg",
    "Arduino_Leonardo": "File:Arduino Leonardo.jpg",
    "ESP32": "File:ESP32 Devkit V1.jpg",
    "Jumper_Wires": "File:Jumper Wires.jpg",
    "Potentiometer": "File:Potentiometer.jpg",
    "Photoresistor": "File:LDR photoresistor.jpg",
    "Ultrasonic": "File:HC-SR04 Ultrasonic sensor module.jpg",
    "Breadboard": "File:Breadboard scheme.svg",
    "LED": "File:5mm Red LED.jpg",
    "Resistor": "File:Resistor CR25.jpg",
    "Arduino_IDE": "File:Arduino IDE 2.0.0.png"
}

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for name, filename in files.items():
    print(f"Fetching {name}...")
    try:
        url = f"https://commons.wikimedia.org/w/api.php?action=query&titles={urllib.parse.quote(filename)}&prop=imageinfo&iiprop=url&iiurlwidth=800&format=json"
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as res:
            data = json.loads(res.read().decode())
            pages = data['query']['pages']
            page = list(pages.values())[0]
            
            if 'imageinfo' in page:
                info = page['imageinfo'][0]
                img_url = info.get('thumburl', info['url'])
                
                ext = img_url.split('.')[-1]
                if "?" in ext:
                    ext = ext.split("?")[0]
                if len(ext) > 4:
                    ext = "jpg"
                
                file_path = os.path.join(output_dir, f"{name}.{ext}")
                print(f"Downloading {name} from {img_url} to {file_path}...")
                
                img_req = urllib.request.Request(img_url, headers={'User-Agent': 'Mozilla/5.0'})
                with urllib.request.urlopen(img_req) as i, open(file_path, 'wb') as o:
                    shutil.copyfileobj(i, o)
                print(f"Success: {name}")
            else:
                print(f"No imageinfo found for {filename}")
    except Exception as e:
        print(f"Failed {name}: {e}")

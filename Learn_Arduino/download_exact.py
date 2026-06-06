import urllib.request
import os

images = {
    "Arduino_Mega": "https://docs.arduino.cc/static/a544bbfecf48202b8d96d246cc379cfc/Mega2560_0.jpg",
    "Jumper_Wires": "https://cdn.sparkfun.com//assets/parts/8/5/3/9/12796-01.jpg",
    "Arduino_IDE": "https://docs.arduino.cc/static/6b2253086ebc8bb0cefc76527b140bc8/ide-v2-main.png",
    "Serial_Monitor": "https://docs.arduino.cc/static/23e9c5665c5898d9cc9bb531fbca8ff9/serial-monitor-icon.png",
    "DHT11": "https://cdn-shop.adafruit.com/1200x900/386-00.jpg",
    "Pushbutton": "https://cdn-shop.adafruit.com/1200x900/1009-00.jpg"
}

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for name, url in images.items():
    file_path = os.path.join(output_dir, f"{name}.png" if url.endswith(".png") else f"{name}.jpg")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response, open(file_path, 'wb') as out_file:
            out_file.write(response.read())
        print(f"Downloaded: {name}")
    except Exception as e:
        print(f"Failed {name}: {e}")

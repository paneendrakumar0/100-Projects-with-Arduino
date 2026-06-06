import urllib.request
import json
import os
import shutil

articles = {
    "Arduino_Uno": "Arduino",
    "Arduino_Nano": "Arduino Nano",
    "Breadboard": "Breadboard",
    "LED": "Light-emitting diode",
    "Resistor": "Resistor",
    "Arduino_IDE": "Arduino IDE"
}

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for name, title in articles.items():
    print(f"Fetching image for {title}...")
    try:
        url = f"https://en.wikipedia.org/w/api.php?action=query&titles={urllib.parse.quote(title)}&prop=pageimages&format=json&pithumbsize=800"
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) LearnArduinoBot/1.0'})
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            pages = data['query']['pages']
            page = list(pages.values())[0]
            if 'thumbnail' in page:
                img_url = page['thumbnail']['source']
                ext = img_url.split('.')[-1]
                if "?" in ext:
                    ext = ext.split("?")[0]
                if len(ext) > 4:
                    ext = "jpg"
                file_path = os.path.join(output_dir, f"{name}.{ext}")
                print(f"Downloading {name} from {img_url} to {file_path}...")
                
                img_req = urllib.request.Request(img_url, headers={'User-Agent': 'Mozilla/5.0'})
                with urllib.request.urlopen(img_req) as img_res, open(file_path, 'wb') as out_file:
                    shutil.copyfileobj(img_res, out_file)
                print(f"Success: {name}")
            else:
                print(f"No image found for {title}")
    except Exception as e:
        print(f"Failed to fetch {name}: {e}")

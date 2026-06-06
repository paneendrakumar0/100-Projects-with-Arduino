import os
import urllib.request
from duckduckgo_search import DDGS

search_terms = {
    "Arduino_Uno": "Arduino Uno R3 top down clear white background",
    "Arduino_Nano": "Arduino Nano board top view",
    "Arduino_Mega": "Arduino Mega 2560 board top view",
    "ESP32": "ESP32 Devkit V1 board top view",
    "Breadboard": "solderless breadboard top view",
    "LED": "5mm Red LED component clear",
    "Resistor": "resistor clear background",
    "IDE": "Arduino IDE 2.x interface screenshot"
}

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

ddgs = DDGS()

for name, query in search_terms.items():
    print(f"Searching for {name}...")
    try:
        results = ddgs.images(query, max_results=1)
        if results:
            url = results[0]['image']
            ext = url.split('.')[-1]
            if "?" in ext:
                ext = ext.split("?")[0]
            if len(ext) > 4:
                ext = "jpg"
            file_path = os.path.join(output_dir, f"{name}.{ext}")
            print(f"Downloading {name} from {url} to {file_path}...")
            
            req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req, timeout=10) as response, open(file_path, 'wb') as out_file:
                out_file.write(response.read())
            print(f"Success: {name}")
        else:
            print(f"No results for {name}")
    except Exception as e:
        print(f"Failed to download {name}: {e}")

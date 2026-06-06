import urllib.request
import urllib.parse
import json
import re
import time

components = {
    "PIR_Sensor": "PIR sensor",
    "IR_Receiver": "IR receiver",
    "Buzzer": "Piezo buzzer",
    "Servo_Motor": "Servomotor",
    "LCD_16x2": "16x2 LCD",
    "Membrane_Keypad": "Membrane keypad",
    "Relay_Module": "Relay",
    "OLED_Display": "OLED display",
    "Seven_Segment": "Seven-segment display",
    "RFID_Reader": "RFID reader",
    "Rotary_Encoder": "Rotary encoder",
    "L298N_Motor_Driver": "H-bridge",
    "DC_Motor": "DC motor",
    "Stepper_Motor": "Stepper motor",
    "Bluetooth_Module": "Bluetooth module",
    "GPS_Module": "GPS receiver",
    "MPU6050": "MPU6050",
    "MicroSD_Module": "MicroSD",
    "Battery_Holder": "Battery holder",
    "Joystick_Module": "Analog stick"
}

with open("Chapter_11_Component_Glossary.md", "r", encoding="utf-8") as f:
    content = f.read()

for name, query in components.items():
    print(f"Fetching URL for {query}...")
    try:
        search_url = f"https://commons.wikimedia.org/w/api.php?action=query&list=search&srsearch=File:{urllib.parse.quote(query)}&srnamespace=6&format=json"
        req = urllib.request.Request(search_url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as res:
            data = json.loads(res.read().decode())
            search_results = data['query']['search']
            if search_results:
                file_title = search_results[0]['title']
                info_url = f"https://commons.wikimedia.org/w/api.php?action=query&titles={urllib.parse.quote(file_title)}&prop=imageinfo&iiprop=url&iiurlwidth=800&format=json"
                info_req = urllib.request.Request(info_url, headers={'User-Agent': 'Mozilla/5.0'})
                with urllib.request.urlopen(info_req) as info_res:
                    info_data = json.loads(info_res.read().decode())
                    page = list(info_data['query']['pages'].values())[0]
                    if 'imageinfo' in page:
                        img_info = page['imageinfo'][0]
                        img_url = img_info.get('thumburl', img_info['url'])
                        # Replace in content
                        pattern = r'!\[(.*?)\]\(\./images/' + name + r'\.(jpg|png)\)'
                        replacement = r'![\1](' + img_url + ')'
                        content = re.sub(pattern, replacement, content)
                        print(f"Successfully replaced {name} with {img_url}")
        time.sleep(1) # Avoid 429
    except Exception as e:
        print(f"Failed {name}: {e}")

with open("Chapter_11_Component_Glossary.md", "w", encoding="utf-8") as f:
    f.write(content)

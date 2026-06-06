import urllib.request
import urllib.parse
import json
import os

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

output_dir = "images"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for name, query in components.items():
    print(f"Searching for {query}...")
    try:
        # Search commons for the file
        search_url = f"https://commons.wikimedia.org/w/api.php?action=query&list=search&srsearch=File:{urllib.parse.quote(query)}&srnamespace=6&format=json"
        req = urllib.request.Request(search_url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as res:
            data = json.loads(res.read().decode())
            search_results = data['query']['search']
            if not search_results:
                print(f"No results for {query}")
                continue
            
            # Get the first file title
            file_title = search_results[0]['title']
            
            # Get the image URL
            info_url = f"https://commons.wikimedia.org/w/api.php?action=query&titles={urllib.parse.quote(file_title)}&prop=imageinfo&iiprop=url&iiurlwidth=800&format=json"
            info_req = urllib.request.Request(info_url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(info_req) as info_res:
                info_data = json.loads(info_res.read().decode())
                pages = info_data['query']['pages']
                page = list(pages.values())[0]
                
                if 'imageinfo' in page:
                    img_info = page['imageinfo'][0]
                    img_url = img_info.get('thumburl', img_info['url'])
                    
                    ext = img_url.split('.')[-1]
                    if "?" in ext:
                        ext = ext.split("?")[0]
                    if len(ext) > 4:
                        ext = "jpg"
                    
                    file_path = os.path.join(output_dir, f"{name}.{ext}")
                    print(f"Downloading {name} from {img_url}...")
                    
                    img_req = urllib.request.Request(img_url, headers={'User-Agent': 'Mozilla/5.0'})
                    with urllib.request.urlopen(img_req) as img_res_file, open(file_path, 'wb') as out_file:
                        out_file.write(img_res_file.read())
                    print(f"Success: {name}")
    except Exception as e:
        print(f"Failed {name}: {e}")

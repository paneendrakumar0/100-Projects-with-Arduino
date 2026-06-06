import wikipedia
import json

try:
    page = wikipedia.page("Arduino Uno")
    print("Images for Arduino Uno:")
    for img in page.images:
        if img.endswith('.jpg') or img.endswith('.png'):
            print(img)
            
    page2 = wikipedia.page("HC-SR04")
    print("Images for HC-SR04:")
    for img in page2.images:
        if img.endswith('.jpg') or img.endswith('.png'):
            print(img)
except Exception as e:
    print(f"Error: {e}")

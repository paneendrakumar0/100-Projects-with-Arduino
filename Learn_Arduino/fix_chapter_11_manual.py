import re

replacements = {
    "PIR_Sensor.jpg": "https://upload.wikimedia.org/wikipedia/commons/d/d5/PIR_Sensor_Visonic_Next%2BMCW.jpg",
    "IR_Receiver.jpg": "https://upload.wikimedia.org/wikipedia/commons/e/e3/IR_receiver_sensor.jpg",
    "Buzzer.jpg": "https://upload.wikimedia.org/wikipedia/commons/e/e4/Cjam-piezo-buzzer.png",
    "Servo_Motor.jpg": "https://upload.wikimedia.org/wikipedia/commons/d/d2/Servomotor_01.jpg",
    "LCD_16x2.jpg": "https://upload.wikimedia.org/wikipedia/commons/9/90/MELT_16x2_LCD_alphanumeric_display_07%28DXO%29.jpg",
    "Membrane_Keypad.jpg": "https://upload.wikimedia.org/wikipedia/commons/f/fe/IBM_Screen_Reader_Pad_membrane_exposed_through_absent_keys.jpg",
    "L298N_Motor_Driver.jpg": "https://upload.wikimedia.org/wikipedia/commons/d/df/L298N_Motor_Driver.jpg",
    "DC_Motor.jpg": "https://upload.wikimedia.org/wikipedia/commons/e/ea/DC_motor_with_gearbox.jpg",
    "Stepper_Motor.jpg": "https://upload.wikimedia.org/wikipedia/commons/2/22/Stepper_motor_with_driver.jpg",
    "Bluetooth_Module.jpg": "https://upload.wikimedia.org/wikipedia/commons/9/9e/Bluetooth_HC-05.jpg",
    "GPS_Module.jpg": "https://upload.wikimedia.org/wikipedia/commons/3/3d/NEO-6M_GPS_module.jpg",
    "MPU6050.jpg": "https://upload.wikimedia.org/wikipedia/commons/4/4b/MPU6050_Module.jpg",
    "MicroSD_Module.jpg": "https://upload.wikimedia.org/wikipedia/commons/f/f3/MicroSD_module.jpg",
    "Joystick_Module.jpg": "https://upload.wikimedia.org/wikipedia/commons/6/6f/Thumbstick_module.jpg"
}

with open("Chapter_11_Component_Glossary.md", "r", encoding="utf-8") as f:
    content = f.read()

for filename, url in replacements.items():
    content = content.replace(f"./images/{filename}", url)

with open("Chapter_11_Component_Glossary.md", "w", encoding="utf-8") as f:
    f.write(content)

print("Done replacing.")

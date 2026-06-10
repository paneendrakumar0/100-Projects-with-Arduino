import os
import sys
import re

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_day.py \"Project Name\"")
        print("Example: python generate_day.py \"WiFi Scanner\"")
        sys.exit(1)

    project_name = sys.argv[1].strip()
    safe_project_name = re.sub(r'[^a-zA-Z0-9]+', '_', project_name).strip('_')
    
    # Find the current highest day
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    day_dirs = [d for d in os.listdir(root_dir) if os.path.isdir(os.path.join(root_dir, d)) and d.startswith("Day_")]
    
    highest_day = 0
    for d in day_dirs:
        match = re.match(r'Day_(\d+)', d)
        if match:
            day_num = int(match.group(1))
            if day_num > highest_day:
                highest_day = day_num
                
    next_day = highest_day + 1
    dir_name = f"Day_{next_day:02d}_{safe_project_name}"
    
    # If past 100, no leading zero
    if next_day >= 100:
        dir_name = f"Day_{next_day}_{safe_project_name}"

    dir_path = os.path.join(root_dir, dir_name)
    
    if os.path.exists(dir_path):
        print(f"Directory {dir_name} already exists!")
        sys.exit(1)
        
    os.makedirs(dir_path)
    print(f"Created directory: {dir_name}")
    
    # Create the sketch file
    sketch_path = os.path.join(dir_path, f"{dir_name}.ino")
    with open(sketch_path, 'w') as f:
        f.write("void setup() {\n")
        f.write("  // Put your setup code here, to run once:\n")
        f.write("  Serial.begin(115200);\n")
        f.write("}\n\n")
        f.write("void loop() {\n")
        f.write("  // Put your main code here, to run repeatedly:\n")
        f.write("\n")
        f.write("}\n")
    print(f"Created sketch: {dir_name}.ino")
    
    # Create the README.md
    readme_path = os.path.join(dir_path, "README.md")
    with open(readme_path, 'w') as f:
        f.write(f"# Day {next_day}: {project_name}\n\n")
        f.write("## Description\n")
        f.write("Briefly describe the project here.\n\n")
        f.write("## Components Required\n")
        f.write("- Arduino/ESP32 Board\n")
        f.write("- Other components\n\n")
        f.write("## Circuit Diagram\n")
        f.write("*Add circuit diagram or description here*\n\n")
        f.write("## Learnings\n")
        f.write("- Point 1\n")
        f.write("- Point 2\n")
    print(f"Created README.md")
    
    print(f"\nSuccessfully generated {dir_name}!")

if __name__ == "__main__":
    main()

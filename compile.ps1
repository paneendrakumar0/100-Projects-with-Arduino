$ErrorActionPreference = 'Continue'
$hasError = $false

foreach ($dir in Get-ChildItem -Directory -Filter "Day_*") {
    $board = "arduino:avr:uno"
    $files = Get-ChildItem -Path $dir.FullName -Filter "*.ino"
    foreach ($file in $files) {
        $content = Get-Content -Path $file.FullName -Raw
        if ($content -match "Keyboard\.h|Mouse\.h|Joystick\.h|HID-Project\.h") {
            $board = "arduino:avr:leonardo"
            break
        }
    }
    Write-Host "Compiling $($dir.Name) for $board..."
    
    # Run the compile command and capture output
    $output = .\arduino-cli.exe compile -b $board $dir.FullName 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $($dir.Name)"
        Write-Host $output
        $hasError = $true
        break
    }
}

if ($hasError) {
    exit 1
} else {
    Write-Host "ALL COMPILED SUCCESSFULLY"
}

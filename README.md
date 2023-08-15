# A custom terminal application

![customTerminal](https://github.com/oPisiti/CustomTerminal/assets/78967454/3e35d576-7284-458f-a515-30a06f1f31d3)

OLCPixelGameEngine is used solely for handling a window and allowing the control of each pixel on screen.

Everything from drawing characters from a specified font forwards is custom made.

Uses the OS's default shell for command execution.

# Supports
- Simple one line commands
- Piping
- Stream redirection

# Usage

## Windows
You will need g++ installed.

Compile and run:
``` bash
g++ -o CustomTerminal.exe CustomTerminal.cpp -luser32 -lgdi32 -lopengl32 -lgdiplus -lShlwapi -ldwmapi -lstdc++fs -static -std=c++17 && ./CustomTerminal
``` 

## Linux
You will need build-essential installed:
``` bash
sudo apt install build-essential
```

Compile and run:
``` bash
g++ -o CustomTerminal CustomTerminal.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17 && ./CustomTerminal
```

# OLCPixelGameEngine
This project uses the OLCPixelGameEngine version 2.16.

Thank you to all the contributors.

For the latest release, go to https://github.com/OneLoneCoder/olcPixelGameEngine 

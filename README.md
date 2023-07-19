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
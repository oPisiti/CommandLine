#define OLC_PGE_APPLICATION

#ifdef _WIN32
#define OS_WINDOWS true
#else
#define OS_WINDOWS false
#endif

#include <iostream>
#include "Font.h"
#include <fstream>
#include "KeysMap.h"
#include "olcPixelGameEngine.h"
#include <sstream>
#include <vector>

class MyText : public olc::PixelGameEngine {
private:
	olc::Pixel backColor = {50, 50, 50, 255};

	uint32_t iBaseCharPosX    = 10, iBaseCharPosY = 10;
	uint32_t iInitCharPosX    = iBaseCharPosX, iInitCharPosY = iBaseCharPosY; // Offset to begin drawing the first character
	int32_t  iBeginRenderPosY = 0;											  // Rendering begins in this position. Changes when user scrolls
	
	uint8_t  iCharWidth = 8, iCharHeight = 8;	  							  // The grid in which to draw the pixels
	uint8_t  charac     = 0, iCharScale = 1;
	uint8_t  iMinScale  = 1, iMaxScale = 10;	  							  // For zooming

	// Keys
	std::vector<uint8_t>     pressedKeys, heldKeys;
	std::vector<std::string> history;										  // Contains all the typed information. Every command is stored one position of this vector
	std::string              sUser = "", addChar;					  		  // Constant initial text on screen
	std::string 		     sWorkingDir = "";

	bool bNewChar = false;	

	// Time and blinking
	float   fTimeCount;														  // Loops from 0 to fMaxTimeBlink
	float   fMaxTimeBlink = 0.75;											  // Intervals in which the blinking light turns on and off
	uint8_t iBlinkChar = 3;													  // Which char (based on ascii) to 
	bool    bDrawBlinker = true;

	std::vector<uint32_t> blinkerPos = { iInitCharPosX, iInitCharPosY };	  // Position to draw the blinker

	// Commands to terminal
	std::string sTemporaryOutputFileName = ".output.terminalApp.text";
	std::string sTerminalOutput = "";

public:

	MyText() {
		sAppName = "Custom Terminal";
	}

	// Draws a single character
	void drawCharacter(int32_t x, int32_t y, uint8_t Char, olc::Pixel color = olc::RED) {

		uint8_t curPosX, curPosY;		// Positions inside the font matrix
		int32_t screenX, screenY;		// Positions regarding the screen pixels

		for (int hei = 0; hei < iCharHeight; hei++) {
			curPosY = hei;
			screenY = y + hei * iCharScale;
			for (int wid = 0; wid < iCharWidth; wid++) {
				curPosX = wid;
				screenX = x + wid * iCharScale;

				// https://stackoverflow.com/questions/2249731/how-do-i-get-bit-by-bit-data-from-an-integer-value-in-c
				if ((font[Char][curPosY] & (1 << curPosX)) >> curPosX) {
					for (int i = 0; i < iCharScale; i++) {
						for (int j = 0; j < iCharScale; j++) {
							Draw(screenX + i, screenY + j, color);
						}
					}
				}
			}
		}
	}

	// Draws a full string. If screen width not enough, jumps to next line
	void drawString(int32_t initialX, int32_t initialY, std::string str, std::vector<uint32_t>& blinkerPos, uint32_t iBeginRenderPosY, olc::Pixel color = olc::RED) {

		int screenWidth = ScreenWidth();

		int32_t y = initialY;
		int maxSingleLineChars = (screenWidth - initialX) / (iCharWidth * iCharScale);

		int mod;
		for (int elemNum = 0; elemNum < str.size(); elemNum++) {
			// If screen space ends, jump to next line
			mod = elemNum % maxSingleLineChars;
			if (!mod && elemNum) {									
				y += iCharHeight * iCharScale;
			}

			// Too high up or down on screen space
			if(y < 0)     		continue;
			if(y > screenWidth) break;

			drawCharacter(initialX + iCharWidth * mod * iCharScale, y, str[elemNum], color);
		}

		// Adjusting the blinker position
		mod = str.size() % maxSingleLineChars;
		if (!mod) {
			y += iCharHeight * iCharScale;
		}
		blinkerPos = { initialX + uint32_t(iCharWidth * mod * iCharScale), uint32_t(y)};
	}

	// Executes a command to the appropriate terminal
	// A temporary output file is used and then the contents are read and put into 
	void ExecuteCommand(std::string sCommand){
		sCommand += " > " + sTemporaryOutputFileName + " 2>&1";
		std::system(sCommand.data());

		// As reading a file (with rdbuf()) returns a stream, this intermediary step is required
		std::stringstream buffer; 
		buffer << std::ifstream(sTemporaryOutputFileName).rdbuf();

		sTerminalOutput = buffer.str();
	}

	// Handling discrepancies between ASCII norm and what "GetAllKeys()" returns.
	void fixToASCII(std::string& addChar, const std::vector<uint8_t>& pressedKeys) {

		uint8_t iASCIIKey;

		for (auto key : pressedKeys) {


			// addChar = key;	// Default

			// Adjusting the key value
			auto equivalent = valueInputKeys.find(key);
			if(equivalent == valueInputKeys.end()) continue; 

			if(isShiftHeld()) iASCIIKey = equivalent->second.upper;
			else			  iASCIIKey = equivalent->second.lower;

			// Most inputs will make a new char
			bNewChar = true;

			for(auto k: pressedKeys)
				std::cout << "Key code: " << int(k) << ", iASCIIKey: " << iASCIIKey << std::endl;

			// // olc::PixelGameEngine simply returns different positions for each character.
			// // For example, "a" is the first position, not position 97, or 0x61. See https://www.asciitable.com/ for more

			// if (key >= 0 && key <= 26) {
			// 	addChar = key + 96;													// a-z
			// 	if (bShiftIsHeld) {													// SHIFT
			// 		addChar = key + 64;												// A-Z
			// 	}

			// 	// � held and no SHIFT	
			// 	if (std::find(heldKeys.begin(), heldKeys.end(), 91) != heldKeys.end()) {
			// 		switch (key) {
			// 		case 1:
			// 			addChar = 160;
			// 			break;
			// 		case 5:
			// 			addChar = 130;
			// 			break;
			// 		case 9:
			// 			addChar = 161;
			// 			break;
			// 		case 15:
			// 			addChar = 162;
			// 			break;
			// 		case 21:
			// 			addChar = 163;
			// 			break;
			// 		}
			// 	}
			// }

			// else if (key >= 27 && key <= 36) {
			// 	addChar = key + 21;													// Numbers
			// 	if (bShiftIsHeld) {
			// 		addChar = key + 5;												// Special Number characters
			// 	}
			// }

			// else if (key >= 69 && key <= 78)			addChar = key - 21;			// Numbers on Numpad
			// else if (key == 53)							addChar = key - 21;			// SPACE
			// else if (key == 63) {													// BACKSPACE
			// 	if (history.back().size() > GetFixText().length()) history.back().pop_back();
			// 	bNewChar = false;
			// }
			if (key == 66) {													// ENTER

				// Removing the initText
				uint8_t iTrashLength = GetFixText().length();
				std::string sOnlyCommand = history.back().substr(
										    iTrashLength, 
										    history.back().length() - iTrashLength);
				
				// Executing the command and showing output
				ExecuteCommand(sOnlyCommand);
				history.push_back("");
				for(auto c: sTerminalOutput){
					if(c == '\n') history.push_back("");
					else		  history.back() += c;
				}

				// User may have used a command that changes directory
				UpdateWorkingDirString();

				history.push_back(GetFixText());
				bNewChar = false;
			}
		}

		if (bNewChar) {
			// Resetting the count if user typed something - Prevents unnecessary blinking all the time (weird)
			fTimeCount = 0.0f;

			// The actual appending
			history.back() += iASCIIKey;
		}
	}
	
	// Returns the fix part of every new command line
	std::string GetFixText(){
		std::string ans = sUser + "@" + sWorkingDir ;
		return ans;
	}

	// Updates the variable sUser
	void UpdateUserString(){
		if(OS_WINDOWS) ExecuteCommand("echo %\\USERNAME%");
		else 		   ExecuteCommand("whoami");

		sUser = sTerminalOutput.substr(0, sTerminalOutput.length() - 1);
	}

	// Updates the variable sWorkingDir
	void UpdateWorkingDirString(){
		if(OS_WINDOWS) ExecuteCommand("cd");
		else 		   ExecuteCommand("pwd");

		sWorkingDir = sTerminalOutput.substr(0, sTerminalOutput.length() - 1) + ": ";
	}

	// Called once at the start, so create things here
	bool OnUserCreate() override {		
		UpdateUserString();
		UpdateWorkingDirString();
		history.push_back(GetFixText());

		return true;
	}

	// Called once per frame
	bool OnUserUpdate(float fElapsedTime) override {
		Clear(backColor);

		// Getting keys	
		pressedKeys      = GetAllPressedKeys();
		heldKeys         = GetAllHeldKeys();
		// bShiftIsHeld     = isShiftHeld();
		// bBackspaceIsHeld = isBackspaceHeld();

		// Zooming - Each wheel tick equals 120 in value (??)
		if (GetMouseWheel()) {			
			int wheel = GetMouseWheel() / 120;

			// When zooming
			if (std::find(heldKeys.begin(), heldKeys.end(), 56) != heldKeys.end()) {		// 56: CTRL
				
				iCharScale += wheel;
				if (iCharScale < iMinScale) iCharScale = iMinScale;
				else if (iCharScale > iMaxScale) iCharScale = iMaxScale;
			}

			// When scrolling
			else{
				int num = -wheel * iCharScale * iCharHeight;

				if (num < 0) {
					if (iBeginRenderPosY > iBaseCharPosX)	iBeginRenderPosY += num;
					else									iBeginRenderPosY = 0;
				}
				else										iBeginRenderPosY += num; 
			}
		}

		// Handling discrepancies between ASCII norm and what "GetAllKeys()" returns.
		bNewChar = true;
		if (pressedKeys.size() > 0) {
			fixToASCII(addChar, pressedKeys);
		}
		
		// Printing whole history
		// Drawing the whole String. If only one character needed, try "drawCharacter"
		for (int i = 0; i < history.size(); i++) {

			drawString(iInitCharPosX,
			 		   i ? blinkerPos[1] + iCharHeight * iCharScale : blinkerPos[1] - iBeginRenderPosY,
			 		   history[i],
			 		   blinkerPos,
			 		   iBeginRenderPosY,
			 		   olc::GREEN);

		}

		// Blinking of last character
		fTimeCount += fElapsedTime;
		if (fTimeCount > fMaxTimeBlink) {
			bDrawBlinker = !bDrawBlinker;
			fTimeCount -= fMaxTimeBlink;
		}

		if (bDrawBlinker) drawCharacter(blinkerPos[0], blinkerPos[1], iBlinkChar, olc::GREEN);
		
		// Resetting stuff
		blinkerPos = { iInitCharPosX, iInitCharPosY };
		return true;
	}
};


int main(){
	MyText myText;
	if (myText.Construct(600, 500, 2, 2))
		myText.Start();

	return 0;
}
#define OLC_PGE_APPLICATION

#include <iostream>
#include "Font.h"
#include <fstream>
#include "olcPixelGameEngine.h"
#include <sstream>
#include <vector>

class MyText : public olc::PixelGameEngine {
private:
	olc::Pixel backColor = {50, 50, 50, 255};

	uint32_t baseCharPosX    = 10, baseCharPosY = 10;
	uint32_t initCharPosX    = baseCharPosX, initCharPosY = baseCharPosY;	// Offset to begin drawing the first character
	long     beginRenderPosY = 0;											// Rendering begins in this position. Changes when user scrolls
	
	uint8_t  charWidth = 8, charHeight = 8;	 // The grid in which to draw the pixels
	uint8_t  charac    = 0, charScale = 1;
	uint8_t  minScale  = 1, maxScale = 10;	 // For zooming

	// Keys
	std::vector<uint8_t>     pressedKeys, heldKeys;
	std::vector<std::string> history;										// Contains all the typed information. Every command is stored one position of this vector
	std::string              initText = "C:>", addChar;						// Constant initial text on screen
	
	bool 	  shiftIsHeld = false, backspaceIsHeld = false;
	const int initTextSize = initText.size();
	bool      newChar = false;	

	// Time and blinking
	float   timeCount;														// Loops from 0 to maxTimeBlink
	float   maxTimeBlink = 0.75;											// Intervals in which the blinking light turns on and off
	uint8_t blinkChar = 3;													// Which char (based on ascii) to 
	bool    drawBlinker = true;

	std::vector<uint32_t> blinkerPos = { initCharPosX, initCharPosY };		// Position to draw the blinker

	// Commands to terminal
	std::string sTemporaryOutputFileName = "output.terminalApp.text";
	std::string sTerminalOutput = "";

public:

	MyText() {
		sAppName = "Custom Terminal";
	}

	// Draws a single character
	void drawCharacter(int32_t x, int32_t y, uint8_t Char, std::vector<std::vector<uint8_t>>& data, unsigned int scale = 1,
		olc::Pixel color = olc::RED) {

		uint8_t curPosX, curPosY;		// Positions inside the font matrix
		int32_t screenX, screenY;		// Positions regarding the screen pixels

		for (int hei = 0; hei < charHeight; hei++) {
			curPosY = hei;
			screenY = y + hei * scale;
			for (int wid = 0; wid < charWidth; wid++) {
				curPosX = wid;
				screenX = x + wid * scale;

				// https://stackoverflow.com/questions/2249731/how-do-i-get-bit-by-bit-data-from-an-integer-value-in-c
				if ((data[Char][curPosY] & (1 << curPosX)) >> curPosX) {
					for (int i = 0; i < scale; i++) {
						for (int j = 0; j < scale; j++) {
							Draw(screenX + i, screenY + j, color);
						}
					}
				}
			}
		}
	}

	// Draws a full string. If screen width not enough, jumps to next line
	void drawString(int32_t initialX, int32_t initialY, std::string str, std::vector<std::vector<uint8_t>>& data, int screenWidth, std::vector<uint32_t>& blinkerPos, uint32_t beginRenderPosY, unsigned int scale = 1,
		olc::Pixel color = olc::RED) {

		int32_t y = initialY;
		int maxSingleLineChars = (screenWidth - initialX) / (charWidth * scale);

		int mod;
		for (int elemNum = 0; elemNum < str.size(); elemNum++) {
			// If screen space ends, jump to next line
			mod = elemNum % maxSingleLineChars;
			if (!mod && elemNum) {									
				y += charHeight * scale;
			}

			drawCharacter(initialX + charWidth * mod * scale, y, str[elemNum], data, scale, color);
		}

		// Adjusting the blinker position
		mod = str.size() % maxSingleLineChars;
		if (!mod) {
			y += charHeight * scale;
		}
		blinkerPos = { initialX + uint32_t(charWidth * mod * scale), uint32_t(y)};
	}

	// Executes a command to the appropriate terminal
	// A temporary output file is used and then the contents are read and put into 
	// TODO: Check if works
	void executeCommand(std::string sCommand){
		sCommand += " > " + sTemporaryOutputFileName;
		std::system(sCommand.data());

		// As reading a file (with rdbuf()) returns a stream, this intermediary step is required
		std::stringstream buffer; 
		buffer << std::ifstream(sTemporaryOutputFileName).rdbuf();

		sTerminalOutput = buffer.str();
	}

	// Handling discrepancies between ASCII norm and what "GetAllKeys()" returns.
	void fixToASCII(std::string& addChar, const std::vector<uint8_t>& pressedKeys) {
		for (auto key : pressedKeys) {
			addChar = key;	// Default

			// olc::PixelGameEngine simply returns different positions for each character.
			// For example, "a" is the first position, not position 97, or 0x61. See https://www.asciitable.com/ for more

			if (key >= 0 && key <= 26) {
				addChar = key + 96;													// a-z
				if (shiftIsHeld) {													// SHIFT
					addChar = key + 64;												// A-Z
				}

				// � held and no SHIFT	
				if (std::find(heldKeys.begin(), heldKeys.end(), 91) != heldKeys.end()) {
					switch (key) {
					case 1:
						addChar = 160;
						break;
					case 5:
						addChar = 130;
						break;
					case 9:
						addChar = 161;
						break;
					case 15:
						addChar = 162;
						break;
					case 21:
						addChar = 163;
						break;
					}
				}
			}

			else if (key >= 27 && key <= 36) {
				addChar = key + 21;													// Numbers
				if (shiftIsHeld) {
					addChar = key + 5;												// Special Number characters
				}
			}

			else if (key >= 69 && key <= 78)			addChar = key - 21;			// Numbers on Numpad
			else if (key == 53)							addChar = key - 21;			// SPACE
			else if (key == 63) {													// BACKSPACE
				if (history.back().size() > initTextSize)	history.back().pop_back();
				newChar = false;
			}
			else if (key == 66) {													// ENTER
				history.push_back(initText);
				newChar = false;
			}
			else if (key == 84)							addChar = key - 38;			// .
			else if (key == 86)							addChar = key - 42;			// ,
			else newChar = false;
		}

		if (newChar) {
			// Resetting the count if user typed something - Prevents unnecessary blinking all the time (weird)
			timeCount = 0.0f;

			// The actual appending
			history.back() += addChar;
		}
	}

	// Called once at the start, so create things here
	bool OnUserCreate() override {		
		history.push_back(initText);

		return true;
	}

	// Called once per frame
	bool OnUserUpdate(float fElapsedTime) override {
		Clear(backColor);

		// Getting keys	
		pressedKeys     = GetAllPressedKeys();
		heldKeys        = GetAllHeldKeys();
		shiftIsHeld     = isShiftHeld();
		backspaceIsHeld = isBackspaceHeld();

		// Zooming - Each wheel tick equals 120 in value (??)
		if (GetMouseWheel()) {			
			int wheel = GetMouseWheel() / 120;

			// When zooming
			if (std::find(heldKeys.begin(), heldKeys.end(), 56) != heldKeys.end()) {		// 56: CTRL
				
				charScale += wheel;
				if (charScale < minScale) charScale = minScale;
				else if (charScale > maxScale) charScale = maxScale;
			}

			// When scrolling
			else{
				int num = -wheel * charScale * charHeight;

				if (num < 0) {
					if (beginRenderPosY > baseCharPosX)		beginRenderPosY += num;
					else									beginRenderPosY = 0;
				}
				else										beginRenderPosY += num; 
			}
		}

		// Handling discrepencies between ASCII norm and what "GetAllKeys()" returns.
		newChar = true;
		if (pressedKeys.size() > 0) {
			fixToASCII(addChar, pressedKeys);
		}
		
		// Printing whole history
		// Drawing the whole String. If only one character needed, try "drawCharacter"
		for (int i = 0; i < history.size(); i++) {
			drawString(initCharPosX, i ? blinkerPos[1] + charHeight * charScale : blinkerPos[1] - beginRenderPosY, history[i], font, ScreenWidth(), blinkerPos, beginRenderPosY, charScale, olc::GREEN);

		}

		// Blinking of last character
		timeCount += fElapsedTime;
		if (timeCount > maxTimeBlink) {
			drawBlinker = !drawBlinker;
			timeCount -= maxTimeBlink;
		}

		if (drawBlinker) drawCharacter(blinkerPos[0], blinkerPos[1], blinkChar, font, charScale, olc::GREEN);
		
		// Resetting stuff
		blinkerPos = { initCharPosX, initCharPosY };
		return true;
	}
};


int main(){
	MyText myText;
	if (myText.Construct(600, 500, 2, 2))
		myText.Start();

	return 0;
}

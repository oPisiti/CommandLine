#define OLC_PGE_APPLICATION

#include <chrono>
#include <filesystem>
#include "Font.h"
#include <fstream>
#include "KeysMap.h"
#include "olcPixelGameEngine.h"
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WINDOWS_OS true
const std::string CURR_DIR_COMMAND = "cd";
#include <cstdint>
#else
#define WINDOWS_OS false
const std::string CURR_DIR_COMMAND = "pwd";
#endif


class MyText : public olc::PixelGameEngine {
private:
	// Rendering
	const olc::Pixel olcPBackColor = {50, 50, 50, 255};
	const olc::Pixel olcPFontColor = olc::MAGENTA;

	uint32_t iBaseCharPosX    = 10, iBaseCharPosY = 10;
	uint32_t iInitCharPosX    = iBaseCharPosX, iInitCharPosY = iBaseCharPosY; // Offset to begin drawing the first character
	int32_t  iBeginRenderPosY = 0;											  // Rendering begins in this position. Changes when user scrolls
	
	uint8_t  iCharWidth = 8, iCharHeight = 8;	  							  // The grid in which to draw the pixels
	uint8_t  charac     = 0, iCharScale = 1;
	uint8_t  iMinScale  = 1, iMaxScale = 10;	  							  // For zooming

	// Keys
	std::vector<uint8_t>     pressedKeys, heldKeys;
	std::vector<std::string> history;										  // Contains all the typed information. Every command is stored one position of this vector
	std::vector<std::string> sCommandsHistory;								  // Contains only the valid issued commands
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
	const std::string sTmpOutputFileName   = ".output.tmp";
	const std::string sTmpUsernameFileName = ".username.tmp";
	const std::string sTmpCurrDirFileName  = ".currdir.tmp";
		  std::string sTerminalOutput      = "";
		  std::string sOnlyCommand;

	// Accessing history
	int16_t iRelativeCommandsHistoryIndex = 0;								  // Which commands history index to show when up or down arrow is pressed. Relative to the last element

	// IO monitoring
	std::filesystem::file_time_type fLastModifiedTime, fCurrModifiedTime;
	std::thread tMonitorStdout, tExecuteCommand; 


public:

	MyText() {
		sAppName = "Custom Terminal";
	}

	~MyText(){
		tMonitorStdout.join();
		tExecuteCommand.join();
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
	void ExecuteCommand(std::string sCommand, std::string sOutputFile, std::string& sOutputString){
		
		std::cout << "sCommand: " << sCommand << std::endl;
		std::cout << "sOutputFile: " << sOutputFile << std::endl;

		sCommand = "cd '" + sWorkingDir + "' && (" + sCommand + ") > " + sOutputFile + " 2>&1";
		sCommand += " && " + CURR_DIR_COMMAND + " > " + sTmpCurrDirFileName;

		std::cout << "FINAL COMMAND: " << sCommand << std::endl;

		std::system(sCommand.data());

		// As reading a file (with rdbuf()) returns a stream, this intermediary step is required
		std::stringstream buffer; 
		buffer << std::ifstream(sOutputFile).rdbuf();

		sOutputString = buffer.str();

	}

	// Returns the fix part of every new command line
	std::string GetFixText(){
		std::string ans = sUser + "@" + sWorkingDir + ": ";
		return ans;
	}

	// Handling discrepancies between ASCII norm and what "GetAllKeys()" returns.
	void HandleKeyPress(std::string& addChar, const std::vector<uint8_t>& pressedKeys) {

		uint8_t iASCIIKey;

		for (auto key : pressedKeys) {
			// Default value
			iASCIIKey = 0;

			// Adjusting the key value
			auto equivalent = valueInputKeys.find(key);
			
			// Key found
			if(equivalent != valueInputKeys.end()){
				if(isShiftHeld()) iASCIIKey = equivalent->second.upper;
				else			  iASCIIKey = equivalent->second.lower;

				// Printable/supported char
				bNewChar = true;
				continue;
			}

			// Key not found -> Not a printable/supported char
			bNewChar = false;

			// ENTER
			if (key == 66) {													

				// Removing the initText
				uint8_t iTrashLength = GetFixText().length();
				sOnlyCommand = history.back().substr(
										    iTrashLength, 
										    history.back().length() - iTrashLength);
				
				// Executing the command and showing output
				tExecuteCommand = std::thread(&MyText::ExecuteCommand,
									 		  this,
				 							  std::ref(sOnlyCommand),
				 							  std::ref(sTmpOutputFileName),
											  std::ref(sTerminalOutput));				

				bNewChar = false;
			}

			// BACKSPACE
			else if (key == 63) {													
				if (history.back().size() > GetFixText().length()) history.back().pop_back();
				bNewChar = false;
			}

			// ARROW KEYS
			else if((key == 49 || key == 50) && history.size() > 1){

				// UP ARROW
				if(key == 49 && -iRelativeCommandsHistoryIndex < sCommandsHistory.size())
					iRelativeCommandsHistoryIndex -= 1;

				// DOWN ARROW
				else if(key == 50 && iRelativeCommandsHistoryIndex <  0)
					iRelativeCommandsHistoryIndex += 1;	

				if(iRelativeCommandsHistoryIndex == 0) history.back() = GetFixText();
				else history.back() = GetFixText() + sCommandsHistory[sCommandsHistory.size() + iRelativeCommandsHistoryIndex];
			}

		}

		if (bNewChar) {
			// Resetting the count if user typed something - Prevents unnecessary blinking all the time (weird)
			fTimeCount = 0.0f;

			// The actual appending
			history.back() += iASCIIKey;
		}
	}

	// Handles new ouput in "sTmpOutputFileName" file
	void HandleNewOuput(){
		std::string sCurrTerminalOutput, sNewData;
		int iIndexStringDiffer;

		// --- Comparing stdout' current version to sTerminalOutput ---
		sCurrTerminalOutput = sTerminalOutput;

		// As reading a file (with rdbuf()) returns a stream, this intermediary step is required
		std::stringstream buffer; 
		buffer << std::ifstream(sTmpOutputFileName).rdbuf();
		sTerminalOutput = buffer.str();

		//--- Adding only the difference in strings ---
		iIndexStringDiffer = sCurrTerminalOutput.compare(sTerminalOutput);

		std::cout << "iIndexStringDiffer: " << iIndexStringDiffer << std::endl;
		
		// Index == 0 means the same output.
		// In this case, the last command must have been the same as the previous
		if(iIndexStringDiffer == 0) sNewData = sTerminalOutput;
		else{
			if(sTerminalOutput.size() < sCurrTerminalOutput.size()){ 
									sNewData = sTerminalOutput;
			}
			else					sNewData = sTerminalOutput.substr(sCurrTerminalOutput.size());
		}
			
		// TODO: FIX bug: Simply crashes at second command
		// Nothing to do with the below code of this function
		// This problem does not occur when .output.tmp file is manually changed, but keeps the same size
		// It has to do with the size of sTerminalOutput
		// If .output.tmp size is changed, it crashes

		history.push_back("");
		for(auto c: sNewData){
			if(c == '\n') history.push_back("");
			else		  history.back() += c;
		}

		// User may have used a command that changes directory
		// UpdateWorkingDirString();

		history.push_back(GetFixText());

		// Dealing with the specific commands history vector
		sCommandsHistory.push_back(sOnlyCommand);
		iRelativeCommandsHistoryIndex = 0;

		fLastModifiedTime = fCurrModifiedTime;
		std::cout << "New File!" << std::endl;
	}


	// Monitors stdout file for changes
	void MonitorStdout(){

		while(true){
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));

			fCurrModifiedTime = std::filesystem::last_write_time(sTmpOutputFileName);

			std::cout << "Looking..." << std::endl;

			// Something was written to stdout
			if(fCurrModifiedTime > fLastModifiedTime){
				HandleNewOuput();
			}

		}
	}


	// Updates the variable sUser
	void UpdateUserString(){
		std::string command;

		if(WINDOWS_OS) command = "echo %username%";
		else 		   command = "whoami";

		ExecuteCommand(command, sTmpUsernameFileName, sUser);

		sUser = sUser.substr(0, sUser.length() - 1);
	}

	// Updates the variable sWorkingDir
	void UpdateWorkingDirString(){
		std::string command;

		if(WINDOWS_OS) command = "cd";
		else 		   command = "pwd";

		ExecuteCommand(command, sTmpCurrDirFileName, sWorkingDir);

		sWorkingDir = sWorkingDir.substr(0, sWorkingDir.length() - 1);
	}

	// Called once at the start, so create things here
	bool OnUserCreate() override {		
		// Creating the temp file
		std::string sUseless;
		ExecuteCommand(">" + sTmpOutputFileName, sTmpOutputFileName, sUseless);

		UpdateUserString();
		UpdateWorkingDirString();
		history.push_back(GetFixText());

		// Getting the last modified time
		fLastModifiedTime = std::filesystem::last_write_time(sTmpOutputFileName);
		fCurrModifiedTime = fLastModifiedTime;

		tMonitorStdout = std::thread(&MyText::MonitorStdout, this);

		return true;
	}

	// Called once per frame
	bool OnUserUpdate(float fElapsedTime) override {
		Clear(olcPBackColor);

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
			HandleKeyPress(addChar, pressedKeys);
		}
		
		// Printing whole history
		// Drawing the whole String. If only one character needed, try "drawCharacter"
		for (int i = 0; i < history.size(); i++) {

			drawString(iInitCharPosX,
			 		   i ? blinkerPos[1] + iCharHeight * iCharScale : blinkerPos[1] - iBeginRenderPosY,
			 		   history[i],
			 		   blinkerPos,
			 		   iBeginRenderPosY,
			 		   olcPFontColor);

		}

		// Blinking of last character
		fTimeCount += fElapsedTime;
		if (fTimeCount > fMaxTimeBlink) {
			bDrawBlinker = !bDrawBlinker;
			fTimeCount -= fMaxTimeBlink;
		}

		if (bDrawBlinker) drawCharacter(blinkerPos[0], blinkerPos[1], iBlinkChar, olcPFontColor);
		
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
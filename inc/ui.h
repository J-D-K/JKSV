#ifndef UI_H
#define UI_H

#include <vector>
#include <string>

#include "data.h"

namespace ui
{
	//Loads in the A, B, X, Y button graphics
	void init();

	//Button controlled menu
	class menu
	{
		public:
			//Adds option
			void addOpt(const std::string& add);
			//Clears menu stuff
			~menu();

			//Handles controller input
			void handleInput(const uint64_t& down, const uint64_t& held);

			//Returns selected option
			int getSelected();

			//Draws the menu at x and y. rectWidth is the width of the rectangle drawn under the selected
			void print(const unsigned& x, const unsigned& y, const uint32_t& rectWidth);

			//Clears and resets menu
			void reset();

		private:
			//Options vector
			std::vector<std::string> opt;
			//Selected + frame counting for auto-scroll
			int selected = 0, fc = 0, start = 0;
			//How much we shift the color of the rectangle
			uint8_t clrSh = 0;
			bool clrAdd = true;
	};

	//Progress bar for showing loading. Mostly so people know it didn't freeze
	class progBar
	{
		public:
			//Constructor. _max is the maximum value
			progBar(const uint32_t& _max);

			//Updates progress
			void update(const uint32_t& _prog);

			//Draws with text at top
			void draw(const std::string& text);

		private:
			float max, prog, width;
	};

	//The biggest mess. The keyboard!
	//Class for individual keys
	class key
	{
		public:
			//Key constructor. Txt = what text is drawn. _let = letter, _txtSz = font size, _w, _h = width, height
			key(const std::string& txt, const char& _let, const unsigned& _txtSz, const unsigned& _x, const unsigned& _y, const unsigned& _w, const unsigned& _h);
			//Updates the text shown
			void updateText(const std::string& txt);
			//Draws key
			void draw();
			//Tells if user is touching screen at key's position
			bool isOver(const touchPosition& p);
			//Tells if user has stopped touching
			bool released(const touchPosition& p);
			//returns character in 'let;
			char getLet();
			//toUpper
			void toCaps();
			//toLower
			void toLower();

			//Return properties about key
			unsigned getX();
			unsigned getY();
			unsigned getW();
			unsigned getH();

		private:
			bool pressed;
			char let;
			unsigned x, y, w, h;
			unsigned tX, tY;
			unsigned txtSz;
			std::string text;
			touchPosition prev;
	};

	class keyboard
	{
		public:
			//Builds keyboard
			keyboard();
			~keyboard();

			void draw();
			//returns string
			std::string getString();

		private:
			std::vector<key> keys;
			std::string str;
			int selKey = 0;
			uint8_t clrSh = 0;
			bool clrAdd = true;
	};

	class button
	{
		public:
			button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h);
			void draw();

			bool isOver(const touchPosition& p);
			bool released(const touchPosition& p);

		private:
			bool pressed = false;
			unsigned x, y, w, h;
			unsigned tx, ty;
			std::string text;
			touchPosition prev;
	};

	//Prepares menus for use
	void userMenuInit();
	void titleMenuPrepare(data::user& usr);
	void folderMenuPrepare(data::user& usr, data::titledata& dat);

	//Updates + draws menus
	void showUserMenu(const uint64_t& down, const uint64_t& held);
	void showTitleMenu(const uint64_t& down, const uint64_t& held);
	void showFolderMenu(const uint64_t& down, const uint64_t& held);

	//Draws menus
	void drawUI();

	//Draws title bar at top
	void drawTitleBar(const std::string& txt);

	//switch case so we don't have problems with multiple main loops like 3DS
	void runApp(const uint64_t& down, const uint64_t& held);

	void showMessage(const std::string& mess);
	void showError(const std::string& mess, const Result& r);
	bool confirm(const std::string& q);
}

#endif

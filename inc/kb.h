#ifndef KB_H
#define KB_H

#include <string>
#include <vector>

namespace ui
{
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
			std::string getString(const std::string& def);

		private:
			std::vector<key> keys;
			std::string str;
			int selKey = 0;
			uint8_t clrSh = 0;
			bool clrAdd = true;
	};
}

#endif // KB_H

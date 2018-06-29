#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

namespace ui
{
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
			void print(const unsigned& x, const unsigned& y, const uint32_t& textClr, const uint32_t& rectWidth);

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
}

#endif

#include <string>
#include <switch.h>

#include "gfx.h"
#include "menu.h"

namespace ui
{
	void menu::addOpt(const std::string& add)
	{
		opt.push_back(add);
	}

	menu::~menu()
	{
		opt.clear();
	}

	void menu::handleInput(const uint64_t& down, const uint64_t& held)
	{
		if( (held & KEY_UP) || (held & KEY_DOWN))
			fc++;
		else
			fc = 0;
		if(fc > 10)
			fc = 0;

		int size = opt.size() - 1;
		if((down & KEY_UP) || ((held & KEY_UP) && fc == 10))
		{
			selected--;
			if(selected < 0)
				selected = size;

			if((start > selected)  && (start > 0))
				start--;
			if(size < 15)
				start = 0;
			if((selected - 14) > start)
				start = selected - 14;
		}
		else if((down & KEY_DOWN) || ((held & KEY_DOWN) && fc == 10))
		{
			selected++;
			if(selected > size)
				selected = 0;

			if((selected > (start + 14)) && ((start + 14) < size))
				start++;
			if(selected == 0)
				start = 0;
		}
		else if(down & KEY_RIGHT)
		{
			selected += 7;
			if(selected > size)
				selected = size;
			if((selected - 14) > start)
				start = selected - 14;
		}
		else if(down & KEY_LEFT)
		{
			selected -= 7;
			if(selected < 0)
				selected = 0;
			if(selected < start)
				start = selected;
		}
	}

	int menu::getSelected()
	{
		return selected;
	}

	void menu::print(const unsigned& x, const unsigned& y, const uint32_t& textClr, const uint32_t& rectWidth)
	{
		if(clrAdd)
		{
			clrSh += 4;
			if(clrSh > 63)
				clrAdd = false;
		}
		else
		{
			clrSh--;
			if(clrSh == 0)
				clrAdd = true;
		}

		int length = 0;
		if((opt.size() - 1) < 15)
			length = opt.size();
		else
			length = start + 15;

		uint32_t rectClr = 0xFF << 24 | ((0xBB + clrSh) & 0xFF) << 16 | ((0x60 + clrSh)) << 8 | 0x00;

		for(int i = start; i < length; i++)
		{
			if(i == selected)
				gfx::drawRectangle(x, y + ((i - start) * 36), rectWidth, 32, rectClr);

			gfx::drawText(opt[i], x, y + ((i - start) * 36), 38, textClr);
		}
	}

	void menu::reset()
	{
		opt.clear();

		selected = 0;
		fc = 0;
		start = 0;
	}
}

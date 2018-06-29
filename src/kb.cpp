#include <switch.h>

#include "gfx.h"
#include "kb.h"
#include "util.h"

static const char qwerty[] =
{
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '_',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ':', '.', '/'
};

namespace ui
{
	key::key(const std::string& txt, const char& _let, const unsigned& _txtSz, const unsigned& _x, const unsigned& _y, const unsigned& _w, const unsigned& _h)
	{
		x = _x;
		y = _y;
		w = _w;
		h = _h;
		txtSz = _txtSz;
		let = _let;

		text = txt;

		tX = x + 16;
		tY = y + 16;

		pressed = false;
	}

	void key::updateText(const std::string& txt)
	{
		text = txt;
	}

	void key::draw()
	{
		if(pressed)
			gfx::drawRectangle(x, y, w, h, 0xFF2D2D2D);
		else
			gfx::drawRectangle(x, y, w, h, 0xFFEBEBEB);

		gfx::drawText(text, tX, tY, txtSz, 0xFF000000);
	}

	bool key::isOver(const touchPosition& p)
	{
		return (p.px > x && p.px < x + w) && (p.py > y && p.py < y + h);
	}

	bool key::released(const touchPosition& p)
	{
		prev = p;
		if(isOver(p))
			pressed = true;
		else
		{
			uint32_t touchCount = hidTouchCount();
			if(pressed && touchCount == 0)
			{
				pressed = false;
				return true;
			}
			else
				pressed = false;
		}

		return false;
	}

	char key::getLet()
	{
		return let;
	}

	void key::toCaps()
	{
		let = toupper(let);

		char tmp[8];
		sprintf(tmp, "%c", let);
		updateText(tmp);
	}

	void key::toLower()
	{
		let = tolower(let);

		char tmp[2];
		sprintf(tmp, "%c", let);
		updateText(tmp);
	}

	unsigned key::getX()
	{
		return x;
	}

	unsigned key::getY()
	{
		return y;
	}

	unsigned key::getW()
	{
		return w;
	}

	unsigned key::getH()
	{
		return h;
	}

	keyboard::keyboard()
	{
		int x = 160, y = 256;
		for(unsigned i = 0; i < 40; i++, x += 96)
		{
			char tmp[2];
			sprintf(tmp, "%c", qwerty[i]);
			key newKey(tmp, qwerty[i], 64, x, y, 80, 80);
			keys.push_back(newKey);

			char ch = qwerty[i];
			if(ch == '0' || ch == 'p' || ch == '_')
			{
				x = 64;
				y += 96;
			}
		}

		//spc key
		key shift("Shift", ' ', 64, 16, 544, 128, 80);
		//Space bar needs to be trimmed back so we don't try to draw off buffer
		key space(" ", ' ', 64, 240, 640, 800, 72);
		key bckSpc("Back", ' ', 64, 1120, 256, 128, 80);
		key enter("Entr", ' ', 64, 1120, 352, 128, 80);
		key cancel("Cancl", ' ', 64, 1120, 448, 128, 80);

		keys.push_back(space);
		keys.push_back(shift);
		keys.push_back(bckSpc);
		keys.push_back(enter);
		keys.push_back(cancel);
	}

	keyboard::~keyboard()
	{
		keys.clear();
	}

	void keyboard::draw()
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

		gfx::drawRectangle(0, 176, 1280, 64, 0xFFFFFFFF);
		gfx::drawRectangle(0, 240, 1280, 480, 0xFF2D2D2D);

		uint32_t rectClr = 0xFF << 24 | ((0xBB + clrSh) & 0xFF) << 16 | ((0x88 + clrSh) & 0xFF) << 8 | 0x00;

		//Draw sel rectangle around key for controller
		gfx::drawRectangle(keys[selKey].getX() - 4, keys[selKey].getY() - 4, keys[selKey].getW() + 8, keys[selKey].getH() + 8, rectClr);

		for(unsigned i = 0; i < keys.size(); i++)
			keys[i].draw();

		gfx::drawText(str, 16, 192, 64, 0xFF000000);
	}

	std::string keyboard::getString(const std::string& def)
	{
		str = def;
		while(true)
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
			if(down & KEY_R)
				str += util::getDateTime();

			touchPosition p;
			hidTouchRead(&p, 0);

			//Controller input for keyboard
			if(down & KEY_RIGHT)
				selKey++;
			else if(down & KEY_LEFT && selKey > 0)
				selKey--;
			else if(down & KEY_DOWN)
			{
				selKey += 10;
				if(selKey > 40)
					selKey = 40;
			}
			else if(down & KEY_UP)
			{
				selKey -= 10;
				if(selKey < 0)
					selKey = 0;
			}
			else if(down & KEY_A)
			{
				if(selKey < 41)
				{
					str += keys[selKey].getLet();
				}
			}
			else if(down & KEY_X)
			{
				str += ' ';
			}

			//Stndrd key
			for(unsigned i = 0; i < 41; i++)
			{
				if(keys[i].released(p))
				{
					str += keys[i].getLet();
				}
			}

			//shift
			if(keys[41].released(p) || down & KEY_LSTICK)
			{
				if(keys[10].getLet() == 'q')
				{
					for(unsigned i = 10; i < 41; i++)
						keys[i].toCaps();
				}
				else
				{
					for(unsigned i = 10; i < 41; i++)
						keys[i].toLower();
				}
			}
			//bckspace
			else if(keys[42].released(p) || down & KEY_Y)
			{
				if(!str.empty())
					str.erase(str.end() - 1, str.end());
			}
			//enter
			else if(keys[43].released(p) || down & KEY_PLUS)
				break;
			//cancel
			else if(keys[44].released(p) || down & KEY_B)
			{
				str.erase(str.begin(), str.end());
				break;
			}

			draw();

			gfx::handleBuffs();
		}

		return str;
	}
}

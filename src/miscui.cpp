#include <switch.h>

#include "gfx.h"
#include "ui.h"
#include "miscui.h"
#include "util.h"

namespace ui
{
	progBar::progBar(const uint32_t& _max)
	{
		max = (float)_max;
	}

	void progBar::update(const uint32_t& _prog)
	{
		prog = (float)_prog;

		float percent = (float)(prog / max) * 100;
		width = (float)(percent * 1088) / 100;
	}

	void progBar::draw(const std::string& text)
	{
		ui::drawTextbox(64, 240, 1152, 240);
		gfx::drawRectangle(96, 400, 1088, 64, 0xFF000000);
		gfx::drawRectangle(96, 400, (uint32_t)width, 64, 0xFF00CC00);

		//char tmp[64];
		//sprintf(tmp, "%u / %u", (unsigned)prog, (unsigned)max);
		gfx::drawText(text, 80, 256, 64, txtClr);
		//gfx::drawText(tmp, 80, 320, 64, 0x000000FF);
	}

	button::button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h)
	{
		x = _x;
		y = _y;
		w = _w;
		h = _h;
		text = _txt;

		unsigned tw = gfx::getTextWidth(text, 48);
		unsigned th = gfx::getTextHeight(48);

		tx = x + (w / 2) - (tw / 2);
		ty = y + (h / 2) - (th / 2);
	}

	void button::draw()
	{
		if(pressed)
			gfx::drawRectangle(x, y, w, h, 0xFFD0D0D0);
		else
		{
			ui::drawTextbox(x, y, w, h);
		}

		gfx::drawText(text, tx, ty, 48, txtClr);
	}

	bool button::isOver(const touchPosition& p)
	{
		return (p.px > x && p.px < x + w) && (p.py > y && p.py < y + h);
	}

	bool button::released(const touchPosition& p)
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

	void showMessage(const std::string& mess)
	{
		button ok("OK (A)", 256, 496, 768, 96);
		std::string wrapMess = util::getWrappedString(mess, 48, 752);
		while(true)
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
			touchPosition p;
			hidTouchRead(&p, 0);

			if(down & KEY_A || down & KEY_B || ok.released(p))
				break;

			ui::drawTextbox(256, 128, 768, 464);
			gfx::drawText(wrapMess, 272, 144, 48, txtClr);
			ok.draw();

			gfx::handleBuffs();
		}
	}

	void showError(const std::string& mess, const Result& r)
	{
		button ok("OK (A)", 256, 496, 768, 96);
		char tmp[512];
		std::string wrapMess = util::getWrappedString(mess, 48, 752);
		sprintf(tmp, "%s\n0x%08X", mess.c_str(), (unsigned)r);

		while(true)
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
			touchPosition p;
			hidTouchRead(&p, 0);

			if(down & KEY_A || down & KEY_B || ok.released(p))
				break;

			ui::drawTextbox(256, 128, 768, 464);
			gfx::drawText(tmp, 272, 144, 48, txtClr);
			ok.draw();

			gfx::handleBuffs();
		}
	}

	bool confirm(const std::string& mess)
	{
		bool ret = false;

		button yes("Yes (A)", 256, 496, 384, 96);
		button no("No (B)", 640, 496, 384, 96);

		std::string wrapMess = util::getWrappedString(mess, 48, 752);

		while(true)
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
			touchPosition p;
			hidTouchRead(&p, 0);

			if(down & KEY_A || yes.released(p))
			{
				ret = true;
				break;
			}
			else if(down & KEY_B || no.released(p))
			{
				ret = false;
				break;
			}

			ui::drawTextbox(256, 128, 768, 464);
			gfx::drawText(wrapMess, 272, 144, 48, txtClr);
			yes.draw();
			no.draw();

			gfx::handleBuffs();
		}

		return ret;
	}

	bool confirmTransfer(const std::string& f, const std::string& t)
	{
		std::string confMess = "Are you sure you want to copy \"" + f + "\" to \"" + t +"\"?";

		return confirm(confMess);
	}

	bool confirmDelete(const std::string& p)
	{
		std::string confMess = "Are you 100% sure you want to delete \"" + p + "\"? This is permanent!";

		return confirm(confMess);
	}

	void drawTextbox(unsigned x, unsigned y, unsigned w, unsigned h)
	{
		//Top
		cornerTopLeft.draw(x, y);
		horEdgeTop.drawRepeatHoriNoBlend(x + 32, y, w - 64);
		cornerTopRight.draw((x + w) - 32, y);

		//middle
		vertEdgeLeft.drawRepeatVertNoBlend(x, y + 32, h - 64);
		gfx::drawRectangle(x + 32, y + 32, w - 64, h - 64, tboxClr);
		vertEdgeRight.drawRepeatVertNoBlend((x + w) - 32, y + 32, h - 64);

		//bottom
		cornerBottomLeft.draw(x, (y + h) - 32);
		horEdgeBot.drawRepeatHoriNoBlend(x + 32, (y + h) - 32, w - 64);
		cornerBottomRight.draw((x + w) - 32, (y + h) -32);

	}
}

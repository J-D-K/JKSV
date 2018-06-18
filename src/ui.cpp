#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <switch.h>

#include "ui.h"
#include "gfx.h"
#include "util.h"
#include "file.h"
#include "sys.h"

static const char qwerty[] =
{
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '_',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', '-', '@', '+'
};

enum menuState
{
	USR_SEL,
	TTL_SEL,
	FLD_SEL,
	DEV_MNU
};

static int mstate = USR_SEL;

static ui::menu userMenu, titleMenu, folderMenu, devMenu;
static gfx::tex buttonA, buttonB, buttonX, buttonY, titleBar;

namespace ui
{
	void init()
	{
		buttonA.loadFromFile("romfs:/img/buttonA.data");
		buttonB.loadFromFile("romfs:/img/buttonB.data");
		buttonX.loadFromFile("romfs:/img/buttonX.data");
		buttonY.loadFromFile("romfs:/img/buttonY.data");
		titleBar.loadFromFile("romfs:/img/topbar.data");
	}

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

	void menu::print(const unsigned& x, const unsigned& y, const uint32_t& rectWidth)
	{
		if(clrAdd)
		{
			clrSh++;
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

		uint32_t rectClr = 0xFF << 24 | ((0xBB + clrSh) & 0xFF) << 16 | ((0x88 + clrSh)) << 8 | 0x00;

		for(int i = start; i < length; i++)
		{
			if(i == selected)
				gfx::drawRectangle(x, y + ((i - start) * 36), rectWidth, 32, rectClr);

			gfx::drawText(opt[i], x, y + ((i - start) * 36), 38, 0xFFFFFFFF);
		}
	}

	void menu::reset()
	{
		opt.clear();

		selected = 0;
		fc = 0;
		start = 0;
	}

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
		gfx::drawRectangle(64, 240, 1152, 240, 0xFFC0C0C0);
		gfx::drawRectangle(96, 400, 1088, 64, 0xFF000000);
		gfx::drawRectangle(96, 400, (uint32_t)width, 64, 0xFF00CC00);

		//char tmp[64];
		//sprintf(tmp, "%u / %u", (unsigned)prog, (unsigned)max);
		gfx::drawText(text, 80, 256, 64, 0xFF000000);
		//gfx::drawText(tmp, 80, 320, 64, 0x000000FF);
	}

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
			gfx::drawRectangle(x, y, w, h, 0xFF2B2B2B);
		else
			gfx::drawRectangle(x, y, w, h, 0xFFC0C0C0);

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
			clrSh++;
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
		gfx::drawRectangle(0, 240, 1280, 480, 0xFF3B3B3B);

		uint32_t rectClr = 0xFF << 24 | ((0xBB + clrSh) & 0xFF) << 16 | ((0x88 + clrSh) & 0xFF) << 8 | 0x00;

		//Draw sel rectangle around key for controller
		gfx::drawRectangle(keys[selKey].getX() - 4, keys[selKey].getY() - 4, keys[selKey].getW() + 8, keys[selKey].getH() + 8, rectClr);

		for(unsigned i = 0; i < keys.size(); i++)
			keys[i].draw();

		gfx::drawText(str, 16, 192, 64, 0xFF000000);
	}

	std::string keyboard::getString()
	{
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
		gfx::drawRectangle(x - 2, y - 2, w + 4, h + 4, 0xFF303030);

		if(pressed)
			gfx::drawRectangle(x, y, w, h, 0xFF2B2B2B);
		else
		{
			gfx::drawRectangle(x, y, w, h, 0xFFC0C0C0);
		}

		gfx::drawText(text, tx, ty, 48, 0xFF000000);
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

	void userMenuInit()
	{
		for(unsigned i = 0; i < data::users.size(); i++)
			userMenu.addOpt(data::users[i].getUsername());
	}

	void titleMenuPrepare(data::user& usr)
	{
		titleMenu.reset();

		for(unsigned i = 0; i < usr.titles.size(); i++)
			titleMenu.addOpt(usr.titles[i].getTitle());
	}

	void folderMenuPrepare(data::user& usr, data::titledata& dat)
	{
		folderMenu.reset();

		util::makeTitleDir(usr, dat);
		std::string scanPath = util::getTitleDir(usr, dat);

		fs::dirList list(scanPath);
		folderMenu.addOpt("New");
		for(unsigned i = 0; i < list.getCount(); i++)
			folderMenu.addOpt(list.getItem(i));
	}

	void devMenuPrepare()
	{
		devMenu.addOpt("Dump all saves");
	}

	void drawMenus()
	{
		switch(mstate)
		{
			case USR_SEL:
				{
					//Menu
					userMenu.print(16, 88, 424);
					//Input guide
					unsigned startX = 1152;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 668, 32, 0xFFFFFFFF);
				}
				break;

			case TTL_SEL:
				{
					//Menu
					titleMenu.print(16, 88, 424);
					//Input guide
					unsigned startX = 1056;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonB.draw(startX += 72, 672);
					gfx::drawText("Back", startX += 38, 668, 32, 0xFFFFFFFF);
				}
				break;

			case FLD_SEL:
				{
					//Menus
					titleMenu.print(16, 88, 424);
					folderMenu.print(458, 88, 806);
					//Input guide
					unsigned startX = 836;
					buttonA.draw(startX, 672);
					gfx::drawText("Backup", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Restore", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonX.draw(startX += 72, 672);
					gfx::drawText("Delete", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonB.draw(startX += 72, 672);
					gfx::drawText("Back", startX += 38, 668, 32, 0xFFFFFFFF);
				}
				break;
		}
	}

	void drawTitleBar(const std::string& txt)
	{
		titleBar.drawRepeatHori(0, 0, 1280);
		gfx::drawText(txt, 16, 16, 64, 0xFFFFFFFF);
	}

	void showUserMenu(const uint64_t& down, const uint64_t& held)
	{
		userMenu.handleInput(down, held);

		drawMenus();

		if(down & KEY_A)
		{
			data::curUser = data::users[userMenu.getSelected()];
			titleMenuPrepare(data::curUser);

			mstate = TTL_SEL;
		}
		else if(down & KEY_MINUS && sys::devMenu)
		{
			devMenu.reset();
			devMenuPrepare();
			mstate = DEV_MNU;
		}
	}

	void showTitleMenu(const uint64_t& down, const uint64_t& held)
	{
		titleMenu.handleInput(down, held);

		drawMenus();

		if(down & KEY_A)
		{
			data::curData = data::curUser.titles[titleMenu.getSelected()];

			if(fs::mountSave(data::curUser, data::curData))
			{
				util::makeTitleDir(data::curUser, data::curData);

				folderMenuPrepare(data::curUser, data::curData);

				mstate = FLD_SEL;
			}
		}
		else if(down & KEY_B)
			mstate = USR_SEL;

		if(sys::sysSave)
		{
			std::string drawType = "Type: ";
			switch(data::curUser.titles[titleMenu.getSelected()].getType())
			{
				case FsSaveDataType_SystemSaveData:
					drawType += "System Save";
					break;

				case FsSaveDataType_SaveData:
					drawType += "Save Data";
					break;

				case FsSaveDataType_BcatDeliveryCacheStorage:
					drawType += "Bcat Delivery Cache";
					break;

				case FsSaveDataType_DeviceSaveData:
					drawType += "Device Save";
					break;

				case FsSaveDataType_TemporaryStorage:
					drawType += "Temp Storage";
					break;

				case FsSaveDataType_CacheStorage:
					drawType += "Cache Storage";
					break;
			}

			gfx::drawText(drawType, 16, 668, 32, 0xFFFFFFFF);
		}
	}

	void showFolderMenu(const uint64_t& down, const uint64_t& held)
	{
		folderMenu.handleInput(down, held);

		drawMenus();

		if(down & KEY_A)
		{
			if(folderMenu.getSelected() == 0)
			{
				ui::keyboard key;
				std::string folder = key.getString();
				if(!folder.empty())
				{
					std::string path = util::getTitleDir(data::curUser, data::curData) + "/" + folder;
					mkdir(path.c_str(), 777);
					path += "/";

					std::string root = "sv:/";
					fs::copyDirToDir(root, path);

					folderMenuPrepare(data::curUser, data::curData);
				}
			}
			else
			{
				std::string scanPath = util::getTitleDir(data::curUser, data::curData);
				fs::dirList list(scanPath);

				std::string folderName = list.getItem(folderMenu.getSelected() - 1);
				if(confirm("Are you sure you want to overwrite \"" + folderName + "\"?"))
				{
					std::string toPath = util::getTitleDir(data::curUser, data::curData) + folderName + "/";
					std::string root = "sv:/";

					fs::copyDirToDir(root, toPath);
				}
			}
		}
		else if(down & KEY_Y)
		{
			if(data::curData.getType() != FsSaveDataType_SystemSaveData)
			{
				if(folderMenu.getSelected() > 0)
				{
					std::string scanPath = util::getTitleDir(data::curUser, data::curData);
					fs::dirList list(scanPath);

					std::string folderName = list.getItem(folderMenu.getSelected() - 1);
					if(confirm("Are you sure you want to restore \"" + folderName + "\"?"))
					{
						std::string fromPath = util::getTitleDir(data::curUser, data::curData) + folderName + "/";
						std::string root = "sv:/";

						fs::copyDirToDirCommit(fromPath, root, "sv");
					}
				}
			}
			else
				ui::showMessage("Writing data to system save data is not allowed currently. It CAN brick your system.");
		}
		else if(down & KEY_X)
		{
			if(folderMenu.getSelected() > 0)
			{
				std::string scanPath = util::getTitleDir(data::curUser, data::curData);
				fs::dirList list(scanPath);

				std::string folderName = list.getItem(folderMenu.getSelected() - 1);
				if(confirm("Are you sure you want to delete \"" + folderName + "\"?"))
				{
					std::string delPath = scanPath + folderName + "/";
					fs::delDir(delPath);
				}

				folderMenuPrepare(data::curUser, data::curData);
			}
		}
		else if(down & KEY_B)
		{
			fsdevUnmountDevice("sv");
			mstate = TTL_SEL;
		}
	}

	void showDevMenu(const uint64_t& down, const uint64_t& held)
	{
		devMenu.handleInput(down, held);

		if(down & KEY_A)
		{
			switch(devMenu.getSelected())
			{
				case 0:
					ui::progBar userProg(data::users.size());
					for(unsigned i = 0; i < data::users.size(); i++)
					{
						data::curUser = data::users[i];
						for(unsigned j = 0; j < data::curUser.titles.size(); j++)
						{
							data::curData = data::curUser.titles[j];
							if(fs::mountSave(data::curUser, data::curData))
							{
								util::makeTitleDir(data::curUser, data::curData);

								std::string to = util::getTitleDir(data::curUser, data::curData) + util::getDateTime();
								mkdir(to.c_str(), 777);
								to += "/";

								std::string root = "sv:/";

								fs::copyDirToDir(root, to);

								fsdevUnmountDevice("sv");
							}

							userProg.draw("Dumping " + data::curUser.getUsername() + "...");

							gfx::handleBuffs();
						}
					}
					break;
			}
		}
		else if(down & KEY_B)
			mstate = USR_SEL;

		devMenu.print(16, 88, 424);
	}

	void runApp(const uint64_t& down, const uint64_t& held)
	{
		switch(mstate)
		{
			case USR_SEL:
				showUserMenu(down, held);
				break;

			case TTL_SEL:
				showTitleMenu(down, held);
				break;

			case FLD_SEL:
				showFolderMenu(down, held);
				break;

			case DEV_MNU:
				showDevMenu(down, held);
				break;
		}
	}

	void showMessage(const std::string& mess)
	{
		button ok("OK (A)", 480, 464, 320, 96);
		std::string wrapMess = util::getWrappedString(mess, 48, 752);
		while(true)
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
			touchPosition p;
			hidTouchRead(&p, 0);

			if(down & KEY_A || down & KEY_B || ok.released(p))
				break;

			gfx::drawRectangle(256, 128, 768, 464, 0xFFC0C0C0);
			gfx::drawText(wrapMess, 272, 144, 48, 0xFF000000);
			ok.draw();

			gfx::handleBuffs();
		}
	}

	void showError(const std::string& mess, const Result& r)
	{
		button ok("OK (A)", 480, 464, 320, 96);
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

			gfx::drawRectangle(256, 128, 768, 464, 0xFFC0C0C0);
			gfx::drawText(tmp, 272, 144, 48, 0xFF000000);
			ok.draw();

			gfx::handleBuffs();
		}
	}

	bool confirm(const std::string& mess)
	{
		bool ret = false;

		button yes("Yes (A)", 288, 464, 320, 96);
		button no("No (B)", 672, 464, 320, 96);

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

			gfx::drawRectangle(256, 128, 768, 464, 0xFFC0C0C0);
			gfx::drawText(wrapMess, 272, 144, 48, 0xFF000000);
			yes.draw();
			no.draw();

			gfx::handleBuffs();
		}

		return ret;
	}
}

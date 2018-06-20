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
	DEV_MNU,
	ADV_MDE
};

static int mstate = USR_SEL;

static ui::menu userMenu, titleMenu, folderMenu, devMenu, saveMenu, sdMenu, copyMenu;
static gfx::tex buttonA, buttonB, buttonX, buttonY, titleBar;

static const std::string sysSaveMess = "No system saves until we get public NAND restore.";
static std::string savePath, sdPath;
static fs::dirList saveList(""), sdList("sdmc:/");
static int advMenuCtrl = 0, advPrev = 0;

namespace ui
{
	void init()
	{
		buttonA.loadFromFile("romfs:/img/buttonA.data");
		buttonB.loadFromFile("romfs:/img/buttonB.data");
		buttonX.loadFromFile("romfs:/img/buttonX.data");
		buttonY.loadFromFile("romfs:/img/buttonY.data");
		titleBar.loadFromFile("romfs:/img/topbar.data");

		copyMenu.addOpt("Copy From");
		copyMenu.addOpt("Delete");
		copyMenu.addOpt("Rename");
		copyMenu.addOpt("Make Dir");
		copyMenu.addOpt("Back");
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

	void menu::print(const unsigned& x, const unsigned& y, const uint32_t& textClr, const uint32_t& rectWidth)
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

	void drawUI()
	{
		gfx::clearBufferColor(0xFF3B3B3B);
		ui::drawTitleBar("JKSV - 06/20/2018");

		switch(mstate)
		{
			case USR_SEL:
			case TTL_SEL:
			case FLD_SEL:
			case DEV_MNU:
				gfx::drawRectangle(448, 64, 1, 592, 0xFF7B7B7B);
				gfx::drawRectangle(449, 64, 2, 592, 0xFF2B2B2B);

				gfx::drawRectangle(16, 656, 1248, 1, 0xFF7B7B7B);
				gfx::drawRectangle(16, 657, 1248, 2, 0xFF2B2B2B);
				break;

			case ADV_MDE:
				gfx::drawRectangle(624, 64, 1, 592, 0xFF7B7B7B);
				gfx::drawRectangle(625, 64, 2, 592, 0xFF2B2B2B);

				gfx::drawRectangle(16, 656, 1248, 1, 0xFF7B7B7B);
				gfx::drawRectangle(16, 657, 1248, 2, 0xFF2B2B2B);
				break;
		}

		switch(mstate)
		{
			case USR_SEL:
				{
					//Menu
					userMenu.print(16, 88, 0xFFFFFFFF, 424);
					//Input guide
					unsigned startX = 1152;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 668, 32, 0xFFFFFFFF);
				}
				break;

			case TTL_SEL:
				{
					//Menu
					titleMenu.print(16, 88, 0xFFFFFFFF, 424);
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
					titleMenu.print(16, 88, 0xFFFFFFFF, 424);
					folderMenu.print(458, 88, 0xFFFFFFFF, 806);
					//Input guide
					unsigned startX = 726;
					gfx::drawText("- Adv. Mode", startX, 668, 32, 0xFFFFFFFF);
					buttonA.draw(startX += 110, 672);
					gfx::drawText("Backup", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Restore", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonX.draw(startX += 72, 672);
					gfx::drawText("Delete", startX += 38, 668, 32, 0xFFFFFFFF);
					buttonB.draw(startX += 72, 672);
					gfx::drawText("Back", startX += 38, 668, 32, 0xFFFFFFFF);
				}
				break;

			case DEV_MNU:
				devMenu.print(16, 88, 0xFFFFFFFF, 424);
				break;

			case ADV_MDE:
				saveMenu.print(16, 88, 0xFFFFFFFF, 600);
				sdMenu.print(632, 88, 0xFFFFFFFF, 632);

				gfx::drawText(savePath, 16, 668, 32, 0xFFFFFFFF);
				gfx::drawText(sdPath, 632, 668, 32, 0xFFFFFFFF);
				break;
		}
	}

	void drawTitleBar(const std::string& txt)
	{
		titleBar.drawRepeatHoriNoBlend(0, 0, 1280);
		gfx::drawText(txt, 16, 16, 64, 0xFFFFFFFF);
	}

	void showUserMenu(const uint64_t& down, const uint64_t& held)
	{
		userMenu.handleInput(down, held);

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

		if(down & KEY_A)
		{
			if(folderMenu.getSelected() == 0)
			{
				ui::keyboard key;
				std::string folder = key.getString("");
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
		else if(down & KEY_MINUS)
		{
			savePath = "sv:/";
			sdPath   = "sdmc:/";

			saveList.reassign(savePath);
			sdList.reassign(sdPath);

			util::copyDirListToMenu(saveList, saveMenu);
			util::copyDirListToMenu(sdList, sdMenu);

			advMenuCtrl = 0;
			mstate = ADV_MDE;

		}
		else if(down & KEY_B)
		{
			fsdevUnmountDevice("sv");
			mstate = TTL_SEL;
		}
	}

	void performCopyMenuOps()
	{
		switch(copyMenu.getSelected())
		{
			//Copy
			case 0:
				{
					switch(advPrev)
					{
						//save
						case 0:
							if(saveMenu.getSelected() > 0)
							{
								//Forget '..'
								int saveSel = saveMenu.getSelected() - 1;
								if(saveList.isDir(saveSel))
								{
									//Dir we're copying from
									std::string fromPath = savePath + saveList.getItem(saveSel) + "/";

									//Where we're copying to
									std::string toPath = sdPath + saveList.getItem(saveSel);
									//make dir on sd
									mkdir(toPath.c_str(), 777);
									toPath += "/";

									fs::copyDirToDir(fromPath, toPath);
								}
								else
								{
									//just copy file
									std::string fromPath = savePath + saveList.getItem(saveSel);
									std::string toPath = sdPath + saveList.getItem(saveSel);

									fs::copyFile(fromPath, toPath);
								}
							}
							break;

						case 1:
							if(data::curData.getType() != FsSaveDataType_SystemSaveData)
							{
								if(sdMenu.getSelected() > 0)
								{
									//Same as above, but reverse
									int sdSel = sdMenu.getSelected() - 1;
									if(sdList.isDir(sdSel))
									{
										std::string fromPath = sdPath + sdList.getItem(sdSel) + "/";

										std::string toPath = savePath + sdList.getItem(sdSel);
										mkdir(toPath.c_str(), 777);
										toPath += "/";

										fs::copyDirToDirCommit(fromPath, toPath, "sv");
									}
									else
									{
										std::string fromPath = sdPath + sdList.getItem(sdSel);
										std::string toPath = savePath + sdList.getItem(sdSel);

										fs::copyFileCommit(fromPath, toPath, "sv");
									}
								}
							}
							else
								ui::showMessage(sysSaveMess);
							break;

					}
				}
				break;

			//delete
			case 1:
				{
					switch(advPrev)
					{
						//save menu
						case 0:
							if(saveMenu.getSelected() > 0 && data::curData.getType() != FsSaveDataType_SystemSaveData)
							{
								int saveSel = saveMenu.getSelected() - 1;
								if(saveList.isDir(saveSel))
								{
									std::string delPath = savePath + saveList.getItem(saveSel) + "/";
									fs::delDir(delPath);
								}
								else
								{
									std::string delPath = savePath + saveList.getItem(saveSel);
									std::remove(delPath.c_str());
								}
								fsdevCommitDevice("sv");
							}
							break;

						//sd
						case 1:
							if(sdMenu.getSelected() > 0)
							{
								int sdSel = sdMenu.getSelected() - 1;
								if(sdList.isDir(sdSel))
								{
									std::string delPath = sdPath + sdList.getItem(sdSel) + "/";
									fs::delDir(delPath);
								}
								else
								{
									std::string delPath = sdPath + sdList.getItem(sdSel);
									std::remove(delPath.c_str());
								}
							}
							break;
					}
				}
				break;

			//Rename
			case 2:
				switch(advPrev)
				{
					//save
					case 0:
						{
							if(saveMenu.getSelected() > 0 && data::curData.getType() != FsSaveDataType_SystemSaveData)
							{
								int selSave = saveMenu.getSelected() - 1;
								keyboard getName;
								std::string newName = getName.getString(saveList.getItem(selSave));
								if(!newName.empty())
								{
									std::string b4Path = savePath + saveList.getItem(selSave);
									std::string newPath = savePath + newName;

									std::rename(b4Path.c_str(), newPath.c_str());
									fsdevCommitDevice("sv");
								}
							}
						}
						break;

					//sd
					case 1:
						{
							if(sdMenu.getSelected() > 0)
							{
								int sdSel = sdMenu.getSelected() - 1;
								keyboard getName;
								std::string newName = getName.getString(sdList.getItem(sdSel));
								if(!newName.empty())
								{
									std::string b4Path = sdPath + sdList.getItem(sdSel);
									std::string newPath = sdPath + newName;

									std::rename(b4Path.c_str(), newPath.c_str());
								}
							}
						}
						break;
				}
				break;

			//Mkdir
			case 3:
				{
					switch(advPrev)
					{
						//save
						case 0:
							{
								if(data::curData.getType() != FsSaveDataType_SystemSaveData)
								{
									keyboard getFolder;
									std::string newFolder = getFolder.getString("");
									if(!newFolder.empty())
									{
										std::string folderPath = savePath + newFolder;
										mkdir(folderPath.c_str(), 777);
										fsdevCommitDevice("sv");
									}
								}
							}
							break;

						//sd
						case 1:
							{
								keyboard getFolder;
								std::string newFolder = getFolder.getString("");
								if(!newFolder.empty())
								{
									std::string folderPath = sdPath + newFolder;
									mkdir(folderPath.c_str(), 777);
								}
							}
							break;
					}
				}
				break;


			//back
			case 4:
				advMenuCtrl = advPrev;
				break;

		}

		//update lists + menus
		sdList.rescan();
		saveList.rescan();
		util::copyDirListToMenu(sdList, sdMenu);
		util::copyDirListToMenu(saveList, saveMenu);
	}

	void showAdvMode(const uint64_t& down, const uint64_t& held)
	{
		//0 = save; 1 = sd; 2 = cpy
		switch(advMenuCtrl)
		{
			case 0:
				saveMenu.handleInput(down, held);
				break;

			case 1:
				sdMenu.handleInput(down, held);
				break;

			case 2:
				copyMenu.handleInput(down, held);
				break;
		}

		//OH BOY HERE WE GO
		if(down & KEY_A)
		{
			switch(advMenuCtrl)
			{
				//save
				case 0:
					{
						int saveSel = saveMenu.getSelected();
						if(saveSel == 0 && savePath != "sv:/")
						{
							util::removeLastFolderFromString(savePath);
							saveList.reassign(savePath);
							util::copyDirListToMenu(saveList, saveMenu);
						}
						else if(saveSel > 0 && saveList.isDir(saveSel - 1))
						{
							savePath += saveList.getItem(saveSel - 1) + "/";
							saveList.reassign(savePath);
							util::copyDirListToMenu(saveList, saveMenu);
						}
					}
					break;

				//sd
				case 1:
					{
						int sdSel = sdMenu.getSelected();
						if(sdSel == 0 && sdPath != "sdmc:/")
						{
							util::removeLastFolderFromString(sdPath);
							sdList.reassign(sdPath);
							util::copyDirListToMenu(sdList, sdMenu);
						}
						else if(sdSel > 0 && sdList.isDir(sdSel - 1))
						{
							sdPath += sdList.getItem(sdSel - 1) + "/";
							sdList.reassign(sdPath);
							util::copyDirListToMenu(sdList, sdMenu);
						}
					}
					break;

				//advanced mode
				case 2:
					performCopyMenuOps();
					break;
			}
		}
		else if(down & KEY_B)
		{
			//save
			if(advMenuCtrl == 0 && savePath != "sv:/")
			{
				util::removeLastFolderFromString(savePath);
				saveList.reassign(savePath);
				util::copyDirListToMenu(saveList, saveMenu);
			}
			//sd
			else if(advMenuCtrl == 1 && sdPath != "sdmc:/")
			{
				util::removeLastFolderFromString(sdPath);
				sdList.reassign(sdPath);
				util::copyDirListToMenu(sdList, sdMenu);
			}
		}
		else if(down & KEY_X)
		{
			if(advMenuCtrl == 2)
			{
				advMenuCtrl = advPrev;
			}
			else
			{
				advPrev = advMenuCtrl;
				advMenuCtrl = 2;
			}
		}
		else if(down & KEY_ZL || down & KEY_ZR)
		{
			if(advMenuCtrl == 0 || advMenuCtrl == 1)
			{
				if(advMenuCtrl == 0)
					advMenuCtrl = 1;
				else
					advMenuCtrl = 0;
			}
		}
		else if(down & KEY_MINUS)
			mstate = FLD_SEL;

		//draw copy menu if it's supposed to be up
		if(advMenuCtrl == 2)
		{
			gfx::drawRectangle(464, 252, 320, 230, 0xFFC0C0C0);

			switch(advPrev)
			{
				case 0:
					gfx::drawText("SAVE", 468, 256, 32, 0xFF000000);
					break;

				case 1:
					gfx::drawText("SDMC", 468, 256, 32, 0xFF000000);
					break;
			}

			copyMenu.print(468, 294, 0xFF000000, 300);
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
	}

	void runApp(const uint64_t& down, const uint64_t& held)
	{
		drawUI();

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

			case ADV_MDE:
				showAdvMode(down, held);
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

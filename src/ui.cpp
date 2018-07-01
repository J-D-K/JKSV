#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <switch.h>

#include "ui.h"
#include "gfx.h"
#include "util.h"
#include "file.h"

enum menuState
{
	USR_SEL,
	TTL_SEL,
	FLD_SEL,
	ADV_MDE
};

//Menu currently up
int mstate = USR_SEL;

//Button controlled menus
static ui::menu folderMenu, devMenu, saveMenu, sdMenu, copyMenu;

//Bar at top
static gfx::tex buttonA, buttonB, buttonX, buttonY;

//Shown if someone tries to write to system saves
static const std::string sysSaveMess = "No system saves until we get public NAND restore.";

//Advanced mode paths
static std::string savePath, sdPath, saveWrap, sdWrap;
//Advanced mode directory listings
static fs::dirList saveList(""), sdList("");
//Current menu and previous menu for advanced mode.
static int advMenuCtrl = 0, advPrev = 0;

namespace ui
{
	uint32_t clearClr = 0xFFFFFFFF, mnuTxt = 0xFF000000, txtClr = 0xFF000000, \
	                    rectLt = 0xFFC0C0C0, rectSh = 0, tboxClr = 0xFFC0C0C0;

	//textbox pieces
	//I was going to flip them when I draw them, but then laziness kicked in.
	gfx::tex cornerTopLeft, cornerTopRight, cornerBottomLeft, cornerBottomRight, horEdgeTop, horEdgeBot, vertEdgeLeft, vertEdgeRight;

	void init()
	{
		ColorSetId gthm;
		setsysGetColorSetId(&gthm);

		switch(gthm)
		{
			case ColorSetId_Light:
				//Dark corners
				cornerTopLeft.loadFromFile("romfs:/img/tbox/tboxCornerTopLeft_drk.png");
				cornerTopRight.loadFromFile("romfs:/img/tbox/tboxCornerTopRight_drk.png");
				cornerBottomLeft.loadFromFile("romfs:/img/tbox/tboxCornerBotLeft_drk.png");
				cornerBottomRight.loadFromFile("romfs:/img/tbox/tboxCornerBotRight_drk.png");

				//Dark edges
				horEdgeTop.loadFromFile("romfs:/img/tbox/tboxHorEdgeTop_drk.png");
				horEdgeBot.loadFromFile("romfs:/img/tbox/tboxHorEdgeBot_drk.png");
				vertEdgeLeft.loadFromFile("romfs:/img/tbox/tboxVertEdgeLeft_drk.png");
				vertEdgeRight.loadFromFile("romfs:/img/tbox/tboxVertEdgeRight_drk.png");

				clearClr = 0xFFEBEBEB;
				mnuTxt   = 0xFF000000;
				txtClr   = 0xFFFFFFFF;
				rectLt   = 0xFFDFDFDF;
				rectSh   = 0xFFCACACA;
				tboxClr  = 0xFF505050;
				break;

			default:
			case ColorSetId_Dark:
				//Light corners
				cornerTopLeft.loadFromFile("romfs:/img/tbox/tboxCornerTopLeft_lght.png");
				cornerTopRight.loadFromFile("romfs:/img/tbox/tboxCornerTopRight_lght.png");
				cornerBottomLeft.loadFromFile("romfs:/img/tbox/tboxCornerBotLeft_lght.png");
				cornerBottomRight.loadFromFile("romfs:/img/tbox/tboxCornerBotRight_lght.png");

				//light edges
				horEdgeTop.loadFromFile("romfs:/img/tbox/tboxHorEdgeTop_lght.png");
				horEdgeBot.loadFromFile("romfs:/img/tbox/tboxHorEdgeBot_lght.png");
				vertEdgeLeft.loadFromFile("romfs:/img/tbox/tboxVertEdgeLeft_lght.png");
				vertEdgeRight.loadFromFile("romfs:/img/tbox/tboxVertEdgeRight_lght.png");

				clearClr = 0xFF2D2D2D;
				mnuTxt   = 0xFFFFFFFF;
				txtClr   = 0xFF000000;
				rectLt   = 0xFF505050;
				rectSh   = 0xFF202020;
				tboxClr  = 0xFFEBEBEB;
				break;
		}

		buttonA.loadFromFile("romfs:/img/buttonA.png");
		buttonB.loadFromFile("romfs:/img/buttonB.png");
		buttonX.loadFromFile("romfs:/img/buttonX.png");
		buttonY.loadFromFile("romfs:/img/buttonY.png");

		copyMenu.addOpt("Copy From");
		copyMenu.addOpt("Delete");
		copyMenu.addOpt("Rename");
		copyMenu.addOpt("Make Dir");
		copyMenu.addOpt("Properties");
		copyMenu.addOpt("Back");
	}

	void exit()
	{
		cornerTopLeft.deleteData();
		cornerTopRight.deleteData();
		cornerBottomLeft.deleteData();
		cornerBottomRight.deleteData();

		horEdgeTop.deleteData();
		horEdgeBot.deleteData();
		vertEdgeLeft.deleteData();
		vertEdgeRight.deleteData();

		buttonA.deleteData();
		buttonB.deleteData();
		buttonX.deleteData();
		buttonY.deleteData();
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

	void drawTitleBar(const std::string& txt)
	{
		gfx::drawText(txt, 16, 16, 64, mnuTxt);
	}

	void drawUI()
	{
		gfx::clearBufferColor(clearClr);
		ui::drawTitleBar("JKSV - 07/01/2018");

		switch(mstate)
		{
			case FLD_SEL:
				gfx::drawRectangle(448, 64, 1, 592, rectLt);
				gfx::drawRectangle(449, 64, 2, 592, rectSh);

				gfx::drawRectangle(16, 656, 1248, 1, rectLt);
				gfx::drawRectangle(16, 657, 1248, 2, rectSh);
				break;

			case USR_SEL:
			case TTL_SEL:
				gfx::drawRectangle(16, 64, 1248, 1, rectLt);
				gfx::drawRectangle(16, 65, 1248, 2, rectSh);

				gfx::drawRectangle(16, 656, 1248, 1, rectLt);
				gfx::drawRectangle(16, 657, 1248, 2, rectSh);
				break;

			case ADV_MDE:
				gfx::drawRectangle(640, 64, 1, 592, rectLt);
				gfx::drawRectangle(641, 64, 2, 592, rectSh);

				gfx::drawRectangle(16, 656, 1248, 1, rectLt);
				gfx::drawRectangle(16, 657, 1248, 2, rectSh);
				break;
		}

		switch(mstate)
		{
			case USR_SEL:
				{
					//Input guide
					unsigned startX = 1010;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 668, 32, mnuTxt);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Dump All", startX += 38, 668, 32, mnuTxt);
				}
				break;

			case TTL_SEL:
				{
					unsigned startX = 914;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 668, 32, mnuTxt);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Dump All", startX += 38, 668, 32, mnuTxt);
					buttonB.draw(startX += 96, 672);
					gfx::drawText("Back", startX += 38, 668, 32, mnuTxt);
				}
				break;

			case FLD_SEL:
				{
					folderMenu.print(458, 88, mnuTxt, 806);
					//Input guide
					unsigned startX = 726;
					gfx::drawText("- Adv. Mode", startX, 668, 32, mnuTxt);
					buttonA.draw(startX += 110, 672);
					gfx::drawText("Backup", startX += 38, 668, 32, mnuTxt);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Restore", startX += 38, 668, 32, mnuTxt);
					buttonX.draw(startX += 72, 672);
					gfx::drawText("Delete", startX += 38, 668, 32, mnuTxt);
					buttonB.draw(startX += 72, 672);
					gfx::drawText("Back", startX += 38, 668, 32, mnuTxt);
				}
				break;

			case ADV_MDE:
				saveMenu.print(16, 88, mnuTxt, 616);
				sdMenu.print(648, 88, mnuTxt, 616);

				gfx::drawText(saveWrap, 16, 652, 32, mnuTxt);
				gfx::drawText(sdWrap, 656, 652, 32, mnuTxt);
				break;
		}
	}

	void updateUserMenu(const uint64_t& down, const uint64_t& held)
	{
		static int start = 0, selected = 0;

		static uint8_t clrShft = 0;
		static bool clrAdd = true;

		static unsigned selRectX = 64, selRectY = 74;

		if(clrAdd)
		{
			clrShft += 4;
			if(clrShft > 63)
				clrAdd = false;
		}
		else
		{
			clrShft--;
			if(clrShft == 0)
				clrAdd = true;
		}


		if(down & KEY_RIGHT)
		{
			if((unsigned)selected < data::users.size() - 1)
				selected++;

			if(selected >= (int)start + 32)
				start += 8;
		}
		else if(down & KEY_LEFT)
		{
			if(selected > 0)
				selected--;

			if(selected < (int)start)
				start -= 8;
		}
		else if(down & KEY_UP)
		{
			selected -= 8;
			if(selected < 0)
				selected = 0;

			if(selected - start >= 32)
				start -= 8;
		}
		else if(down & KEY_DOWN)
		{
			selected += 8;
			if(selected > (int)data::users.size() - 1)
				selected = data::users.size() - 1;

			if(selected - start >= 32)
				start += 8;
		}
		else if(down & KEY_A)
		{
			data::curUser = data::users[selected];
			//Reset this
			start = 0;
			selected = 0;
			selRectX = 64, selRectY = 74;
			mstate = TTL_SEL;
		}
		else if(down & KEY_Y)
		{
			for(unsigned i = 0; i < data::users.size(); i++)
				fs::dumpAllUserSaves(data::users[i]);
		}

		unsigned x = 70, y = 80;
		unsigned endUser = start + 32;
		if(start + 32 > (int)data::users.size())
			endUser = data::users.size();

		uint32_t rectClr = 0xFF << 24 | ((0xBB + clrShft) & 0xFF) << 16 | ((0x60 + clrShft)) << 8 | 0x00;
		gfx::drawRectangle(selRectX, selRectY, 140, 140, rectClr);

		for(unsigned i = start; i < endUser; y += 144)
		{
			unsigned endRow = i + 8;
			for(unsigned tX = x; i < endRow; i++, tX += 144)
			{
				if(i == endUser)
					break;

				if((int)i == selected)
				{
					if(selRectX != tX - 6)
					{
						if(selRectX < tX - 6)
							selRectX += 24;
						else
							selRectX -= 24;
					}

					if(selRectY != y - 6)
					{
						if(selRectY < y - 6)
							selRectY += 24;
						else
							selRectX -= 24;
					}

					std::string username = data::users[selected].getUsername();
					unsigned userWidth = gfx::getTextWidth(username, 32);
					int userRectWidth = userWidth + 32, userRectX = (tX + 64) - (userRectWidth  / 2);
					if(userRectX < 16)
						userRectX = 16;

					if(userRectX + userRectWidth > 1264)
						userRectX = 1264 - userRectWidth;

					drawTextbox(userRectX, y - 50, userRectWidth, 38);
					gfx::drawText(username, userRectX + 16, y - 50, 32, txtClr);
				}
				data::users[i].icn.drawNoBlendSkip(tX, y);
			}
		}
	}

	void updateTitleMenu(const uint64_t& down, const uint64_t& held)
	{
		//Static vars so they don't change on every loop
		//Where to start in titles, selected title
		static int start = 0, selected = 0;

		//Color shift for rect
		static uint8_t clrShft = 0;
		//Whether or not we're adding or subtracting from clrShft
		static bool clrAdd = true;

		//Selected rectangle X and Y.
		static unsigned selRectX = 64, selRectY = 74;

		if(clrAdd)
		{
			clrShft += 4;
			if(clrShft > 63)
				clrAdd = false;
		}
		else
		{
			clrShft--;
			if(clrShft == 0)
				clrAdd = true;
		}

		if(down & KEY_RIGHT)
		{
			if(selected < (int)data::curUser.titles.size() - 1)
				selected++;

			if(selected >= (int)start + 32)
				start += 8;
		}
		else if(down & KEY_LEFT)
		{
			if(selected > 0)
				selected--;

			if(selected < (int)start)
				start -= 8;
		}
		else if(down & KEY_UP)
		{
			selected -= 8;
			if(selected < 0)
				selected = 0;

			if(selected < start)
				start -= 8;
		}
		else if(down & KEY_DOWN)
		{
			selected += 8;
			if(selected > (int)data::curUser.titles.size() - 1)
				selected = data::curUser.titles.size() - 1;

			if(selected - start >= 32)
				start += 8;
		}
		else if(down & KEY_A)
		{
			data::curData = data::curUser.titles[selected];
			if(fs::mountSave(data::curUser, data::curData))
			{
				util::makeTitleDir(data::curUser, data::curData);
				folderMenuPrepare(data::curUser, data::curData);
				mstate = FLD_SEL;
			}
		}
		else if(down & KEY_Y)
		{
			fs::dumpAllUserSaves(data::curUser);
		}
		else if(down & KEY_B)
		{
			start = 0;
			selected = 0;
			selRectX = 64;
			selRectY = 74;
			mstate = USR_SEL;
			return;
		}

		unsigned x = 70, y = 80;

		unsigned endTitle = start + 32;
		if(start + 32 > (int)data::curUser.titles.size())
			endTitle = data::curUser.titles.size();

		//draw Rect so it's always behind icons
		uint32_t rectClr = 0xFF << 24 | ((0xBB + clrShft) & 0xFF) << 16 | ((0x60 + clrShft)) << 8 | 0x00;
		gfx::drawRectangle(selRectX, selRectY, 140, 140, rectClr);

		for(unsigned i = start; i < endTitle; y += 144)
		{
			unsigned endRow = i + 8;
			for(unsigned tX = x; i < endRow; i++, tX += 144)
			{
				if(i == endTitle)
					break;

				if((int)i == selected)
				{
					//Most Switch icons seem to be 256x256, we're drawing them 128x128
					if(selRectX != tX - 6)
					{
						if(selRectX < tX - 6)
							selRectX += 24;
						else
							selRectX -= 24;
					}

					if(selRectY != y - 6)
					{
						if(selRectY < y - 6)
							selRectY += 24;
						else
							selRectY -= 24;
					}

					std::string title = data::curUser.titles[selected].getTitle();
					unsigned titleWidth = gfx::getTextWidth(title, 32);
					int rectWidth = titleWidth + 32, rectX = (tX + 64) - (rectWidth / 2);
					if(rectX < 16)
						rectX = 16;

					if(rectX + rectWidth > 1264)
						rectX = 1264 - rectWidth;

					drawTextbox(rectX, y - 50, rectWidth, 38);
					gfx::drawText(title, rectX + 16, y - 50, 32, txtClr);
				}
				data::curUser.titles[i].icon.drawHalf(tX, y);
			}
		}

	}

	void updateFolderMenu(const uint64_t& down, const uint64_t& held)
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

			saveWrap = "sv:/";
			sdWrap   = "sdmc:/";

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

	//Performs copy menu operations. To big to stuff into case IMO.
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
							if(saveMenu.getSelected() == 0)
							{
								//Copy current open dir
								if(confirmTransfer(savePath, sdPath))
									fs::copyDirToDir(savePath, sdPath);
							}
							else if(saveMenu.getSelected() > 1)
							{
								//Forget '..'
								int saveSel = saveMenu.getSelected() - 2;
								if(saveList.isDir(saveSel))
								{
									//Dir we're copying from
									std::string fromPath = savePath + saveList.getItem(saveSel) + "/";

									//Where we're copying to
									std::string toPath = sdPath + saveList.getItem(saveSel);
									if(confirmTransfer(fromPath, toPath))
									{
										//make dir on sd
										mkdir(toPath.c_str(), 777);
										toPath += "/";

										fs::copyDirToDir(fromPath, toPath);
									}
								}
								else
								{
									//just copy file
									std::string fromPath = savePath + saveList.getItem(saveSel);
									std::string toPath = sdPath + saveList.getItem(saveSel);
									if(confirmTransfer(fromPath, toPath))
										fs::copyFile(fromPath, toPath);
								}
							}
							break;

						case 1:
							if(data::curData.getType() != FsSaveDataType_SystemSaveData)
							{
								if(sdMenu.getSelected() == 0)
								{
									if(confirmTransfer(sdPath, savePath))
										fs::copyDirToDirCommit(sdPath, savePath, "sv");
								}
								else if(sdMenu.getSelected() > 1)
								{
									//Same as above, but reverse
									int sdSel = sdMenu.getSelected() - 2;
									if(sdList.isDir(sdSel))
									{
										std::string fromPath = sdPath + sdList.getItem(sdSel) + "/";

										std::string toPath = savePath + sdList.getItem(sdSel);

										if(confirmTransfer(fromPath, toPath))
										{
											mkdir(toPath.c_str(), 777);
											toPath += "/";

											fs::copyDirToDirCommit(fromPath, toPath, "sv");
										}
									}
									else
									{
										std::string fromPath = sdPath + sdList.getItem(sdSel);
										std::string toPath = savePath + sdList.getItem(sdSel);
										if(confirmTransfer(fromPath, toPath))
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
							if(data::curData.getType() != FsSaveDataType_SystemSaveData)
							{
								if(saveMenu.getSelected() == 0)
								{
									if(confirmDelete(savePath))
									{
										fs::delDir(savePath);
										fsdevCommitDevice("sv");
									}
								}
								else if(saveMenu.getSelected() > 1)
								{
									int saveSel = saveMenu.getSelected() - 2;
									if(saveList.isDir(saveSel))
									{
										std::string delPath = savePath + saveList.getItem(saveSel) + "/";
										if(confirmDelete(delPath))
											fs::delDir(delPath);
									}
									else
									{
										std::string delPath = savePath + saveList.getItem(saveSel);
										if(confirmDelete(delPath))
											std::remove(delPath.c_str());
									}
									fsdevCommitDevice("sv");
								}
							}
							break;

						//sd
						case 1:
							if(sdMenu.getSelected() == 0)
							{
								if(confirmDelete(sdPath) && sdPath != "sdmc:/")
									fs::delDir(sdPath);
							}
							else if(sdMenu.getSelected() > 1)
							{
								int sdSel = sdMenu.getSelected() - 2;
								if(sdList.isDir(sdSel))
								{
									std::string delPath = sdPath + sdList.getItem(sdSel) + "/";
									if(confirmDelete(delPath))
										fs::delDir(delPath);
								}
								else
								{
									std::string delPath = sdPath + sdList.getItem(sdSel);
									if(confirmDelete(delPath))
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
							if(saveMenu.getSelected() > 1 && data::curData.getType() != FsSaveDataType_SystemSaveData)
							{
								int selSave = saveMenu.getSelected() - 2;
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
							if(sdMenu.getSelected() > 1)
							{
								int sdSel = sdMenu.getSelected() - 2;
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
									std::string newFolder = util::safeString(getFolder.getString(""));
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

			//Props
			case 4:
				{
					switch(advPrev)
					{
						//sv
						case 0:
							{
								if(saveMenu.getSelected() > 1)
								{
									int sel = saveMenu.getSelected() - 2;
									std::string fullPath = savePath + saveList.getItem(sel);
									std::string props = fs::getFileProps(fullPath);
									if(!props.empty())
										showMessage(props);
								}
							}
							break;

						case 1:
							{
								if(sdMenu.getSelected() > 1)
								{
									int sel = sdMenu.getSelected() - 2;
									std::string fullPath = sdPath + sdList.getItem(sel);
									std::string props = fs::getFileProps(fullPath);
									if(!props.empty())
										showMessage(props);
								}
							}
							break;
					}
				}
				break;

			//back
			case 5:
				advMenuCtrl = advPrev;
				break;

		}

		//update lists + menus
		sdList.rescan();
		saveList.rescan();
		util::copyDirListToMenu(sdList, sdMenu);
		util::copyDirListToMenu(saveList, saveMenu);
	}

	void updateAdvMode(const uint64_t& down, const uint64_t& held)
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
						if(saveSel == 1 && savePath != "sv:/")
						{
							util::removeLastFolderFromString(savePath);
							saveWrap = util::getWrappedString(savePath, 32, 600);

							saveList.reassign(savePath);
							util::copyDirListToMenu(saveList, saveMenu);
						}
						else if(saveSel > 1 && saveList.isDir(saveSel - 2))
						{
							savePath += saveList.getItem(saveSel - 2) + "/";
							saveWrap = util::getWrappedString(savePath, 32, 600);

							saveList.reassign(savePath);
							util::copyDirListToMenu(saveList, saveMenu);
						}
					}
					break;

				//sd
				case 1:
					{
						int sdSel = sdMenu.getSelected();
						if(sdSel == 1 && sdPath != "sdmc:/")
						{
							util::removeLastFolderFromString(sdPath);
							sdWrap = util::getWrappedString(sdPath, 32, 600);

							sdList.reassign(sdPath);
							util::copyDirListToMenu(sdList, sdMenu);
						}
						else if(sdSel > 1 && sdList.isDir(sdSel - 2))
						{
							sdPath += sdList.getItem(sdSel - 2) + "/";
							sdWrap  = util::getWrappedString(sdPath, 32, 600);

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
				saveWrap = util::getWrappedString(savePath, 32, 600);

				saveList.reassign(savePath);
				util::copyDirListToMenu(saveList, saveMenu);
			}
			//sd
			else if(advMenuCtrl == 1 && sdPath != "sdmc:/")
			{
				util::removeLastFolderFromString(sdPath);
				sdWrap = util::getWrappedString(sdPath, 32, 600);

				sdList.reassign(sdPath);
				util::copyDirListToMenu(sdList, sdMenu);
			}
			else if(advMenuCtrl == 2)
				advMenuCtrl = advPrev;
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
			ui::drawTextbox(464, 236, 320, 268);

			switch(advPrev)
			{
				case 0:
					gfx::drawText("SAVE", 472, 242, 32,txtClr);
					break;

				case 1:
					gfx::drawText("SDMC", 472, 242, 32, txtClr);
					break;
			}

			copyMenu.print(472, 278, txtClr, 304);
		}
	}

	void runApp(const uint64_t& down, const uint64_t& held)
	{
		//Draw first. Shouldn't, but it simplifies the showX functions
		drawUI();

		switch(mstate)
		{
			case USR_SEL:
				updateUserMenu(down, held);
				break;

			case TTL_SEL:
				updateTitleMenu(down, held);
				break;

			case FLD_SEL:
				updateFolderMenu(down, held);
				break;

			case ADV_MDE:
				updateAdvMode(down, held);
				break;
		}
	}
}

#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <switch.h>

#include "ui.h"
#include "gfx.h"
#include "util.h"
#include "file.h"

#define TITLE_TEXT "JKSV - 07/11/2018"

//Secret background that can be drawn from ".JKSV/back.jpg"
static gfx::tex background;

namespace ui
{
	//Classic mode
	bool clsMode = false;

	//Current menu state
	int mstate = USR_SEL;

	//Info printed on folder menu
	std::string folderMenuInfo;

	//Touch button vector
	std::vector<ui::button> selButtons;

	//UI colors 0xAABBGGRR
	uint32_t clearClr = 0xFFFFFFFF, mnuTxt = 0xFF000000, txtClr = 0xFF000000, \
	                    rectLt = 0xFFC0C0C0, rectSh = 0, tboxClr = 0xFFC0C0C0;

	//textbox pieces
	//I was going to flip them when I draw them, but then laziness kicked in.
	gfx::tex cornerTopLeft, cornerTopRight, cornerBottomLeft, cornerBottomRight, \
	horEdgeTop, horEdgeBot, vertEdgeLeft, vertEdgeRight;

	gfx::tex buttonA, buttonB, buttonX, buttonY, buttonMin;

	void init()
	{
		ColorSetId gthm;
		setsysGetColorSetId(&gthm);

		switch(gthm)
		{
			case ColorSetId_Light:
				//Dark corners
				cornerTopLeft.loadPNGFile("romfs:/img/tbox/tboxCornerTopLeft_drk.png");
				cornerTopRight.loadPNGFile("romfs:/img/tbox/tboxCornerTopRight_drk.png");
				cornerBottomLeft.loadPNGFile("romfs:/img/tbox/tboxCornerBotLeft_drk.png");
				cornerBottomRight.loadPNGFile("romfs:/img/tbox/tboxCornerBotRight_drk.png");

				//Dark edges
				horEdgeTop.loadPNGFile("romfs:/img/tbox/tboxHorEdgeTop_drk.png");
				horEdgeBot.loadPNGFile("romfs:/img/tbox/tboxHorEdgeBot_drk.png");
				vertEdgeLeft.loadPNGFile("romfs:/img/tbox/tboxVertEdgeLeft_drk.png");
				vertEdgeRight.loadPNGFile("romfs:/img/tbox/tboxVertEdgeRight_drk.png");

				//Dark buttons
				buttonA.loadPNGFile("romfs:/img/button/buttonA_drk.png");
				buttonB.loadPNGFile("romfs:/img/button/buttonB_drk.png");
				buttonX.loadPNGFile("romfs:/img/button/buttonX_drk.png");
				buttonY.loadPNGFile("romfs:/img/button/buttonY_drk.png");
				buttonMin.loadPNGFile("romfs:/img/button/buttonMin_drk.png");

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
				cornerTopLeft.loadPNGFile("romfs:/img/tbox/tboxCornerTopLeft_lght.png");
				cornerTopRight.loadPNGFile("romfs:/img/tbox/tboxCornerTopRight_lght.png");
				cornerBottomLeft.loadPNGFile("romfs:/img/tbox/tboxCornerBotLeft_lght.png");
				cornerBottomRight.loadPNGFile("romfs:/img/tbox/tboxCornerBotRight_lght.png");

				//light edges
				horEdgeTop.loadPNGFile("romfs:/img/tbox/tboxHorEdgeTop_lght.png");
				horEdgeBot.loadPNGFile("romfs:/img/tbox/tboxHorEdgeBot_lght.png");
				vertEdgeLeft.loadPNGFile("romfs:/img/tbox/tboxVertEdgeLeft_lght.png");
				vertEdgeRight.loadPNGFile("romfs:/img/tbox/tboxVertEdgeRight_lght.png");

				//Light buttons
				buttonA.loadPNGFile("romfs:/img/button/buttonA_lght.png");
				buttonB.loadPNGFile("romfs:/img/button/buttonB_lght.png");
				buttonX.loadPNGFile("romfs:/img/button/buttonX_lght.png");
				buttonY.loadPNGFile("romfs:/img/button/buttonY_lght.png");
				buttonMin.loadPNGFile("romfs:/img/button/buttonMin_lght.png");

				clearClr = 0xFF2D2D2D;
				mnuTxt   = 0xFFFFFFFF;
				txtClr   = 0xFF000000;
				rectLt   = 0xFF505050;
				rectSh   = 0xFF202020;
				tboxClr  = 0xFFEBEBEB;
				break;
		}

		setupSelButtons();

		if(fs::fileExists("back.jpg"))
			background.loadJpegFile("back.jpg");

		if(fs::fileExists("cls.txt"))
		{
			clsUserPrep();
			clsMode = true;
			mstate = CLS_USR;
		}

		advCopyMenuPrep();
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

		background.deleteData();
	}

	void setupSelButtons()
	{
		int x = 70, y = 80;
		for(int i = 0; i < 32; y += 144)
		{
			int endRow = i + 8;
			for(int tX = x; i < endRow; tX += 144, i++)
			{
				//Make a new button with no text. We're not drawing them anyway
				ui::button newSelButton("", tX, y, 128, 128);
				selButtons.push_back(newSelButton);
			}
		}
	}

	void drawUI()
	{
		if(background.getDataPointer() == NULL)
			gfx::clearBufferColor(clearClr);
		else
			background.drawNoBlend(0, 0);

		gfx::drawText(TITLE_TEXT, 16, 16, 32, mnuTxt);

		switch(mstate)
		{
			case FLD_SEL:
				gfx::drawRectangle(16, 64, 1248, 1, rectLt);
				gfx::drawRectangle(16, 65, 1248, 2, rectSh);

				gfx::drawRectangle(288, 65, 1, 592, rectLt);
				gfx::drawRectangle(289, 65, 2, 592, rectSh);

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
				gfx::drawRectangle(16, 64, 1248, 1, rectLt);
				gfx::drawRectangle(16, 65, 1248, 2, rectSh);

				gfx::drawRectangle(640, 64, 1, 592, rectLt);
				gfx::drawRectangle(641, 64, 2, 592, rectSh);

				gfx::drawRectangle(16, 656, 1248, 1, rectLt);
				gfx::drawRectangle(16, 657, 1248, 2, rectSh);
				break;

			case CLS_TTL:
			case CLS_USR:
				gfx::drawRectangle(16, 64, 1248, 1, rectLt);
				gfx::drawRectangle(16, 65, 1248, 2, rectSh);

				gfx::drawRectangle(448, 64, 1, 592, rectLt);
				gfx::drawRectangle(449, 64, 2, 592, rectSh);

				gfx::drawRectangle(16, 656, 1248, 1, rectLt);
				gfx::drawRectangle(16, 657, 1248, 2, rectSh);
				break;
		}

		switch(mstate)
		{
			case USR_SEL:
			case CLS_USR:
				{
					//Input guide
					unsigned startX = 848;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 680, 14, mnuTxt);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Dump All", startX += 38, 680, 14, mnuTxt);
					buttonX.draw(startX += 96, 672);
					gfx::drawText("Classic Mode", startX += 38, 680, 14, mnuTxt);
				}
				break;

			case TTL_SEL:
			case CLS_TTL:
				{
					unsigned startX = 914;
					buttonA.draw(startX, 672);
					gfx::drawText("Select", startX += 38, 680, 14, mnuTxt);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Dump All", startX += 38, 680, 14, mnuTxt);
					buttonB.draw(startX += 96, 672);
					gfx::drawText("Back", startX += 38, 680, 14, mnuTxt);
				}
				break;

			case FLD_SEL:
				{
					//Input guide
					unsigned startX = 690;
					buttonMin.draw(startX, 672);
					gfx::drawText("Adv. Mode", startX += 38, 680, 14, mnuTxt);
					buttonA.draw(startX += 100, 672);
					gfx::drawText("Backup", startX += 38, 680, 14, mnuTxt);
					buttonY.draw(startX += 72, 672);
					gfx::drawText("Restore", startX += 38, 680, 14, mnuTxt);
					buttonX.draw(startX += 72, 672);
					gfx::drawText("Delete", startX += 38, 680, 14, mnuTxt);
					buttonB.draw(startX += 72, 672);
					gfx::drawText("Back", startX += 38, 680, 14, mnuTxt);
				}
				break;
		}
	}

	void runApp(const uint64_t& down, const uint64_t& held, const touchPosition& p)
	{
		//Draw first. Shouldn't, but it simplifies the showX functions
		drawUI();

		switch(mstate)
		{
			case USR_SEL:
				updateUserMenu(down, held, p);
				break;

			case TTL_SEL:
				updateTitleMenu(down, held, p);
				break;

			case FLD_SEL:
				updateFolderMenu(down, held, p);
				break;

			case ADV_MDE:
				updateAdvMode(down, held, p);
				break;

			case CLS_USR:
				classicUserMenuUpdate(down, held, p);
				break;

			case CLS_TTL:
				classicTitleMenuUpdate(down, held, p);
				break;
		}
	}
}

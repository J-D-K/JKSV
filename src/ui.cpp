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

#define VER_STRING "v. 01/08/2019"

//background that can be drawn from "/JKSV/back.jpg"
//txtSide and fldSide are to fake alpha blending so the framerate doesn't suffer
static tex *background = NULL, *txtSide = NULL, *fldSide = NULL;

//Nav buttons
std::vector<ui::button> usrNav, ttlNav, fldNav;

namespace ui
{
    //Classic mode
    bool clsMode = false;

    //Current menu state
    int mstate = USR_SEL, prevState = USR_SEL;

    //Info printed on folder menu
    std::string folderMenuInfo;

    //Touch button vector
    std::vector<ui::button> selButtons;

    //UI colors
    clr clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr, sideRect, divClr;

    //textbox pieces
    //I was going to flip them when I draw them, but then laziness kicked in.
    tex *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;

    tex *buttonA, *buttonB, *buttonX, *buttonY, *buttonMin;

    tex *selBox, *icn, *mnuGrad;

    font *shared;

    void init()
    {
        ColorSetId gthm;
        setsysGetColorSetId(&gthm);

        switch(gthm)
        {
            case ColorSetId_Light:
                //Dark corners
                cornerTopLeft = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerTopLeft.png");
                cornerTopRight = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerTopRight.png");
                cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerBotLeft.png");
                cornerBottomRight = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerBotRight.png");

                //Dark buttons
                buttonA = texLoadPNGFile("romfs:/img/button/buttonA_drk.png");
                buttonB = texLoadPNGFile("romfs:/img/button/buttonB_drk.png");
                buttonX = texLoadPNGFile("romfs:/img/button/buttonX_drk.png");
                buttonY = texLoadPNGFile("romfs:/img/button/buttonY_drk.png");
                buttonMin = texLoadPNGFile("romfs:/img/button/buttonMin_drk.png");

                icn = texLoadPNGFile("romfs:/img/icn/icnDrk.png");
                mnuGrad = texLoadPNGFile("romfs:/img/mnu/gradLght.png");

                clearClr = clrCreateU32(0xFFEBEBEB);
                mnuTxt = clrCreateU32(0xFF282828);
                txtClr = clrCreateU32(0xFFFFFFFF);
                rectLt = clrCreateU32(0xFFDFDFDF);
                rectSh = clrCreateU32(0xFFCACACA);
                tboxClr = clrCreateU32(0xFF505050);
                sideRect = clrCreateU32(0xFFDCDCDC);
                divClr = clrCreateU32(0xFF2D2D2D);
                break;

            default:
            case ColorSetId_Dark:
                //Light corners
                cornerTopLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
                cornerTopRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
                cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
                cornerBottomRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotRight.png");

                //Light buttons
                buttonA = texLoadPNGFile("romfs:/img/button/buttonA_lght.png");
                buttonB = texLoadPNGFile("romfs:/img/button/buttonB_lght.png");
                buttonX = texLoadPNGFile("romfs:/img/button/buttonX_lght.png");
                buttonY = texLoadPNGFile("romfs:/img/button/buttonY_lght.png");
                buttonMin = texLoadPNGFile("romfs:/img/button/buttonMin_lght.png");
	
                icn = texLoadPNGFile("romfs:/img/icn/icnLght.png");
                mnuGrad = texLoadPNGFile("romfs:/img/mnu/gradDrk.png");

                clearClr = clrCreateU32(0xFF2D2D2D);
                mnuTxt = clrCreateU32(0xFFFFFFFF);
                txtClr = clrCreateU32(0xFF000000);
                rectLt = clrCreateU32(0xFF505050);
                rectSh = clrCreateU32(0xFF202020);
                tboxClr = clrCreateU32(0xFFEBEBEB);
                sideRect = clrCreateU32(0xFF373737);
                divClr = clrCreateU32(0xFFFFFFFF);
                break;
        }

        if(fs::fileExists(fs::getWorkDir() + "font.ttf"))
            shared = fontLoadTTF(std::string(fs::getWorkDir() + "font.ttf").c_str());
        else
            shared = fontLoadSharedFonts();

        if(fs::fileExists(fs::getWorkDir() + "cls.txt"))
        {
            clsUserPrep();
            clsMode = true;
            mstate = CLS_USR;
        }

        setupSelButtons();
        setupNavButtons();

        selBox = texLoadPNGFile("romfs:/img/icn/icnSelBox.png");

        if(fs::fileExists(fs::getWorkDir() + "back.jpg"))
        {
            background = texLoadJPEGFile(std::string(fs::getWorkDir() + "back.jpg").c_str());
            //Fake alpha Rects
            fldSide = texCreateFromPart(background, 0, 88, 410, 559);
            clr tempRect = sideRect;
            tempRect.a = 0xAA;
            drawRectAlpha(fldSide, 0, 0, 410, 559, tempRect);

            txtSide = texCreateFromPart(background, 0, 88, 448, 559);
            drawRectAlpha(txtSide, 0, 0, 448, 559, tempRect);
        }
        menuPrepGfx();

        advCopyMenuPrep();
    }

    void exit()
    {
        texDestroy(cornerTopLeft);
        texDestroy(cornerTopRight);
        texDestroy(cornerBottomLeft);
        texDestroy(cornerBottomRight);

        texDestroy(buttonA);
        texDestroy(buttonB);
        texDestroy(buttonX);
        texDestroy(buttonY);
        texDestroy(buttonMin);

        texDestroy(selBox);
        texDestroy(mnuGrad);

        menuDestGfx();

        if(background != NULL)
            texDestroy(background);
        if(fldSide != NULL)
            texDestroy(fldSide);
        if(txtSide != NULL)
            texDestroy(txtSide);

        fontDestroy(shared);
    }

    void setupSelButtons()
    {
        int x = 70, y = 98;
        for(int i = 0; i < 18; y += 184)
        {
            int endRow = i + 8;
            for(int tX = x; i < endRow; tX += 184, i++)
            {
                //Make a new button with no text. We're not drawing them anyway
                ui::button newSelButton("", tX, y, 174, 174);
                selButtons.push_back(newSelButton);
            }
        }
    }

    void setupNavButtons()
    {
        //User Select
        int startX = 754;
        ui::button sel("", startX, 656, 110, 64);
        ui::button dmp("", startX += 110, 656, 134, 64);
        ui::button cls("", startX += 134, 656, 110, 64);
        ui::button ex("", startX += 110, 656, 110, 64);
        usrNav.push_back(sel);
        usrNav.push_back(dmp);
        usrNav.push_back(cls);
        usrNav.push_back(ex);

        //Title
        startX = 804;
        ui::button ttlSel("", startX, 656, 110, 64);
        ui::button ttlDmp("", startX += 110, 656, 134, 64);
        ui::button ttlBlk("", startX += 134, 656, 110, 64);
        ui::button ttlBck("", startX += 110, 656, 110, 64);
        ttlNav.push_back(ttlSel);
        ttlNav.push_back(ttlDmp);
        ttlNav.push_back(ttlBlk);
        ttlNav.push_back(ttlBck);

        //Folder. Skip adv since it can't be touch controlled
        startX = 800;
        ui::button fldBackup("", startX, 656, 110, 64);
        ui::button fldRestor("", startX += 110, 656, 110, 64);
        ui::button fldDelete("", startX += 110, 656, 110, 64);
        ui::button fldBack("", startX += 110, 672, 110, 64);
        fldNav.push_back(fldBackup);
        fldNav.push_back(fldRestor);
        fldNav.push_back(fldDelete);
        fldNav.push_back(fldBack);
    }

    void drawUI()
    {
        if(background == NULL)
            texClearColor(frameBuffer, clearClr);
        else
            texDrawNoAlpha(background, frameBuffer, 0, 0);

        texDraw(icn, frameBuffer, 66, 27);
        drawText("JKSV", frameBuffer, shared, 130, 38, 24, mnuTxt);
        drawText(VER_STRING, frameBuffer, shared, 8, 702, 12, mnuTxt);
        drawRect(frameBuffer, 30, 87, 1220, 1, divClr);
        drawRect(frameBuffer, 30, 647, 1220, 1, divClr);

        switch(mstate)
        {
            case FLD_SEL:
                if(fldSide == NULL)
                    //drawRect(frameBuffer, 0, 88, 410, 559, sideRect);
                    texDraw(mnuGrad, frameBuffer, 0, 88);
                else
                    texDraw(fldSide, frameBuffer, 30, 88);
                break;

            case ADV_MDE:
                drawRect(frameBuffer, 640, 87, 1, 561, divClr);
                break;

            case CLS_TTL:
            case CLS_USR:
            case CLS_FLD:
            case EX_MNU:
                if(txtSide == NULL)
                    //drawRect(frameBuffer, 30, 88, 448, 559, sideRect);
                    texDraw(mnuGrad, frameBuffer, 0, 88);
                else
                    texDraw(txtSide, frameBuffer, 30, 88);
                break;
        }

        switch(mstate)
        {
            case USR_SEL:
            case CLS_USR:
                {
                    //Input guide
                    unsigned startX = 0;
					if(ui::clsMode)
                        startX = 581;
					else 
                        startX = 574;
                    texDraw(buttonA, frameBuffer, startX, 672);
                    drawText("Select", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonY, frameBuffer, startX += 110, 672);
                    drawText("Dump All", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonX, frameBuffer, startX += 148, 672);
                    if(ui::clsMode) {
                        drawText("GUI Mode", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                        texDraw(buttonMin, frameBuffer, startX += 158, 672);
                    } else {
                        drawText("Text Mode", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                        texDraw(buttonMin, frameBuffer, startX += 165, 672);
					}
                    drawText("Extras", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                }
                break;

            case TTL_SEL:
            case CLS_TTL:
                {
                    unsigned startX = 619;
                    texDraw(buttonA, frameBuffer, startX, 672);
                    drawText("Select", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonY, frameBuffer, startX += 110, 672);
                    drawText("Dump All", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonX, frameBuffer, startX += 148, 672);
                    drawText("Blacklist", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonB, frameBuffer, startX += 134, 672);
                    drawText("Back", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                }
                break;

            case FLD_SEL:
            case CLS_FLD:
                {
                    //Input guide
                    unsigned startX = 440;
                    texDraw(buttonMin, frameBuffer, startX, 672);
                    drawText("Adv. Mode", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonA, frameBuffer, startX += 165, 672);
                    drawText("Backup", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonY, frameBuffer, startX += 125, 672);
                    drawText("Restore", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonX, frameBuffer, startX += 126, 672);
                    drawText("Delete", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
                    texDraw(buttonB, frameBuffer, startX += 117, 672);
                    drawText("Back", frameBuffer, shared, startX += 38, 675, 18, mnuTxt);
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

            case CLS_FLD:
                classicFolderMenuUpdate(down, held, p);
                break;

            case EX_MNU:
                updateExMenu(down, held, p);
                break;
        }
    }
}

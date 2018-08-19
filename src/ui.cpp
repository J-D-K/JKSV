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

#define TITLE_TEXT "JKSV - 08/18/2018"

//background that can be drawn from "/JKSV/back.jpg"
static tex *background = NULL;

//Nav buttons
std::vector<ui::button> usrNav, ttlNav, fldNav;

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

    //UI colors
    clr clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr, sideRect;

    //textbox pieces
    //I was going to flip them when I draw them, but then laziness kicked in.
    tex *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;

    tex *buttonA, *buttonB, *buttonX, *buttonY, *buttonMin;

    tex *selBox;

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

                clearClr = clrCreateU32(0xFFEBEBEB);
                mnuTxt = clrCreateU32(0xFF000000);
                txtClr = clrCreateU32(0xFFFFFFFF);
                rectLt = clrCreateU32(0xFFDFDFDF);
                rectSh = clrCreateU32(0xFFCACACA);
                tboxClr = clrCreateU32(0xFF505050);
                sideRect = clrCreateU32(0xFFDCDCDC);
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

                clearClr = clrCreateU32(0xFF2D2D2D);
                mnuTxt = clrCreateU32(0xFFFFFFFF);
                txtClr = clrCreateU32(0xFF000000);
                rectLt = clrCreateU32(0xFF505050);
                rectSh = clrCreateU32(0xFF202020);
                tboxClr = clrCreateU32(0xFFEBEBEB);
                sideRect = clrCreateU32(0xFF373737);
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
            background = texLoadJPEGFile(std::string(fs::getWorkDir() + "back.jpg").c_str());

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

        if(background != NULL)
            texDestroy(background);

        fontDestroy(shared);
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

    void setupNavButtons()
    {
        //User Select
        int startX = 848;
        ui::button sel("", startX, 656, 110, 64);
        ui::button dmp("", startX += 110, 656, 134, 64);
        ui::button cls("", startX += 134, 656, 110, 64);
        usrNav.push_back(sel);
        usrNav.push_back(dmp);
        usrNav.push_back(cls);

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

        drawText(TITLE_TEXT, frameBuffer, shared, 16, 16, 32, mnuTxt);

        switch(mstate)
        {
            case FLD_SEL:
                drawRect(frameBuffer, 16, 64, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 65, 1248, 2, rectSh);

                drawRect(frameBuffer, 16, 66, 320, 592, sideRect);

                drawRect(frameBuffer, 16, 656, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 657, 1248, 2, rectSh);
                break;

            case USR_SEL:
            case TTL_SEL:
                drawRect(frameBuffer, 16, 64, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 65, 1248, 2, rectSh);

                drawRect(frameBuffer, 16, 656, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 657, 1248, 2, rectSh);
                break;

            case ADV_MDE:
                drawRect(frameBuffer, 16, 64, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 65, 1248, 2, rectSh);

                drawRect(frameBuffer, 640, 64, 1, 592, rectLt);
                drawRect(frameBuffer, 641, 64, 2, 592, rectSh);

                drawRect(frameBuffer, 16, 656, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 657, 1248, 2, rectSh);
                break;

            case CLS_TTL:
            case CLS_USR:
            case CLS_FLD:
                drawRect(frameBuffer, 16, 64, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 65, 1248, 2, rectSh);

                drawRect(frameBuffer, 16, 67, 448, 589, sideRect);

                drawRect(frameBuffer, 16, 656, 1248, 1, rectLt);
                drawRect(frameBuffer, 16, 657, 1248, 2, rectSh);
                break;
        }

        switch(mstate)
        {
            case USR_SEL:
            case CLS_USR:
                {
                    //Input guide
                    unsigned startX = 848;
                    texDraw(buttonA, frameBuffer, startX, 672);
                    drawText("Select", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonY, frameBuffer, startX += 72, 672);
                    drawText("Dump All", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonX, frameBuffer, startX += 96, 672);
                    drawText("Text Mode", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                }
                break;

            case TTL_SEL:
            case CLS_TTL:
                {
                    unsigned startX = 804;
                    texDraw(buttonA, frameBuffer, startX, 672);
                    drawText("Select", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonY, frameBuffer, startX += 72, 672);
                    drawText("Dump All", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonX, frameBuffer, startX += 96, 672);
                    drawText("Blacklist", frameBuffer, shared, startX += 38, 682, 12, mnuTxt);
                    texDraw(buttonB, frameBuffer, startX += 72, 672);
                    drawText("Back", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                }
                break;

            case FLD_SEL:
            case CLS_FLD:
                {
                    //Input guide
                    unsigned startX = 690;
                    texDraw(buttonMin, frameBuffer, startX, 672);
                    drawText("Adv. Mode", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonA, frameBuffer, startX += 100, 672);
                    drawText("Backup", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonY, frameBuffer, startX += 72, 672);
                    drawText("Restore", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonX, frameBuffer, startX += 72, 672);
                    drawText("Delete", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
                    texDraw(buttonB, frameBuffer, startX += 72, 672);
                    drawText("Back", frameBuffer, shared, startX += 38, 680, 14, mnuTxt);
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
        }
    }
}

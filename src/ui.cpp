#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <switch.h>

#include "ui.h"
#include "gfx.h"
#include "util.h"
#include "file.h"

#define VER_STRING "v. 04.06.2020"

//Nav buttons
std::vector<ui::button> usrNav, ttlNav, fldNav;

//Don't waste time drawing top and bottom over and over
static tex *top, *bot;

//Help text
static const std::string userHelp = "\ue0e0 Select   \ue0e3 Dump All   \ue0e2 UI Mode   \ue0e7 Options   \ue0f0 Extras";
static const std::string titleHelp = "\ue0e0 Select   \ue0e4\ue0e5 Change User   \ue0e3 Dump All   \ue0e2 BlackList   \ue0f0 Favorite   \ue0e1 Back";
static const std::string folderHelp = "\ue0f0 File Mode   \ue0e4\ue0e5 AutoName   \ue0e0 Backup   \ue0e3 Restore   \ue0e2 Delete   \ue0e1 Back";
static const std::string optHelp = "\ue0e0 Toggle   \ue0e1 Back";

//X position of help texts. Calculated to make editing quicker/easier
static unsigned userHelpX, titleHelpX, folderHelpX, optHelpX;

namespace ui
{
    //text mode
    bool textMode = false;

    //Current menu state
    int mstate = USR_SEL, prevState = USR_SEL;

    //Theme id
    ColorSetId thmID;

    //Info printed on folder menu
    std::string folderMenuInfo;

    //Touch button vector
    std::vector<ui::button> selButtons;

    //UI colors
    clr clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr, sideRect, divClr;

    //textbox pieces
    //I was going to flip them when I draw them, but then laziness kicked in.
    tex *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;
    tex *progCovLeft, *progCovRight;

    //Menu box pieces
    tex *mnuTopLeft, *mnuTopRight, *mnuBotLeft, *mnuBotRight;

    //Select box + top left icon
    tex *icn, *sideBar;

    //Shared font
    font *shared;

    void initTheme()
    {
        shared = fontLoadSharedFonts();

        setsysGetColorSetId(&thmID);

        mnuTopLeft = texLoadPNGFile("romfs:/img/fb/menuTopLeft.png");
        mnuTopRight = texLoadPNGFile("romfs:/img/fb/menuTopRight.png");
        mnuBotLeft = texLoadPNGFile("romfs:/img/fb/menuBotLeft.png");
        mnuBotRight = texLoadPNGFile("romfs:/img/fb/menuBotRight.png");

        switch(thmID)
        {
            case ColorSetId_Light:
                //Dark corners
                cornerTopLeft = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerTopLeft.png");
                cornerTopRight = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerTopRight.png");
                cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerBotLeft.png");
                cornerBottomRight = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerBotRight.png");
                progCovLeft = texLoadPNGFile("romfs:/img/tboxDrk/progBarCoverLeftDrk.png");
                progCovRight = texLoadPNGFile("romfs:/img/tboxDrk/progBarCoverRightDrk.png");

                icn = texLoadPNGFile("romfs:/img/icn/icnDrk.png");
                sideBar = texLoadPNGFile("romfs:/img/fb/lLight.png");

                clearClr = clrCreateU32(0xFFEBEBEB);
                mnuTxt = clrCreateU32(0xFF000000);
                txtClr = clrCreateU32(0xFFFFFFFF);
                rectLt = clrCreateU32(0xFFDFDFDF);
                rectSh = clrCreateU32(0xFFCACACA);
                tboxClr = clrCreateU32(0xFF505050);
                sideRect = clrCreateU32(0xFFDCDCDC);
                divClr = clrCreateU32(0xFF000000);
                break;

            default:
            case ColorSetId_Dark:
                //jic
                thmID = ColorSetId_Dark;
                //Light corners
                cornerTopLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
                cornerTopRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
                cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
                cornerBottomRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotRight.png");
                progCovLeft = texLoadPNGFile("romfs:/img/tboxLght/progBarCoverLeftLight.png");
                progCovRight = texLoadPNGFile("romfs:/img/tboxLght/progBarCoverRightLight.png");

                icn = texLoadPNGFile("romfs:/img/icn/icnLght.png");
                sideBar = texLoadPNGFile("romfs:/img/fb/lDark.png");

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
    }

    void init()
    {
        top = texCreate(1280, 88);
        bot = texCreate(1280, 72);

        if(fs::fileExists(fs::getWorkDir() + "cls.txt"))
        {
            textUserPrep();
            textMode = true;
            mstate = TXT_USR;
        }

        setupSelButtons();
        setupNavButtons();

        //Setup top and bottom gfx
        texClearColor(top, clearClr);
        texDraw(icn, top, 66, 27);
        drawText("JKSV", top, shared, 130, 38, 24, mnuTxt);
        drawRect(top, 30, 87, 1220, 1, mnuTxt);

        texClearColor(bot, clearClr);
        drawRect(bot, 30, 0, 1220, 1, mnuTxt);
        drawText(VER_STRING, bot, shared, 8, 56, 12, mnuTxt);

        //Not needed anymore
        texDestroy(icn);

        //Calculate x position of help text
        userHelpX = 1220 - textGetWidth(userHelp.c_str(), ui::shared, 18);
        titleHelpX = 1220 - textGetWidth(titleHelp.c_str(), ui::shared, 18);
        folderHelpX = 1220 - textGetWidth(folderHelp.c_str(), ui::shared, 18);
        optHelpX = 1220 - textGetWidth(optHelp.c_str(), ui::shared, 18);

        advCopyMenuPrep();
        ui::exMenuPrep();
        ui::optMenuInit();
    }

    void exit()
    {
        texDestroy(cornerTopLeft);
        texDestroy(cornerTopRight);
        texDestroy(cornerBottomLeft);
        texDestroy(cornerBottomRight);
        texDestroy(progCovLeft);
        texDestroy(progCovRight);

        texDestroy(mnuTopLeft);
        texDestroy(mnuTopRight);
        texDestroy(mnuBotLeft);
        texDestroy(mnuBotRight);

        fontDestroy(shared);
    }

    void setupSelButtons()
    {
        int x = 70, y = 98;
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
        texClearColor(frameBuffer, clearClr);
        texDrawNoAlpha(top, frameBuffer, 0, 0);
        texDrawNoAlpha(bot, frameBuffer, 0, 648);

        switch(mstate)
        {
            case FLD_SEL:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                break;

            case ADV_MDE:
                drawRect(frameBuffer, 640, 87, 1, 561, divClr);
                break;

            case TXT_TTL:
            case TXT_USR:
            case TXT_FLD:
            case EX_MNU:
            case OPT_MNU:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                break;
        }

        switch(mstate)
        {
            case USR_SEL:
            case TXT_USR:
                drawText(userHelp.c_str(), frameBuffer, shared, userHelpX, 676, 18, mnuTxt);
                break;

            case TTL_SEL:
            case TXT_TTL:
                drawText(titleHelp.c_str(), frameBuffer, shared, titleHelpX, 676, 18, mnuTxt);
                break;

            case FLD_SEL:
            case TXT_FLD:
                drawText(folderHelp.c_str(), frameBuffer, shared, folderHelpX, 676, 18, mnuTxt);
                break;

            case OPT_MNU:
                drawText(optHelp.c_str(), frameBuffer, ui::shared, optHelpX, 676, 18, ui::mnuTxt);
                break;
        }
    }

    void drawBoundBox(int x, int y, int w, int h, int clrSh)
    {
        clr rectClr = clrCreateRGBA(0x00, 0x88 + clrSh, 0xC5, 0xFF);

        texSwapColors(mnuTopLeft, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);
        texSwapColors(mnuTopRight, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);
        texSwapColors(mnuBotLeft, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);
        texSwapColors(mnuBotRight, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);

        switch(ui::thmID)
        {
            case ColorSetId_Light:
                drawRect(frameBuffer, x + 4, y + 4, w - 8, h - 8, clrCreateU32(0xFFFDFDFD));
                break;

            default:
            case ColorSetId_Dark:
                drawRect(frameBuffer, x + 4, y + 4, w - 8, h - 8, clrCreateU32(0xFF272221));
                break;
        }

        //top
        texDraw(mnuTopLeft, frameBuffer, x, y);
        drawRect(frameBuffer, x + 4, y, w - 8, 4, rectClr);
        texDraw(mnuTopRight, frameBuffer, (x + w) - 4, y);

        //mid
        drawRect(frameBuffer, x, y + 4, 4, h - 8, rectClr);
        drawRect(frameBuffer, (x + w) - 4, y + 4, 4, h - 8, rectClr);

        //bottom
        texDraw(mnuBotLeft, frameBuffer, x, (y + h) - 4);
        drawRect(frameBuffer, x + 4, (y + h) - 4, w - 8, 4, rectClr);
        texDraw(mnuBotRight, frameBuffer, (x + w) - 4, (y + h) - 4);

        texSwapColors(mnuTopLeft, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
        texSwapColors(mnuTopRight, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
        texSwapColors(mnuBotLeft, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
        texSwapColors(mnuBotRight, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
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

            case TXT_USR:
                textUserMenuUpdate(down, held, p);
                break;

            case TXT_TTL:
                textTitleMenuUpdate(down, held, p);
                break;

            case TXT_FLD:
                textFolderMenuUpdate(down, held, p);
                break;

            case EX_MNU:
                updateExMenu(down, held, p);
                break;

            case OPT_MNU:
                updateOptMenu(down, held, p);
                break;
        }

        drawPopup(down);
    }
}

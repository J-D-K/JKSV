#include <string>
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include <switch.h>

#include "file.h"
#include "ui.h"
#include "gfx.h"
#include "util.h"

//Current menu state
int ui::mstate = USR_SEL, ui::prevState = USR_SEL;

float ui::animScale = 3.0f;

//pad data?
PadState ui::pad;
HidTouchScreenState ui::touchState;

//Theme id
ColorSetId ui::thmID;

//Info printed on folder menu
std::string ui::folderMenuInfo;

//UI colors
SDL_Color ui::clearClr, ui::txtCont, ui::txtDiag, ui::rectLt, ui::rectSh, ui::tboxClr, ui::divClr, ui::slidePanelColor;
SDL_Color ui::transparent = {0x00, 0x00, 0x00, 0x00};

//textbox pieces
//I was going to flip them when I draw them, but then laziness kicked in.
SDL_Texture *ui::cornerTopLeft, *ui::cornerTopRight, *ui::cornerBottomLeft, *ui::cornerBottomRight;

//Progress bar covers + dialog box predrawn
SDL_Texture *ui::progCovLeft, *ui::progCovRight, *ui::diaBox;

//Menu box pieces
SDL_Texture *ui::mnuTopLeft, *ui::mnuTopRight, *ui::mnuBotLeft, *ui::mnuBotRight;

//Select box + top left icon
SDL_Texture *ui::sideBar;

static SDL_Texture *icn, *corePanel;
SDL_Color ui::heartColor = {0xFF, 0x44, 0x44, 0xFF};

static int settPos, extPos;

//Vector of pointers to slideOutPanels. Is looped and drawn last so they are always on top
std::vector<ui::slideOutPanel *> panels;

static ui::popMessageMngr *popMessages;
static ui::threadProcMngr *threadMngr;

//8
const std::string ui::loadGlyphArray[] =
{
    "\ue020", "\ue021", "\ue022", "\ue023",
    "\ue024", "\ue025", "\ue026", "\ue027"
};

void ui::initTheme()
{
    uint64_t lang;
    setGetSystemLanguage(&lang);
    setMakeLanguage(lang, &data::sysLang);

    loadTrans();

    setsysGetColorSetId(&thmID);

    switch(thmID)
    {
        case ColorSetId_Light:
            clearClr = {0xEB, 0xEB, 0xEB, 0xFF};
            txtCont  = {0x00, 0x00, 0x00, 0xFF};
            txtDiag  = {0xFF, 0xFF, 0xFF, 0xFF};
            rectLt   = {0xDF, 0xDF, 0xDF, 0xFF};
            rectSh   = {0xCA, 0xCA, 0xCA, 0xFF};
            tboxClr  = {0xEB, 0xEB, 0xEB, 0xFF};
            divClr   = {0x00, 0x00, 0x00, 0xFF};
            slidePanelColor = {0xEE, 0xEE, 0xEE, 0xEE};
            break;

        default:
        case ColorSetId_Dark:
            //jic
            thmID = ColorSetId_Dark;
            clearClr = {0x2D, 0x2D, 0x2D, 0xFF};
            txtCont  = {0xFF, 0xFF, 0xFF, 0xFF};
            txtDiag  = {0x00, 0x00, 0x00, 0xFF};
            rectLt   = {0x50, 0x50, 0x50, 0xFF};
            rectSh   = {0x20, 0x20, 0x20, 0xFF};
            tboxClr  = {0x50, 0x50, 0x50, 0xFF};
            divClr   = {0xFF, 0xFF, 0xFF, 0xFF};
            slidePanelColor  = {0x00, 0x00, 0x00, 0xEE};
            break;
    }
}

void ui::init()
{
    mnuTopLeft  = gfx::texMgr->textureLoadFromFile("romfs:/img/fb/menuTopLeft.png");
    mnuTopRight = gfx::texMgr->textureLoadFromFile("romfs:/img/fb/menuTopRight.png");
    mnuBotLeft  = gfx::texMgr->textureLoadFromFile("romfs:/img/fb/menuBotLeft.png");
    mnuBotRight = gfx::texMgr->textureLoadFromFile("romfs:/img/fb/menuBotRight.png");

    corePanel  = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET | SDL_TEXTUREACCESS_STATIC, 1220, 559);
    SDL_SetTextureBlendMode(corePanel, SDL_BLENDMODE_BLEND);

    switch(ui::thmID)
    {
        case ColorSetId_Light:
            //Dark corners
            cornerTopLeft     = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
            cornerTopRight    = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
            cornerBottomLeft  = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
            cornerBottomRight = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxLght/tboxCornerBotRight.png");
            progCovLeft       = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxDrk/progBarCoverLeftDrk.png");
            progCovRight      = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxDrk/progBarCoverRightDrk.png");
            icn               = gfx::texMgr->textureLoadFromFile("romfs:/img/icn/icnDrk.png");
            sideBar           = gfx::texMgr->textureLoadFromFile("romfs:/img/fb/lLight.png");
            break;

        default:
            //Light corners
            cornerTopLeft     = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxDrk/tboxCornerTopLeft.png");
            cornerTopRight    = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxDrk/tboxCornerTopRight.png");
            cornerBottomLeft  = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxDrk/tboxCornerBotLeft.png");
            cornerBottomRight = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxDrk/tboxCornerBotRight.png");
            progCovLeft       = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxLght/progBarCoverLeftLight.png");
            progCovRight      = gfx::texMgr->textureLoadFromFile("romfs:/img/tboxLght/progBarCoverRightLight.png");
            icn               = gfx::texMgr->textureLoadFromFile("romfs:/img/icn/icnLght.png");
            sideBar           = gfx::texMgr->textureLoadFromFile("romfs:/img/fb/lDark.png");
            break;
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&ui::pad);

    ui::usrInit();
    ui::ttlInit();
    ui::settInit();
    ui::extInit();
    ui::fmInit();
    ui::fldInit();

    popMessages = new ui::popMessageMngr;
    threadMngr  = new ui::threadProcMngr;

    //Need these from user/main menu
    settPos = ui::usrMenu->getOptPos(ui::getUICString("mainMenuSettings", 0));
    extPos  = ui::usrMenu->getOptPos(ui::getUICString("mainMenuExtras", 0));
}

void ui::exit()
{
    ui::usrExit();
    ui::ttlExit();
    ui::settExit();
    ui::extExit();
    ui::fmExit();
    ui::fldExit();

    delete popMessages;
    delete threadMngr;
}

int ui::registerPanel(slideOutPanel *sop)
{
    panels.push_back(sop);
    return panels.size() - 1;
}

threadInfo *ui::newThread(ThreadFunc func, void *args, funcPtr _drawFunc)
{
    return threadMngr->newThread(func, args, _drawFunc);
}

void ui::showLoadScreen()
{
    SDL_Texture *icon = gfx::texMgr->textureLoadFromFile("romfs:/icon.png");
    gfx::clearTarget(NULL, &ui::clearClr);
    gfx::texDraw(NULL, icon, 512, 232);
    gfx::drawTextf(NULL, 16, 1100, 673, &ui::txtCont, ui::getUICString("loadingStartPage", 0));
    gfx::present();
}

void ui::drawUI()
{
    gfx::clearTarget(NULL, &ui::clearClr);
    gfx::clearTarget(corePanel, &transparent);

    gfx::drawLine(NULL, &divClr, 30, 88, 1250, 88);
    gfx::drawLine(NULL, &divClr, 30, 648, 1250, 648);
    gfx::texDraw(NULL, icn, 66, 27);

    if(util::isApplet())
        gfx::drawTextf(NULL, 24, 130, 38, &ui::txtCont, "JKSV *APPLET MODE*");
    else
        gfx::drawTextf(NULL, 24, 130, 38, &ui::txtCont, "JKSV");

    //Version / translation author
    gfx::drawTextf(NULL, 12, 8, 700, &ui::txtCont, "v. %02d.%02d.%04d", BLD_MON, BLD_DAY, BLD_YEAR);
    if(ui::getUIString("author", 0) != "NULL")
        gfx::drawTextf(NULL, 12, 8, 682, &ui::txtCont, "%s%s", ui::getUICString("translationMainPage", 0), ui::getUICString("author", 0));

    //This only draws the help text now and only does when user select is open
    ui::usrDraw(NULL);

    if((ui::usrMenu->getActive() && ui::usrMenu->getSelected() == settPos) || ui::mstate == OPT_MNU)
        ui::settDraw(corePanel);
    else if((ui::usrMenu->getActive() && ui::usrMenu->getSelected() == extPos) || ui::mstate == EX_MNU)
        ui::extDraw(corePanel);
    else if(ui::mstate == FIL_MDE)
        ui::fmDraw(corePanel);
    else
        ui::ttlDraw(corePanel);

    gfx::texDraw(NULL, corePanel, 30, 89);
    for(slideOutPanel *s : panels)
        s->draw(&ui::slidePanelColor);

    threadMngr->draw();

    popMessages->draw();
}

static bool debugDisp = false;

bool ui::runApp()
{
    ui::updateInput();
    uint64_t down = ui::padKeysDown();

    if(threadMngr->empty())
    {
        if(down & HidNpadButton_StickL && down & HidNpadButton_StickR)
            debugDisp = true;
        else if(down & HidNpadButton_Plus)
            return false;

        switch(ui::mstate)
        {
            case USR_SEL:
                ui::usrUpdate();
                break;

            case TTL_SEL:
                ui::ttlUpdate();
                break;

            case OPT_MNU:
                ui::settUpdate();
                break;

            case EX_MNU:
                ui::extUpdate();
                break;

            case FIL_MDE:
                ui::fmUpdate();
                break;
        }
    }
    else
        threadMngr->update();

    popMessages->update();

    drawUI();
    if(debugDisp)
        data::dispStats();

    gfx::present();

    return true;
}

void ui::showPopMessage(int frameCount, const char *fmt, ...)
{
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    popMessages->popMessageAdd(tmp, frameCount);
}

void ui::toTTL(void *a)
{
    data::user *u = data::getCurrentUser();
    unsigned curUserIndex = data::getCurrentUserIndex();
    if(u->titleInfo.size() > 0)
    {
        ui::changeState(TTL_SEL);
        ui::ttlSetActive(curUserIndex, true, true);
        ui::usrMenu->setActive(false);
    }
    else
    {
        data::user *u = data::getCurrentUser();
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataNoneFound", 0), u->getUsername().c_str());
    }
}

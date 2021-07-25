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

static SDL_Texture *icn, *leftPanel, *rightPanel;
SDL_Color ui::heartColor = {0xFF, 0x44, 0x44, 0xFF};

//X position of help texts. Calculated to make editing quicker/easier
static unsigned userHelpX, titleHelpX, folderHelpX, optHelpX;
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
            slidePanelColor = {0xEE, 0xEE, 0xEE, 0xDD};
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
            slidePanelColor  = {0x00, 0x00, 0x00, 0xDD};
            break;
    }
}

void ui::init()
{
    mnuTopLeft  = gfx::loadImageFile("romfs:/img/fb/menuTopLeft.png");
    mnuTopRight = gfx::loadImageFile("romfs:/img/fb/menuTopRight.png");
    mnuBotLeft  = gfx::loadImageFile("romfs:/img/fb/menuBotLeft.png");
    mnuBotRight = gfx::loadImageFile("romfs:/img/fb/menuBotRight.png");

    leftPanel = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET | SDL_TEXTUREACCESS_STATIC, 200, 559);
    rightPanel  = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET | SDL_TEXTUREACCESS_STATIC, 1080, 559);
    SDL_SetTextureBlendMode(leftPanel, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(rightPanel, SDL_BLENDMODE_BLEND);

    switch(ui::thmID)
    {
        case ColorSetId_Light:
            //Dark corners
            cornerTopLeft     = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
            cornerTopRight    = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
            cornerBottomLeft  = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
            cornerBottomRight = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerBotRight.png");
            progCovLeft       = gfx::loadImageFile("romfs:/img/tboxDrk/progBarCoverLeftDrk.png");
            progCovRight      = gfx::loadImageFile("romfs:/img/tboxDrk/progBarCoverRightDrk.png");
            icn               = gfx::loadImageFile("romfs:/img/icn/icnDrk.png");
            sideBar           = gfx::loadImageFile("romfs:/img/fb/lLight.png");
            break;

        default:
            //Light corners
            cornerTopLeft     = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerTopLeft.png");
            cornerTopRight    = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerTopRight.png");
            cornerBottomLeft  = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerBotLeft.png");
            cornerBottomRight = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerBotRight.png");
            progCovLeft       = gfx::loadImageFile("romfs:/img/tboxLght/progBarCoverLeftLight.png");
            progCovRight      = gfx::loadImageFile("romfs:/img/tboxLght/progBarCoverRightLight.png");
            icn               = gfx::loadImageFile("romfs:/img/icn/icnLght.png");
            sideBar           = gfx::loadImageFile("romfs:/img/fb/lDark.png");
            break;
    }

    loadTrans();

    //Replace the button [x] in strings that need it. Needs to be outside loadTrans so even defaults will get replaced
    util::replaceButtonsInString(ui::userHelp);
    util::replaceButtonsInString(ui::titleHelp);
    util::replaceButtonsInString(ui::folderHelp);
    util::replaceButtonsInString(ui::optHelp);
    util::replaceButtonsInString(ui::yt);
    util::replaceButtonsInString(ui::nt);

    //Calculate x position of help text
    userHelpX = 1220 - gfx::getTextWidth(ui::userHelp.c_str(), 18);
    titleHelpX = 1220 - gfx::getTextWidth(ui::titleHelp.c_str(), 18);
    folderHelpX = 1220 - gfx::getTextWidth(ui::folderHelp.c_str(), 18);
    optHelpX = 1220 - gfx::getTextWidth(ui::optHelp.c_str(), 18);

    //setup pad
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&ui::pad);

    //Setup touch
    hidInitializeTouchScreen();

    //advCopyMenuPrep();
    ui::usrInit();
    ui::ttlInit();
    ui::settInit();

    popMessages = new ui::popMessageMngr;
    threadMngr  = new ui::threadProcMngr;

    //Need these from user/main menu
    settPos = ui::usrMenu->getOptPos("Settings");
    extPos  = ui::usrMenu->getOptPos("Extras");
}

void ui::exit()
{
    ui::usrExit();
    ui::ttlExit();
    ui::settExit();

    delete popMessages;
    delete threadMngr;

    SDL_DestroyTexture(cornerTopLeft);
    SDL_DestroyTexture(cornerTopRight);
    SDL_DestroyTexture(cornerBottomLeft);
    SDL_DestroyTexture(cornerBottomRight);
    SDL_DestroyTexture(progCovLeft);
    SDL_DestroyTexture(progCovRight);

    SDL_DestroyTexture(mnuTopLeft);
    SDL_DestroyTexture(mnuTopRight);
    SDL_DestroyTexture(mnuBotLeft);
    SDL_DestroyTexture(mnuBotRight);

    SDL_DestroyTexture(leftPanel);
    SDL_DestroyTexture(rightPanel);

    SDL_DestroyTexture(icn);
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
    SDL_Texture *icon = gfx::loadImageFile("romfs:/icon.png");
    gfx::clearTarget(NULL, &ui::clearClr);
    gfx::texDraw(NULL, icon, 512, 232);
    gfx::drawTextf(NULL, 16, 1100, 673, &ui::txtCont, "Loading...");
    gfx::present();
    SDL_DestroyTexture(icon);
}

void ui::drawUI()
{
    gfx::clearTarget(NULL, &ui::clearClr);
    gfx::clearTarget(leftPanel, &transparent);
    gfx::clearTarget(rightPanel, &transparent);

    gfx::drawLine(NULL, &divClr, 30, 88, 1250, 88);
    gfx::drawLine(NULL, &divClr, 30, 648, 1250, 648);
    gfx::texDraw(NULL, icn, 66, 27);
    gfx::drawTextf(NULL, 24, 130, 38, &ui::txtCont, "JKSV");

    //Version / translation author
    gfx::drawTextf(NULL, 12, 8, 700, &ui::txtCont, "v. %02d.%02d.%04d", BLD_MON, BLD_DAY, BLD_YEAR);
    if(author != "NULL")
        gfx::drawTextf(NULL, 12, 8, 682, &ui::txtCont, "Translation: %s", author.c_str());

    gfx::texDraw(leftPanel, sideBar, 0, 89);
    ui::usrDraw(leftPanel);

    if(ui::usrMenu->getSelected() == settPos || ui::mstate == OPT_MNU)
        ui::settDraw(rightPanel);
    else if(usrMenu->getSelected() == extPos || ui::mstate == EX_MNU)
        gfx::drawTextf(rightPanel, 24, 32, 32, &ui::txtCont, "PLACE HOLDER");
    else
        ui::ttlDraw(rightPanel);

    gfx::texDraw(NULL, rightPanel, 200, 89);
    gfx::texDraw(NULL, leftPanel, 0, 89);
    for(slideOutPanel *s : panels)
        s->draw(&ui::slidePanelColor);

    popMessages->draw();

    if(!threadMngr->empty())
        threadMngr->draw();
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
                usrUpdate();
                break;

            case TTL_SEL:
                ttlUpdate();
                break;

            case OPT_MNU:
                settUpdate();
                break;

            case EX_MNU:
                /*extMenu.update();
                if(down & HidNpadButton_B)
                {
                    ui::changeState(USR_SEL);
                    mainMenu.setActive(true);
                    extMenu.setActive(false);
                }*/
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
    if(data::curUser.titleInfo.size() > 0)
    {
        ui::changeState(TTL_SEL);
        ui::ttlSetActive(data::selUser);
        ui::usrMenu->setActive(false);
    }
    else
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::noSavesFound.c_str(), data::curUser.getUsername().c_str());
}

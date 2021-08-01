#include <string>
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <switch.h>

#include "ui.h"
#include "gfx.h"
#include "util.h"
#include "file.h"

//text mode
bool ui::textMode = false;

//Current menu state
int ui::mstate = USR_SEL, ui::prevState = USR_SEL;

//pad data?
PadState ui::pad;

//Theme id
ColorSetId ui::thmID;

//Info printed on folder menu
std::string ui::folderMenuInfo;

//UI colors
SDL_Color ui::clearClr, ui::txtCont, ui::txtDiag, ui::rectLt, ui::rectSh, ui::tboxClr, divClr;

//textbox pieces
//I was going to flip them when I draw them, but then laziness kicked in.
SDL_Texture *ui::cornerTopLeft, *ui::cornerTopRight, *ui::cornerBottomLeft, *ui::cornerBottomRight;

//Progress bar covers + dialog box predrawn
SDL_Texture *ui::progCovLeft, *ui::progCovRight, *ui::diaBox;

//Menu box pieces
SDL_Texture *mnuTopLeft, *mnuTopRight, *mnuBotLeft, *mnuBotRight;

//Select box + top left icon
SDL_Texture *ui::sideBar;

static SDL_Texture *icn;

//X position of help texts. Calculated to make editing quicker/easier
static unsigned userHelpX, titleHelpX, folderHelpX, optHelpX;

//Map to associate external string names to unsigned ints for switch case.
static std::unordered_map<std::string, unsigned> uistrdef =
{
    {"author", 0}, {"userHelp", 1}, {"titleHelp", 2}, {"folderHelp", 3}, {"optHelp", 4},
    {"yt", 5}, {"nt", 6}, {"on", 7}, {"off", 8}, {"confirmBlacklist", 9}, {"confirmOverwrite", 10},
    {"confirmRestore", 11}, {"confirmDelete", 12}, {"confirmCopy", 13}, {"confirmEraseNand", 14},
    {"confirmEraseFolder", 15}, {"confirmHead", 16}, {"copyHead", 17}, {"noSavesFound", 18},
    {"advMenu", 19}, {"extMenu", 20}, {"optMenu", 21}, {"optMenuExp", 22}, {"holdingText", 23},
    {"errorConnecting", 24}, {"noUpdate", 25}, {"sortType", 26}
};

static void loadTrans()
{
    bool transFile = fs::fileExists(fs::getWorkDir() + "trans.txt");
    if(!transFile && (data::sysLang == SetLanguage_ENUS || data::langOverride))
        return;//Don't bother loading from file. It serves as a translation guide

    std::string file;
    if(transFile)
        file = fs::getWorkDir() + "trans.txt";
    else
    {
        file = "romfs:/lang/";
        switch(data::sysLang)
        {
            case SetLanguage_JA:
                file += "ja.txt";
                break;

            case SetLanguage_ZHCN:
            case SetLanguage_ZHHANS:
                file += "zh-CN.txt";
                break;

            case SetLanguage_ZHTW:
            case SetLanguage_ZHHANT:
                file += "zh-TW.txt";
                break;


            case SetLanguage_PTBR:
                file += "pt-BR.txt";
                break;

            default:
                return;
                break;
        }
    }

    fs::dataFile lang(file);
    while(lang.readNextLine(true))
    {
        fs::logWrite("%s: %u\n", lang.getName().c_str(), uistrdef[lang.getName()]);
        switch(uistrdef[lang.getName()])
        {
            case 0:
                ui::author = lang.getNextValueStr();
                break;

            case 1:
                ui::userHelp = lang.getNextValueStr();
                break;

            case 2:
                ui::titleHelp = lang.getNextValueStr();
                break;

            case 3:
                ui::folderHelp = lang.getNextValueStr();
                break;

            case 4:
                ui::optHelp = lang.getNextValueStr();
                break;

            case 5:
                ui::yt = lang.getNextValueStr();
                break;

            case 6:
                ui::nt = lang.getNextValueStr();
                break;

            case 7:
                ui::on = lang.getNextValueStr();
                break;

            case 8:
                ui::off = lang.getNextValueStr();
                break;

            case 9:
                ui::confBlacklist = lang.getNextValueStr();
                break;

            case 10:
                ui::confOverwrite = lang.getNextValueStr();
                break;

            case 11:
                ui::confRestore = lang.getNextValueStr();
                break;

            case 12:
                ui::confDel = lang.getNextValueStr();
                break;

            case 13:
                ui::confCopy = lang.getNextValueStr();
                break;

            case 14:
                ui::confEraseNand = lang.getNextValueStr();
                break;

            case 15:
                ui::confEraseFolder = lang.getNextValueStr();
                break;

            case 16:
                ui::confirmHead = lang.getNextValueStr();
                break;

            case 17:
                ui::copyHead = lang.getNextValueStr();
                break;

            case 18:
                ui::noSavesFound = lang.getNextValueStr();
                break;

            case 19:
                {
                    int ind = lang.getNextValueInt();
                    ui::advMenuStr[ind] = lang.getNextValueStr();
                }
                break;

            case 20:
                {
                    int ind = lang.getNextValueInt();
                    ui::exMenuStr[ind] = lang.getNextValueStr();
                }
                break;

            case 21:
                {
                    int ind = lang.getNextValueInt();
                    ui::optMenuStr[ind] = lang.getNextValueStr();
                }
                break;

            case 22:
                {
                    int ind = lang.getNextValueInt();
                    ui::optMenuExp[ind] = lang.getNextValueStr();
                }
                break;

            case 23:
                {
                    int ind = lang.getNextValueInt();
                    ui::holdingText[ind] = lang.getNextValueStr();
                }
                break;

            case 24:
                ui::errorConnecting = lang.getNextValueStr();
                break;

            case 25:
                ui::noUpdate = lang.getNextValueStr();
                break;

            case 26:
                {
                    int ind = lang.getNextValueInt();
                    ui::sortString[ind] = lang.getNextValueStr();
                }
                break;

            default:
                //ui::showMessage("*Translation File Error:*", "On Line: %s\n*%s* is not a known or valid string name.", lang.getLine(), lang.getName());
                break;
        }
    }
}

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
            tboxClr  = {0x50, 0x50, 0x50, 0xFF};
            divClr   = {0x00, 0x00, 0x00, 0xFF};
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
            tboxClr  = {0xEB, 0xEB, 0xEB, 0xFF};
            divClr   = {0xFF, 0xFF, 0xFF, 0xFF};
            break;
    }
}

void ui::init()
{
    mnuTopLeft  = gfx::loadImageFile("romfs:/img/fb/menuTopLeft.png");
    mnuTopRight = gfx::loadImageFile("romfs:/img/fb/menuTopRight.png");
    mnuBotLeft  = gfx::loadImageFile("romfs:/img/fb/menuBotLeft.png");
    mnuBotRight = gfx::loadImageFile("romfs:/img/fb/menuBotRight.png");
    switch(ui::thmID)
    {
        case ColorSetId_Light:
            //Dark corners
            cornerTopLeft     = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerTopLeft.png");
            cornerTopRight    = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerTopRight.png");
            cornerBottomLeft  = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerBotLeft.png");
            cornerBottomRight = gfx::loadImageFile("romfs:/img/tboxDrk/tboxCornerBotRight.png");
            progCovLeft       = gfx::loadImageFile("romfs:/img/tboxDrk/progBarCoverLeftDrk.png");
            progCovRight      = gfx::loadImageFile("romfs:/img/tboxDrk/progBarCoverRightDrk.png");
            icn               = gfx::loadImageFile("romfs:/img/icn/icnDrk.png");
            sideBar           = gfx::loadImageFile("romfs:/img/fb/lLight.png");
            break;

        default:
            //Light corners
            cornerTopLeft     = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
            cornerTopRight    = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
            cornerBottomLeft  = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
            cornerBottomRight = gfx::loadImageFile("romfs:/img/tboxLght/tboxCornerBotRight.png");
            progCovLeft       = gfx::loadImageFile("romfs:/img/tboxLght/progBarCoverLeftLight.png");
            progCovRight      = gfx::loadImageFile("romfs:/img/tboxLght/progBarCoverRightLight.png");
            icn               = gfx::loadImageFile("romfs:/img/icn/icnLght.png");
            sideBar           = gfx::loadImageFile("romfs:/img/fb/lDark.png");
            break;
    }

    if(ui::textMode && data::skipUser)
    {
        ui::textTitlePrep(data::curUser);
        mstate = TXT_TTL;
    }
    else if(ui::textMode)
        mstate = TXT_USR;
    else if(data::skipUser)
        mstate = TTL_SEL;

    textUserPrep();
    loadTrans();

    //Replace the button [x] in strings that need it. Needs to be outside loadTrans so even defaults will get replaced
    util::replaceButtonsInString(ui::userHelp);
    util::replaceButtonsInString(ui::titleHelp);
    util::replaceButtonsInString(ui::folderHelp);
    util::replaceButtonsInString(ui::optHelp);
    util::replaceButtonsInString(ui::yt);
    util::replaceButtonsInString(ui::nt);
    util::replaceButtonsInString(ui::optMenuExp[3]);
    util::replaceButtonsInString(ui::optMenuExp[4]);
    util::replaceButtonsInString(ui::optMenuExp[5]);

    //Calculate x position of help text
    userHelpX = 1220 - gfx::getTextWidth(ui::userHelp.c_str(), 18);
    titleHelpX = 1220 - gfx::getTextWidth(ui::titleHelp.c_str(), 18);
    folderHelpX = 1220 - gfx::getTextWidth(ui::folderHelp.c_str(), 18);
    optHelpX = 1220 - gfx::getTextWidth(ui::optHelp.c_str(), 18);

    //setup pad
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&ui::pad);

    advCopyMenuPrep();
    ui::exMenuPrep();
    ui::optMenuInit();
}

void ui::exit()
{
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

    SDL_DestroyTexture(icn);
}

void ui::showLoadScreen()
{
    SDL_Texture *icon = gfx::loadImageFile("romfs:/icon.png");
    gfx::clear(&ui::clearClr);
    gfx::texDraw(icon, 512, 226);
    gfx::drawTextf(18, 1100, 673, &ui::txtCont, "Loading...");
    gfx::present();
    SDL_DestroyTexture(icon);
}

void ui::drawUI()
{
    gfx::clear(&ui::clearClr);
    gfx::texDraw(icn, 66, 27);
    gfx::drawTextf(24, 130, 38, &ui::txtCont, "JKSV");
    gfx::drawLine(&divClr, 30, 88, 1250, 88);
    gfx::drawLine(&divClr, 30, 648, 1250, 648);

    //Version / translation author
    gfx::drawTextf(12, 8, 700, &ui::txtCont, "v. %02d.%02d.%04d", BLD_MON, BLD_DAY, BLD_YEAR);
    if(author != "NULL")
        gfx::drawTextf(12, 8, 682, &ui::txtCont, "Translation: %s", author.c_str());

    switch(mstate)
    {
        case USR_SEL:
            gfx::drawTextf(18, userHelpX, 673, &ui::txtCont, userHelp.c_str());
            ui::drawUserMenu();
            break;

        case TTL_SEL:
            gfx::drawTextf(18, titleHelpX, 673, &ui::txtCont, titleHelp.c_str());
            ui::drawTitleMenu();
            break;

        case FLD_SEL:
            gfx::texDraw(sideBar, 0, 89);
            gfx::drawTextf(18, folderHelpX, 673, &ui::txtCont, folderHelp.c_str());
            ui::drawFolderMenu();
            break;

        case TXT_USR:
            gfx::texDraw(sideBar, 0, 89);
            gfx::drawTextf(18, userHelpX, 673, &ui::txtCont, userHelp.c_str());
            ui::drawTextUserMenu();
            break;

        case TXT_TTL:
            gfx::texDraw(sideBar, 0, 89);
            gfx::drawTextf(18, titleHelpX, 673, &ui::txtCont, titleHelp.c_str());
            ui::drawTextTitleMenu();
            break;

        case TXT_FLD:
            gfx::texDraw(sideBar, 0, 89);
            gfx::drawTextf(18, folderHelpX, 673, &ui::txtCont, folderHelp.c_str());
            ui::drawTextFolderMenu();
            break;

        case EX_MNU:
            gfx::texDraw(sideBar, 0, 89);
            ui::drawExMenu();
            break;

        case OPT_MNU:
            gfx::texDraw(sideBar, 0, 89);
            ui::drawOptMenu();
            break;

        case ADV_MDE:
            gfx::drawRect(&ui::txtCont, 640, 88, 1, 559);
            drawAdvMode();
            break;
    }
}

void ui::drawBoundBox(int x, int y, int w, int h, int clrSh)
{
    SDL_Color rectClr;

    if(ui::thmID == ColorSetId_Light)
        rectClr = {0xFD, 0xFD, 0xFD, 0xFF};
    else
        rectClr = {0x21, 0x22, 0x21, 0xFF};

    gfx::drawRect(&rectClr, x + 4, y + 4, w - 8, h - 8);

    rectClr = {0x00,(uint8_t)(0x88 + clrSh), (uint8_t)(0xC5 + (clrSh / 2)), 0xFF};

    SDL_SetTextureColorMod(mnuTopLeft, rectClr.r, rectClr.g, rectClr.b);
    SDL_SetTextureColorMod(mnuTopRight, rectClr.r, rectClr.g, rectClr.b);
    SDL_SetTextureColorMod(mnuBotLeft, rectClr.r, rectClr.g, rectClr.b);
    SDL_SetTextureColorMod(mnuBotRight, rectClr.r, rectClr.g, rectClr.b);

    //top
    gfx::texDraw(mnuTopLeft, x, y);
    gfx::drawRect(&rectClr, x + 4, y, w - 8, 4);
    gfx::texDraw(mnuTopRight, (x + w) - 4, y);

    //mid
    gfx::drawRect(&rectClr, x, y + 4, 4, h - 8);
    gfx::drawRect(&rectClr, (x + w) - 4, y + 4, 4, h - 8);

    //bottom
    gfx::texDraw(mnuBotLeft, x, (y + h) - 4);
    gfx::drawRect(&rectClr, x + 4, (y + h) - 4, w - 8, 4);
    gfx::texDraw(mnuBotRight, (x + w) - 4, (y + h) - 4);
}

void ui::runApp(const uint64_t& down, const uint64_t& held)
{
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

        case TXT_USR:
            textUserMenuUpdate(down, held);
            break;

        case TXT_TTL:
            textTitleMenuUpdate(down, held);
            break;

        case TXT_FLD:
            textFolderMenuUpdate(down, held);
            break;

        case EX_MNU:
            updateExMenu(down, held);
            break;

        case OPT_MNU:
            updateOptMenu(down, held);
            break;
    }
    drawUI();
    drawPopup(down);
}

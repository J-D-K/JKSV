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

#define VER_STRING "v. 06.23.2020"

//text mode
bool ui::textMode = false;

//Current menu state
int ui::mstate = USR_SEL, ui::prevState = USR_SEL;

//Theme id
ColorSetId ui::thmID;

//Info printed on folder menu
std::string ui::folderMenuInfo;

//UI colors
clr ui::clearClr, ui::txtCont, ui::txtDiag, ui::rectLt, ui::rectSh, ui::tboxClr, divClr;

//textbox pieces
//I was going to flip them when I draw them, but then laziness kicked in.
tex *ui::cornerTopLeft, *ui::cornerTopRight, *ui::cornerBottomLeft, *ui::cornerBottomRight;

//Progress bar covers + dialog box predrawn
tex *ui::progCovLeft, *ui::progCovRight, *ui::diaBox;

//Menu box pieces
tex *mnuTopLeft, *mnuTopRight, *mnuBotLeft, *mnuBotRight;

//Select box + top left icon
tex *ui::sideBar;

alphaMask *ui::iconMask;

//Shared font
font *ui::shared;

//Don't waste time drawing top and bottom over and over
//guide graphics are to save cpu drawing that over and over with alpha
static tex *top, *bot, *usrGuide, *ttlGuide, *fldrGuide, *optGuide;

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
    {"errorConnecting", 24}, {"noUpdate", 25}
};

static void loadTrans()
{
    bool transFile = fs::fileExists(fs::getWorkDir() + "trans.txt");
    if(!transFile && data::sysLang == SetLanguage_ENUS)
        return;//Don't bother loading from file. It serves as a translation guide

    std::string file;
    if(transFile)
        file = fs::getWorkDir() + "trans.txt";
    else
    {
        file = "romfs:/lang/";
        switch(data::sysLang)
        {
            case SetLanguage_ZHCN:
            case SetLanguage_ZHHANS:
                file += "zh-CN.txt";
                break;

            case SetLanguage_ZHTW:
            case SetLanguage_ZHHANT:
                file += "zh-TW.txt";
                break;

            default:
                return;
                break;
        }
    }

    fs::dataFile lang(file);
    while(lang.readNextLine(true))
    {
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

            default:
                ui::showMessage("*Translation File Error:*", "On Line: %s\n*%s* is not a known or valid string name.", lang.getLine(), lang.getName());
                break;
        }
    }
}

void ui::initTheme()
{
    if(fs::fileExists(fs::getWorkDir() + "font.ttf"))
        shared = fontLoadTTF(std::string(fs::getWorkDir() + "font.ttf").c_str());
    else
        shared = fontLoadSharedFonts();

    iconMask = alphaMaskLoad(256, 256, "romfs:/img/icn/icon.msk");

    setsysGetColorSetId(&thmID);

    switch(thmID)
    {
        case ColorSetId_Light:
            clearClr = clrCreateU32(0xFFEBEBEB);
            txtCont = clrCreateU32(0xFF000000);
            txtDiag = clrCreateU32(0xFFFFFFFF);
            rectLt = clrCreateU32(0xFFDFDFDF);
            rectSh = clrCreateU32(0xFFCACACA);
            tboxClr = clrCreateU32(0xFF505050);
            divClr = clrCreateU32(0xFF000000);
            break;

        default:
        case ColorSetId_Dark:
            //jic
            thmID = ColorSetId_Dark;
            clearClr = clrCreateU32(0xFF2D2D2D);
            txtCont = clrCreateU32(0xFFFFFFFF);
            txtDiag = clrCreateU32(0xFF000000);
            rectLt = clrCreateU32(0xFF505050);
            rectSh = clrCreateU32(0xFF202020);
            tboxClr = clrCreateU32(0xFFEBEBEB);
            divClr = clrCreateU32(0xFFFFFFFF);
            break;
    }
}

void ui::init()
{
    tex *icn;
    char verStr[16];
    mnuTopLeft = texLoadPNGFile("romfs:/img/fb/menuTopLeft.png");
    mnuTopRight = texLoadPNGFile("romfs:/img/fb/menuTopRight.png");
    mnuBotLeft = texLoadPNGFile("romfs:/img/fb/menuBotLeft.png");
    mnuBotRight = texLoadPNGFile("romfs:/img/fb/menuBotRight.png");
    switch(ui::thmID)
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
            break;

        default:
            //Light corners
            cornerTopLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
            cornerTopRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
            cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
            cornerBottomRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotRight.png");
            progCovLeft = texLoadPNGFile("romfs:/img/tboxLght/progBarCoverLeftLight.png");
            progCovRight = texLoadPNGFile("romfs:/img/tboxLght/progBarCoverRightLight.png");

            icn = texLoadPNGFile("romfs:/img/icn/icnLght.png");
            sideBar = texLoadPNGFile("romfs:/img/fb/lDark.png");
            break;
    }

    top = texCreate(1280, 88);
    bot = texCreate(1280, 72);
    diaBox = texCreate(640, 420);

    //Setup dialog box
    drawTextbox(diaBox, 0, 0, 640, 420);
    drawRect(diaBox, 0, 56, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));

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

    //Setup top and bottom gfx
    texClearColor(top, clearClr);
    texDraw(icn, top, 66, 27);
    drawText("JKSV", top, shared, 130, 38, 24, ui::txtCont);
    drawRect(top, 30, 87, 1220, 1, ui::txtCont);

    texClearColor(bot, clearClr);
    drawRect(bot, 30, 0, 1220, 1, ui::txtCont);
    sprintf(verStr, "v. %02d.%02d.%04d", BLD_MON, BLD_DAY, BLD_YEAR);
    drawText(verStr, bot, shared, 8, author == "NULL" ? 56 : 38, 12, ui::txtCont);
    if(author != "NULL")
        drawText(std::string("Translation: " + author).c_str(), bot, ui::shared, 8, 56, 12, ui::txtCont);

    //Not needed anymore
    texDestroy(icn);

    //Create graphics to hold guides
    usrGuide = texCreate(textGetWidth(userHelp.c_str(), ui::shared, 18), 28);
    ttlGuide = texCreate(textGetWidth(titleHelp.c_str(), ui::shared, 18), 28);
    fldrGuide = texCreate(textGetWidth(folderHelp.c_str(), ui::shared, 18), 28);
    optGuide = texCreate(textGetWidth(optHelp.c_str(), ui::shared, 18), 28);

    //Clear with bg color
    texClearColor(usrGuide, ui::clearClr);
    texClearColor(ttlGuide, ui::clearClr);
    texClearColor(fldrGuide, ui::clearClr);
    texClearColor(optGuide, ui::clearClr);

    //Draw text to them
    drawText(userHelp.c_str(), usrGuide, ui::shared, 0, 3, 18, ui::txtCont);
    drawText(titleHelp.c_str(), ttlGuide, ui::shared, 0, 3, 18, ui::txtCont);
    drawText(folderHelp.c_str(), fldrGuide, ui::shared, 0, 3, 18, ui::txtCont);
    drawText(optHelp.c_str(), optGuide, ui::shared, 0, 3, 18, ui::txtCont);

    //Calculate x position of help text
    userHelpX = 1220 - usrGuide->width;
    titleHelpX = 1220 - ttlGuide->width;
    folderHelpX = 1220 - fldrGuide->width;
    optHelpX = 1220 - optGuide->width;

    advCopyMenuPrep();
    ui::exMenuPrep();
    ui::optMenuInit();
}

void ui::exit()
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

    texDestroy(usrGuide);
    texDestroy(ttlGuide);
    texDestroy(fldrGuide);
    texDestroy(optGuide);

    texDestroy(top);
    texDestroy(bot);
    texDestroy(diaBox);

    alphaMaskDestroy(iconMask);

    fontDestroy(shared);
}

void ui::showLoadScreen()
{
    tex *icn = texLoadJPEGFile("romfs:/icon.jpg");
    gfxBeginFrame();
    texClearColor(frameBuffer, clrCreateU32(0xFF2D2D2D));
    texDrawNoAlpha(icn, frameBuffer, 512, 232);
    drawText("Loading...", frameBuffer, ui::shared, 1100, 673, 16, clrCreateU32(0xFFFFFFFF));
    gfxEndFrame();
    texDestroy(icn);
}

void ui::drawUI()
{
    texClearColor(frameBuffer, clearClr);
    texDrawNoAlpha(top, frameBuffer, 0, 0);
    texDrawNoAlpha(bot, frameBuffer, 0, 648);

    switch(mstate)
    {
        case USR_SEL:
            texDrawNoAlpha(usrGuide, frameBuffer, userHelpX, 673);
            ui::drawUserMenu();
            break;

        case TTL_SEL:
            texDrawNoAlpha(ttlGuide, frameBuffer, titleHelpX, 673);
            ui::drawTitleMenu();
            break;

        case FLD_SEL:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(fldrGuide, frameBuffer, folderHelpX, 673);
            ui::drawFolderMenu();
            break;

        case TXT_USR:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(usrGuide, frameBuffer, userHelpX, 673);
            ui::drawTextUserMenu();
            break;

        case TXT_TTL:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(ttlGuide, frameBuffer, titleHelpX, 673);
            ui::drawTextTitleMenu();
            break;

        case TXT_FLD:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(fldrGuide, frameBuffer, folderHelpX, 673);
            ui::drawTextFolderMenu();
            break;

        case EX_MNU:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            ui::drawExMenu();
            break;

        case OPT_MNU:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(optGuide, frameBuffer, optHelpX, 673);
            ui::drawOptMenu();
            break;

        case ADV_MDE:
            drawRect(frameBuffer, 640, 88, 1, 559, ui::txtCont);
            drawAdvMode();
            break;
    }
}

void ui::drawBoundBox(int x, int y, int w, int h, int clrSh)
{
    clr rectClr = clrCreateRGBA(0x00, 0x88 + clrSh, 0xC5 + (clrSh / 2), 0xFF);

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
            drawRect(frameBuffer, x + 4, y + 4, w - 8, h - 8, clrCreateU32(0xFF212221));
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

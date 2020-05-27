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

#define VER_STRING "v. 05.26.2020"

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

//Shared font
font *ui::shared;

//Don't waste time drawing top and bottom over and over
//guide graphics are to save cpu drawing that over and over with alpha
static tex *top, *bot, *usrGuide, *ttlGuide, *fldrGuide, *optGuide;

//UI text strings
//Freedom English stored by default
std::string author = "NULL";
std::string ui::userHelp = "[A] Select   [Y] Dump All   [X] UI Mode   [-] Options   [ZR] Extras";
std::string ui::titleHelp = "[A] Select   [L][R] Change User   [Y] Dump All   [X] Favorite   [-] BlackList   [ZR] Erase   [B] Back";
std::string ui::folderHelp = "[-] File Mode  [L]/[R]+[A] Auto  [A] Backup  [Y] Restore  [X] Delete Folder  [ZR] Erase  [B] Back";
std::string ui::optHelp = "[A] Toggle   [B] Back";
std::string ui::yt = "Yes [A]", ui::nt = "No  [B]";
std::string ui::on = ">On>", ui::off = "Off";
std::string ui::confBlacklist = "Are you sure you want to add #%s# to your blacklist?";
std::string ui::confOverwrite = "Are you sure you want to overwrite #%s#?";
std::string ui::confRestore = "Are you sure you want to restore #%s#?";
std::string ui::confDel = "Are you sure you want to delete #%s#? *This is permanent*!";
std::string ui::confCopy = "Are you sure you want to copy #%s# to #%s#?";
std::string ui::confirmHead = "Confirm";
std::string ui::confEraseNand = "*WARNING*: This *will* erase the save data for #%s# *from your system*. This is the same as deleting it from #Data Management#! Are you sure you want to continue?";
std::string ui::confEraseFolder = "*WARNING*: This *will* delete the current save data for #%s# *from your system*! Are you sure you want to continue?";
std::string ui::advMenuStr[6] = { "Copy to ", "Delete", "Rename", "Make Dir", "Properties", "Close" };
std::string ui::exMenuStr[10] = { "SD to SD Browser", "BIS: PRODINFOF", "BIS: SAFE", "BIS: SYSTEM", "BIS: USER", "Remove Update", "Terminate Process", "Mount System Save", "Rescan Titles", "Mount Process RomFS" };
std::string ui::optMenuStr[12] = { "Include Dev Sv: ", "AutoBackup: ", "Overclock: ", "Hold to Delete: ", "Hold to Restore: ", "Hold to Overwrite: ", "Force Mount: ", "Account Sys. Saves: ", "Write to Sys. Saves: ", "Text UI Mode: ", "Direct FS Cmd: ", "Skip User Select: " };
std::string ui::optMenuExp[12] =
{
    "Includes Device Save data in user accounts.",
    "Automatically creates a save backup before restoring a save.",
    "Applies a small CPU over clock to 1224Mhz at boot. This is the same speed developer units run at.",
    "Whether or not holding [A] is required when deleting folders and files.",
    "Whether or not holding [A] is required when restoring save data.",
    "Whether or not holding [A] is required when overwriting saves on SD.",
    "When enabled, JKSV will only load and show save data that can be opened. When disabled, everything found will be shown.",
    "When enabled, system save data tied to accounts will be shown.",
    "Controls whether system save data and partitions can have files and data written and deleted from them. *This can be extremely dangerous if you don't know what you're doing!*",
    "Changes the UI to be text menu based like the original JKSM for 3DS.",
    "Directly uses the Switch's FS commands to copy files instead of stdio.",
    "Skips the user selection screen and jumps directly to the first user account found."
};
std::string ui::holdingText[3] = { "(Hold) ", "(Keep Holding) ", "(Almost there!) " };

//X position of help texts. Calculated to make editing quicker/easier
static unsigned userHelpX, titleHelpX, folderHelpX, optHelpX;

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
            default:
                file += "en-US.txt";
                break;
        }
    }

    fs::dataFile lang(file);
    while(lang.readNextLine(true))
    {
        std::string varName = lang.getName();
        //Holy shit this'll be fun
        if(varName == "author")
            author = lang.getNextValueStr();
        else if(varName == "userHelp")
            ui::userHelp = lang.getNextValueStr();
        else if(varName == "titleHelp")
            ui::titleHelp = lang.getNextValueStr();
        else if(varName == "folderHelp")
            ui::folderHelp = lang.getNextValueStr();
        else if(varName == "optHelp")
            ui::optHelp = lang.getNextValueStr();
        else if(varName == "yt")
            ui::yt = lang.getNextValueStr();
        else if(varName == "nt")
            ui::nt = lang.getNextValueStr();
        else if(varName == "on")
            ui::on = lang.getNextValueStr();
        else if(varName == "off")
            ui::off = lang.getNextValueStr();
        else if(varName == "confirmBlacklist")
            ui::confBlacklist = lang.getNextValueStr();
        else if(varName == "confirmOverwrite")
            ui::confOverwrite = lang.getNextValueStr();
        else if(varName == "confirmRestore")
            ui::confRestore = lang.getNextValueStr();
        else if(varName == "confirmDelete")
            ui::confDel = lang.getNextValueStr();
        else if(varName == "confirmCopy")
            ui::confCopy = lang.getNextValueStr();
        else if(varName == "confirmEraseNand")
            ui::confEraseNand = lang.getNextValueStr();
        else if(varName == "confirmEraseFolder")
            ui::confEraseFolder = lang.getNextValueStr();
        else if(varName == "confirmHead")
            ui::confirmHead = lang.getNextValueStr();
        else if(varName == "advMenu")
        {
            int ind = lang.getNextValueInt();
            ui::advMenuStr[ind] = lang.getNextValueStr();
        }
        else if(varName == "extMenu")
        {
            int ind = lang.getNextValueInt();
            ui::exMenuStr[ind] = lang.getNextValueStr();
        }
        else if(varName == "optMenu")
        {
            int ind = lang.getNextValueInt();
            ui::optMenuStr[ind] = lang.getNextValueStr();
        }
        else if(varName == "optMenuExp")
        {
            int ind = lang.getNextValueInt();
            ui::optMenuExp[ind] = lang.getNextValueStr();
        }
        else if(varName == "holdingText")
        {
            int ind = lang.getNextValueInt();
            ui::holdingText[ind] = lang.getNextValueStr();
        }
        else
            ui::showMessage("*Translation File Error:*", "*%s* is not a known or valid string name.", varName.c_str());
    }
}

void ui::initTheme()
{
    if(fs::fileExists(fs::getWorkDir() + "font.ttf"))
        shared = fontLoadTTF(std::string(fs::getWorkDir() + "font.ttf").c_str());
    else
        shared = fontLoadSharedFonts();

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
    drawText(VER_STRING, bot, shared, 8, author == "NULL" ? 56 : 38, 12, ui::txtCont);
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
            break;

        case TTL_SEL:
            texDrawNoAlpha(ttlGuide, frameBuffer, titleHelpX, 673);
            break;

        case FLD_SEL:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(fldrGuide, frameBuffer, folderHelpX, 673);
            break;

        case TXT_USR:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(usrGuide, frameBuffer, userHelpX, 673);
            break;

        case TXT_TTL:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(ttlGuide, frameBuffer, titleHelpX, 673);
            break;

        case TXT_FLD:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(fldrGuide, frameBuffer, folderHelpX, 673);
            break;

        case EX_MNU:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            break;

        case OPT_MNU:
            texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
            texDrawNoAlpha(optGuide, frameBuffer, optHelpX, 673);
            break;

        case ADV_MDE:
            drawRect(frameBuffer, 640, 88, 1, 559, ui::txtCont);
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
    drawPopup(down);
}

#ifndef UI_H
#define UI_H

#include <vector>
#include <string>

#include "data.h"
#include "gfx.h"

//ui headers - split up to keep a bit more organized
#include "menu.h"
#include "miscui.h"
#include "txtui.h"
#include "uiupdate.h"

enum menuState
{
    USR_SEL,
    TTL_SEL,
    FLD_SEL,
    ADV_MDE,
    TXT_USR,
    TXT_TTL,
    TXT_FLD,
    EX_MNU,
    OPT_MNU
};

namespace ui
{
    //Text menus
    extern bool textMode;

    //Current menu/ui state
    extern int mstate, prevState;

    //Holds theme set id
    extern ColorSetId thmID;

    //Both UI modes need access to this
    extern std::string folderMenuInfo;

    //Strings since translation support soonish
    extern std::string userHelp, titleHelp, folderHelp, optHelp, \
    confBlackList, confOverwrite, confRestore, confDel, confCopy, \
    confEraseNand, confEraseFolder, yt, nt;

    /*Colors
        clearClr = color to clear buffer
        txtCont = text that contrasts clearClr
        txtDiag = text color for dialogs
    */
    extern clr clearClr, txtCont, txtDiag, rectLt, rectSh, tboxClr, sideRect;

    //Textbox graphics
    extern tex *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;
    //Covers left and right of progress bar to fake being not a rectangle.
    extern tex *progCovLeft, *progCovRight, *diaBox;

    //Side bar from Freebird. RIP. NEVERMIND
    extern tex *sideBar;

    //Shared font
    extern font *shared;

    //Sets colors and loads font for icon creation
    void initTheme();

    //Loads graphics and stuff
    void init();
    void exit();

    //Just draws a screen and flips JIC boot takes long.
    void showLoadScreen();

    //Clears and draws general stuff used by multiple screens
    void drawUI();

    //Draws selection box
    void drawBoundBox(int x, int y, int w, int h, int clrSh);

    //switch case so we don't have problems with multiple main loops like 3DS
    void runApp(const uint64_t& down, const uint64_t& held);
}

#endif

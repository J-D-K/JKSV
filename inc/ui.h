#ifndef UI_H
#define UI_H

#include <vector>
#include <string>

#include "data.h"
#include "gfx.h"

//ui headers - split up to keep a bit more organized
#include "menu.h"
#include "miscui.h"
#include "clsui.h"
#include "uiupdate.h"

enum menuState
{
    USR_SEL,
    TTL_SEL,
    FLD_SEL,
    ADV_MDE,
    CLS_USR,
    CLS_TTL,
    CLS_FLD,
    EX_MNU
};


namespace ui
{
    //Classic mode/text menus
    extern bool clsMode;

    //Current menu/ui state
    extern int mstate, prevState;

    //Holds theme set id
    extern ColorSetId thmID;

    //Both UI modes need access to this
    extern std::string folderMenuInfo;

    //Colors to use now that I added theme detection
    /*NEED TO COME UP WITH BETTER NAMES*/
    extern clr clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr, sideRect;

    //Textbox graphics
    extern tex *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;

    //Side bar from Freebird. RIP
    extern tex *sideBar;

    //Shared font
    extern font *shared;

    extern std::vector<ui::button> selButtons;

    //Sets colors and stuff
    void initTheme();

    //Loads in the A, B, X, Y button graphics
    void init();
    void exit();

    //Prepares ui
    //Sets up buttons for icon touchin
    void setupSelButtons();
    void setupNavButtons();

    //Clears and draws general stuff used by multiple screens
    void drawUI();

    //Draws selection box
    void drawBoundBox(int x, int y, int w, int h, int clrSh);

    //switch case so we don't have problems with multiple main loops like 3DS
    void runApp(const uint64_t& down, const uint64_t& held, const touchPosition& p);
}

#endif

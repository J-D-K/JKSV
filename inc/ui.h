#ifndef UI_H
#define UI_H

#include <switch.h>
#include <vector>
#include <string>

#include "data.h"
#include "uistr.h"
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

    //pad data cause i don't know where else to put it
    extern PadState pad;
    static inline void updatePad() { padUpdate(&pad); }
    inline uint64_t padKeysDown() { return padGetButtonsDown(&pad); }
    inline uint64_t padKeysHeld() { return padGetButtons(&pad); }
    inline uint64_t padKeysUp() { return padGetButtonsUp(&pad); }

    inline void changeState(int newState)
    {
        prevState = mstate;
        mstate = newState;
    }

    //Holds theme set id
    extern ColorSetId thmID;

    //Both UI modes need access to thi
    extern std::string folderMenuInfo;

    /*Colors
        clearClr = color to clear buffer
        txtCont = text that contrasts clearClr
        txtDiag = text color for dialogs
    */
    extern SDL_Color clearClr, txtCont, txtDiag, rectLt, rectSh, tboxClr, sideRect;

    //Textbox graphics
    extern SDL_Texture *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;
    //Covers left and right of progress bar to fake being not a rectangle.
    extern SDL_Texture *progCovLeft, *progCovRight, *diaBox;

    //Side bar from Freebird. RIP.
    extern SDL_Texture *sideBar;

    //Sets colors and loads font for icon creation
    void initTheme();

    //Loads graphics and stuff
    void init();
    void exit();

    void showLoadScreen();

    //Clears and draws general stuff used by multiple screens
    void drawUI();

    //Draws selection box
    void drawBoundBox(int x, int y, int w, int h, int clrSh);

    //switch case so we don't have problems with multiple main loops like 3DS
    void runApp(const uint64_t& down, const uint64_t& held);

    //These are shared by both folder menus
    void createNewBackup(const uint64_t& held);
    void overwriteBackup(unsigned ind);
    void restoreBackup(unsigned ind);
    void deleteBackup(unsigned ind);
}

#endif

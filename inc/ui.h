#pragma once

#include <switch.h>
#include <vector>
#include <string>

#include "data.h"
#include "gfx.h"

//ui headers - split up to keep a bit more organized
#include "miscui.h"
#include "uistr.h"
#include "usr.h"
#include "ttl.h"
#include "sett.h"
#include "fm.h"

enum menuState
{
    USR_SEL,
    TTL_SEL,
    ADV_MDE,
    EX_MNU,
    OPT_MNU
};

namespace ui
{
    //Current menu/ui state
    extern int mstate, prevState;

    //Slide/animation scaling
    extern float animScale;

    //Loading glyph
    extern const std::string loadGlyphArray[];

    //pad data cause i don't know where else to put it
    extern PadState pad;
    extern HidTouchScreenState touchState;
    static inline void updateInput() { touchState = {0}; padUpdate(&pad); hidGetTouchScreenStates(&touchState, 1); }
    inline uint64_t padKeysDown() { return padGetButtonsDown(&pad); }
    inline uint64_t padKeysHeld() { return padGetButtons(&pad); }
    inline uint64_t padKeysUp() { return padGetButtonsUp(&pad); }
    inline void touchGetXY(uint32_t *x, uint32_t *y) { *x = touchState.touches[0].x; *y = touchState.touches[0].y; }

    inline void changeState(int newState)
    {
        prevState = mstate;
        mstate = newState;
    }

    //Holds theme set id
    extern ColorSetId thmID;

    //Both UI modes need access to this
    extern std::string folderMenuInfo;

    /*Colors
        clearClr = color to clear buffer
        txtCont = text that contrasts clearClr
        txtDiag = text color for dialogs
    */
    extern SDL_Color clearClr, transparent, txtCont, txtDiag, rectLt, rectSh, tboxClr, sideRect, divClr, heartColor, slidePanelColor, loadGlyphClr;

    //Textbox graphics
    extern SDL_Texture *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;
    //Menu bounding
    extern SDL_Texture *mnuTopLeft, *mnuTopRight, *mnuBotLeft, *mnuBotRight;

    //Covers left and right of progress bar to fake being not a rectangle.
    extern SDL_Texture *progCovLeft, *progCovRight, *diaBox;

    //Side bar from Freebird. RIP.
    extern SDL_Texture *sideBar;

    //Sets colors and loads font for icon creation
    void initTheme();

    //Loads graphics and stuff
    void init();
    void exit();

    //Adds a panel pointer to a vector since they need to be drawn over everything else
    int registerMenu(ui::menu *m);
    int registerPanel(ui::slideOutPanel *sop);
    int newThread(ThreadFunc func, void *args);

    //Just draws a screen and flips JIC boot takes long.
    void showLoadScreen();

    //Clears and draws general stuff used by multiple screens
    void drawUI();

    //switch case so we don't have problems with multiple main loops like 3DS
    bool runApp();

    void showPopMessage(int frameCount, const char *fmt, ...);

    //Used for multiple menu functions/callback
    void toTTL(void *);
}

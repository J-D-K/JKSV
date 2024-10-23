#pragma once

#include "dir.h"
#include "ui.h"

namespace ui
{
    //extern ui::menu *fldMenu;
    // I was tired of this conflict holding shit up.
    class slideOutPanel;
    extern ui::slideOutPanel *fldPanel;
    //extern fs::dirList *fldList;

    void fldInit();
    void fldExit();
    void fldUpdate();

    //Populate to open menu, refresh for updating after actions
    void fldPopulateMenu();
    void fldRefreshMenu();
} // namespace ui

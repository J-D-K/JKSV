#pragma once

#include "ui.h"
#include "dir.h"

namespace ui
{
    //extern ui::menu *fldMenu;
    extern ui::slideOutPanel *fldPanel;
    //extern fs::dirList *fldList;

    void fldInit();
    void fldExit();
    void fldUpdate();

    //Populate to open menu, refresh for updating after actions
    void fldPopulateMenu();
    void fldRefreshMenu();
}
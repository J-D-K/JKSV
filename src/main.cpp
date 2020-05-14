#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <switch.h>

#include "gfx.h"
#include "data.h"
#include "ui.h"
#include "file.h"

extern "C"
{
    void userAppInit(void)
    {
        appletInitialize();
        romfsInit();
        hidInitialize();
        nsInitialize();
        setsysInitialize();
        setInitialize();
        accountInitialize(AccountServiceType_System);
        pmshellInitialize();
    }

    void userAppExit(void)
    {
        appletExit();
        romfsExit();
        hidExit();
        nsExit();
        setsysExit();
        setExit();
        accountExit();
        pmshellExit();
    }
}

int main(int argc, const char *argv[])
{
    fs::init();
    graphicsInit(1280, 720);
    //Needed for icon gen
    ui::initTheme();

    /*Not completely stable yet
    Thread uiInitThrd;
    threadCreate(&uiInitThrd, ui::init, NULL, NULL, 0x80000, 0x2B, -2);
    threadStart(&uiInitThrd);*/
    data::init();
    ui::init(NULL);
    /*threadWaitForExit(&uiInitThrd);
    threadClose(&uiInitThrd);*/

    while(appletMainLoop())
    {
        hidScanInput();

        uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
        uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);

        touchPosition p;
        hidTouchRead(&p, 0);

        if(down & KEY_PLUS)
            break;

        gfxBeginFrame();
        ui::runApp(down, held, p);
        gfxEndFrame();
    }

    ui::exit();
    data::exit();
    graphicsExit();
    fs::exit();
}

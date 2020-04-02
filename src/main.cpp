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
    }
}

int main(int argc, const char *argv[])
{
    fs::init();
    graphicsInit(1280, 720);
    //Needed for icon gen
    ui::initTheme();
    data::loadDataInfo();
    ui::init();

    //built with 'make debug CFLAGS:=-D__debug__'
#ifdef __debug__
    socketInitializeDefault();
    nxlinkStdio();
#endif
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
#ifdef __debug__
    socketExit();
#endif
    ui::exit();
    data::exit();
    graphicsExit();
}

#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <switch.h>

#include "gfx.h"
#include "data.h"
#include "ui.h"
#include "file.h"
#include "util.h"

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
        //socketInitializeDefault();
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
        //socketExit();
    }
}

bool debDataStats = false;

int main(int argc, const char *argv[])
{
    fs::init();
    graphicsInit(1280, 720);
    ui::initTheme();
    ui::showLoadScreen();
    data::init();
    ui::init();

    while(appletMainLoop())
    {
        hidScanInput();

        uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
        uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);
        if(held & KEY_LSTICK && held & KEY_RSTICK)
            debDataStats = true;
        else if(down & KEY_PLUS)
            break;

        gfxBeginFrame();
        ui::runApp(down, held);

        if(debDataStats)
            data::dispStats();
        gfxEndFrame();
    }

    ui::exit();
    data::exit();
    graphicsExit();
    fs::exit();
}

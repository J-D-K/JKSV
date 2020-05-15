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
    //Max cpu to speed up boot. Doesn't take long.
    util::setCPU(1785000000);
    fs::init();
    graphicsInit(1280, 720);
    ui::initTheme();
    data::init();
    ui::init();
    //Reset cpu
    util::setCPU(data::ovrClk ? 1224000000 : 1020000000);

    while(appletMainLoop())
    {
        hidScanInput();

        uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
        uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);

        if(down & KEY_PLUS)
            break;

        gfxBeginFrame();
        ui::runApp(down, held);
        gfxEndFrame();
    }

    //reset cpu on exit
    if(data::ovrClk)
        util::setCPU(1020000000);

    ui::exit();
    data::exit();
    graphicsExit();
    fs::exit();
}

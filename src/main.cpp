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
        hidInitialize();
        nsInitialize();
        setsysInitialize();
        setInitialize();
        accountInitialize(AccountServiceType_System);
        pmshellInitialize();
        socketInitializeDefault();
        pdmqryInitialize();
    }

    void userAppExit(void)
    {
        appletExit();
        hidExit();
        nsExit();
        setsysExit();
        setExit();
        accountExit();
        pmshellExit();
        socketExit();
        pdmqryExit();
    }
}

bool debDataStats = false;

int main(int argc, const char *argv[])
{
    romfsInit();
    fs::init();
    graphicsInit(1280, 720);
    ui::initTheme();
    ui::showLoadScreen();
    data::init();
    ui::init();
    romfsExit();

    while(appletMainLoop())
    {
        ui::updatePad();

        uint64_t down = ui::padKeysDown(), held = ui::padKeysHeld();

        if(held & HidNpadButton_StickL && held & HidNpadButton_StickR)
            debDataStats = true;
        else if(down & HidNpadButton_Plus)
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

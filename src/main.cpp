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
        romfsInit();
        hidInitialize();
        nsInitialize();
        setsysInitialize();
        accountInitialize();
    }

    void userAppExit(void)
    {
        romfsExit();
        hidExit();
        nsExit();
        setsysExit();
        accountExit();
    }
}

int main(int argc, const char *argv[])
{
    fs::init();
    graphicsInit(1280, 720);
    data::loadDataInfo();
    ui::init();

    //built with 'make debug CFLAGS:=-D__debug__'
#ifdef __debug__
    socketInitializeDefault();
    nxlinkStdio();
#endif

    bool run = true;
    while(appletMainLoop() && run)
    {
        hidScanInput();

        uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
        uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);

        touchPosition p;
        hidTouchRead(&p, 0);

        if((held & KEY_L) && (held & KEY_R) && (held & KEY_ZL) && (held & KEY_ZR))
        {
            if(ui::confirm("You are about to enable system save dumping and remove checks. Are you sure you want to continue?"))
            {
                //Just to be sure
                fsdevUnmountDevice("sv");

                data::sysSave = true;
                if(ui::confirm("Do you want to disable isMountable Checks?"))
                    data::forceMount = false;
                data::loadDataInfo();

                //Kick back to user
                ui::mstate = ui::clsMode ? CLS_USR : USR_SEL;
            }
        }
        else if((held & KEY_ZL) && (held & KEY_ZR) && (held & KEY_Y) && ui::confirm("You are using this mode at your own risk."))
        {
            fsdevUnmountDevice("sv");
            data::curData.setType(FsSaveDataType_SystemSaveData);
            ui::devMenuPrep();
            ui::mstate = DEV_MNU;
        }
        else if(down & KEY_PLUS)
            break;

        ui::runApp(down, held, p);

        gfxHandleBuffs();
    }
#ifdef __debug__
    socketExit();
#endif

    graphicsExit();
    ui::exit();
    data::exit();
}

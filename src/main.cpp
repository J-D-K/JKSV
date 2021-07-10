#include <switch.h>

#include "gfx.h"
#include "file.h"
#include "data.h"
#include "ui.h"
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
        accountInitialize(AccountServiceType_Administrator);
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

int main(int argc, const char *argv[])
{
    romfsInit();
    fs::init();
    gfx::init();
    ui::initTheme();
    ui::showLoadScreen();
    data::init();
    ui::init();
    romfsExit();

    while(ui::runApp()){ }

    ui::exit();
    data::exit();
    gfx::exit();
    fs::exit();
}

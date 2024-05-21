#include <switch.h>
#include "jksv.hpp"
#include "log.hpp"

extern "C"
{
    void userAppInit()
    {
        appletInitialize();
        hidInitialize();
        nsInitialize();
        setsysInitialize();
        setInitialize();
        accountInitialize(AccountServiceType_Application);
        pmshellInitialize();
        socketInitializeDefault();
        pdmqryInitialize();
        plInitialize(PlServiceType_User);
        romfsInit();
    }

    void userAppExit()
    {
        romfsExit();
        plExit();
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
    nxlinkStdio();
    jksv::init();
    while(jksv::isRunning())
    {
        jksv::update();
        jksv::render();
    }
    jksv::exit();
    return 0;
}
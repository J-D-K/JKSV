#include "Config.hpp"
#include "JKSV.hpp"
#include <switch.h>

int main(void)
{
    JKSV Jksv{};
    while (Jksv.IsRunning())
    {
        Jksv.Update();
        Jksv.Render();
    }
    Config::Save();
    return 0;
}

extern "C" {

// constructor thats called before main
void userAppInit(void) {
    Result rc;

    if (R_FAILED(rc = appletLockExit()))
        diagAbortWithResult(rc);
}

// destructor thats called after main
void userAppExit(void) {
    appletUnlockExit();
}

} // extern "C"

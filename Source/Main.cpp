#include "Config.hpp"
#include "JKSV.hpp"
#include <switch.h>

int main(void)
{
    JKSV Jksv;
    while (appletMainLoop() && Jksv.IsRunning())
    {
        Jksv.Update();
        Jksv.Render();
    }
    Config::Save();
    return 0;
}

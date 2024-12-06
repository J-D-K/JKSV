#include "JKSV.hpp"

int main(void)
{
    JKSV Jksv;
    while (Jksv.IsRunning())
    {
        Jksv.Update();
        Jksv.Render();
    }
    return 0;
}

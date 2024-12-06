#pragma once
#include <switch.h>

namespace Input
{
    void Initialize(void);
    void Update(void);
    bool ButtonPressed(HidNpadButton Button);
    bool ButtonHeld(HidNpadButton Button);
    bool ButtonReleased(HidNpadButton Button);
} // namespace Input

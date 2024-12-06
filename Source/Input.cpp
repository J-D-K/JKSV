#include "Input.hpp"

namespace
{
    PadState s_Gamepad;
}

void Input::Initialize(void)
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&s_Gamepad);
}

void Input::Update(void)
{
    padUpdate(&s_Gamepad);
}

bool Input::ButtonPressed(HidNpadButton Button)
{
    return (s_Gamepad.buttons_cur & Button) && !(s_Gamepad.buttons_old & Button);
}

bool Input::ButtonHeld(HidNpadButton Button)
{
    return (s_Gamepad.buttons_cur & Button) && (s_Gamepad.buttons_old & Button);
}

bool Input::ButtonReleased(HidNpadButton Button)
{
    return (s_Gamepad.buttons_old & Button) && !(s_Gamepad.buttons_cur & Button);
}

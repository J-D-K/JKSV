#include <cstring>
#include "system/input.hpp"
#include "log.hpp"

namespace
{
    PadState s_PadState;   
}

void sys::input::init(void)
{
    padInitializeDefault(&s_PadState);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    logger::log("input::init(): Succeeded.");
}

void sys::input::update(void)
{
    padUpdate(&s_PadState);
}

bool sys::input::buttonDown(const HidNpadButton &button)
{
    return padGetButtonsDown(&s_PadState) & button;
}

bool sys::input::buttonHeld(const HidNpadButton &button)
{
    return padGetButtons(&s_PadState) & button;
}

std::string sys::input::getString(const std::string &defaultText, const std::string &headerText, const size_t &maximumLength)
{
    SwkbdConfig config;
    char inputBuffer[maximumLength + 1];

    // Clear inputBuffer
    std::memset(inputBuffer, 0x00, maximumLength + 1);

    // Setup soft keyboard
    swkbdCreate(&config, 0);
    swkbdConfigMakePresetDefault(&config);
    swkbdConfigSetInitialText(&config, defaultText.c_str());
    swkbdConfigSetHeaderText(&config, headerText.c_str());
    swkbdConfigSetGuideText(&config, headerText.c_str());
    swkbdConfigSetStringLenMax(&config, maximumLength);
    swkbdConfigSetKeySetDisableBitmask(&config, SwkbdKeyDisableBitmask_Backslash | SwkbdKeyDisableBitmask_Percent);

    Result showKeyboard = swkbdShow(&config, inputBuffer, maximumLength + 1);
    if(R_FAILED(showKeyboard))
    {
        logger::log("Error showing keyboard: 0x%X", showKeyboard);
    }
    return std::string(inputBuffer);
}
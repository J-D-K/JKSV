#pragma once
#include <switch.h>
#include <string>

namespace sys
{
    namespace input
    {
        // Inits pad
        void init(void);

        // Updates pad
        void update(void);

        // Returns if button was pressed
        bool buttonDown(HidNpadButton button);
        bool buttonHeld(HidNpadButton button);
        bool buttonReleased(HidNpadButton button);

        uint64_t buttonsDown(void);
        uint64_t buttonsHeld(void);
        uint64_t buttonsReleased(void);

        // Gets input and returns C++ string
        std::string getString(const std::string &defaultText, const std::string &headerText, size_t maximumLength);
    }
}
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
        bool buttonDown(const HidNpadButton &button);
        bool buttonHeld(const HidNpadButton &button);

        // Gets input and returns C++ string
        std::string getString(const std::string &defaultText, const std::string &headerText, const size_t &maximumLength);
    }
}
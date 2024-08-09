#pragma once
#include <string>

namespace ui
{
    namespace strings
    {
        // This inits strings/loads translations from romfs
        void init(void);

        // These return strings for ui.
        std::string getString(const std::string &name, int index);
        const char *getCString(const std::string &name, int index);
    }
}
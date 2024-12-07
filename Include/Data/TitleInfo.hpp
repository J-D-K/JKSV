#pragma once
#include "SDL.hpp"
#include <cstdint>
#include <switch.h>

namespace Data
{
    class TitleInfo
    {
        public:
            // Loads control data and icon.
            TitleInfo(uint64_t ApplicationID);

            // Returns title.
            const char *GetTitle(void);
            // Returns publisher
            const char *GetPublisher(void);

            // Returns icon
            SDL::SharedTexture GetIcon(void) const;

        private:
            // This is where all the important stuff is.
            NacpStruct m_NACP;
            // This is the icon.
            SDL::SharedTexture m_Icon = nullptr;
    };
} // namespace Data

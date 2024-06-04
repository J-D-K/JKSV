#pragma once
#include <string>
#include <switch.h>
#include <SDL2/SDL.h>

namespace data
{
    class titleInfo
    {
        public:
            // Constructor loads data for title
            titleInfo(const uint64_t &titleID);
            // Returns title as UTF-8 from NACP
            std::string getTitle(void);
            // Returns the path safe version of above
            std::string getPathSafeTitle(void);
            // Returns icon texture
            SDL_Texture *getIcon(void);
            // Returns journal size
            uint64_t getJournalSize(const FsSaveDataType &saveType);

        private:
            // Title ID is needed for a few things
            uint64_t m_TitleID;
            // NACP contains all the fun important stuff
            NacpStruct m_Nacp;
            // Pointer to icon
            SDL_Texture *m_Icon;
    };
}

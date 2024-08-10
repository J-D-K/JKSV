#pragma once
#include <string>

#include <switch.h>
#include <SDL2/SDL.h>

#include "graphics/graphics.hpp"

namespace data
{
    class titleInfo
    {
        public:
            // Constructor loads data for title
            titleInfo(uint64_t titleID);
            // Returns title as UTF-8 from NACP
            std::string getTitle(void);
            // Returns the path safe version of above
            std::string getPathSafeTitle(void);
            // Returns icon texture
            graphics::sdlTexture getIcon(void);
            // Returns save data size
            int64_t getSaveDataSize(FsSaveDataType saveType);
            int64_t getSaveDataSizeMax(FsSaveDataType saveType);
            // Returns journal size
            int64_t getJournalSize(FsSaveDataType saveType);
            int64_t getJournalSizeMax(FsSaveDataType saveType);
            // Returns save data owner id from NACP
            uint64_t getSaveDataOwnerID(void);
            // Return if title has data for save type
            bool hasAccountSaveData(void);
            bool hasBCATSaveData(void);
            bool hasDeviceSaveData(void);
            bool hasCacheSaveData(void);

        private:
            // Title ID is needed for a few things
            uint64_t m_TitleID;
            // NACP contains all the fun important stuff
            NacpStruct m_Nacp;
            // Pointer to icon
            graphics::sdlTexture m_Icon;
    };
}

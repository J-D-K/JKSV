#pragma once
#include <string>
#include <cstdint>
#include <switch.h>

namespace data
{
    class userSaveInfo
    {
        public:
            // Inits data
            userSaveInfo(const uint64_t& titleID, const FsSaveDataInfo &saveDataInfo, const PdmPlayStatistics &playStats);

            // These do not return references or pointers so they can't be changed
            uint64_t getTitleID(void) const;
            FsSaveDataInfo getSaveDataInfo(void) const;
            PdmPlayStatistics getPlayStatistics(void) const;

        private:
            // Title ID/Application ID
            uint64_t m_TitleID;
            // Save info
            FsSaveDataInfo m_SaveDataInfo;
            // Play stats
            PdmPlayStatistics m_PlayStats;
    };
}
#include "data/userSaveInfo.hpp"

data::userSaveInfo::userSaveInfo(const uint64_t &titleID, const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats) : m_TitleID(titleID), m_SaveDataInfo(saveInfo), m_PlayStats(playStats) { }

uint64_t data::userSaveInfo::getTitleID(void) const
{
    return m_TitleID;
}

FsSaveDataInfo data::userSaveInfo::getSaveDataInfo(void) const
{
    return m_SaveDataInfo;
}

PdmPlayStatistics data::userSaveInfo::getPlayStatistics(void) const
{
    return m_PlayStats;
}
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstring>
#include <switch.h>
#include "data/data.hpp"
#include "ui/ui.hpp"
#include "log.hpp"

namespace
{
    std::vector<std::pair<u128, data::user>> s_UserVector;
    std::unordered_map<uint64_t, data::titleInfo> s_TitleMap;
}

// Converts AccountUid to uint128
static inline u128 accountUIDToU128(const AccountUid &accountID)
{
    return ((u128)accountID.uid[0] << 64 | accountID.uid[1]);
}

// Same as above, but reversed
static inline AccountUid u128ToAccountUID(const u128 &accountID)
{
    return (AccountUid){ (uint64_t)(accountID >> 64), (uint64_t)accountID };
}

// Checks if title exists in map
static inline bool titleIsLoadedInMap(const uint64_t &titleID)
{
    return s_TitleMap.find(titleID) != s_TitleMap.end();
}

bool data::init(void)
{
    // Get user account list
    AccountUid accountIDs[8];
    int totalAccounts = 0;
    Result accountList = accountListAllUsers(accountIDs, 8, &totalAccounts);
    if (R_FAILED(accountList))
    {
        logger::log("accountListAllUsers failed: 0x%X.", accountList);
        return false;
    }

    // Loop through results and load them to vector
    for (int i = 0; i < totalAccounts; i++)
    {
        s_UserVector.push_back(std::make_pair(accountUIDToU128(accountIDs[i]), data::user(accountIDs[i])));
    }

    // Add system users
    std::string deviceString = ui::strings::getString(LANG_SAVE_TYPE_MAIN_MENU, 0);
    std::string bcatString = ui::strings::getString(LANG_SAVE_TYPE_MAIN_MENU, 1);
    std::string cacheString = ui::strings::getString(LANG_SAVE_TYPE_MAIN_MENU, 2);
    std::string systemString = ui::strings::getString(LANG_SAVE_TYPE_MAIN_MENU, 3);
    s_UserVector.push_back(std::make_pair(3, data::user(u128ToAccountUID(3), deviceString, "Device")));
    s_UserVector.push_back(std::make_pair(4, data::user(u128ToAccountUID(2), bcatString, "BCAT")));
    s_UserVector.push_back(std::make_pair(5, data::user(u128ToAccountUID(5), cacheString, "Cache")));
    s_UserVector.push_back(std::make_pair(0, data::user(u128ToAccountUID(0), systemString, "System")));

    // Do the same except for title records
    NsApplicationRecord currentRecord;
    int entryCount = 0;
    int entryOffset = 0;
    while(R_SUCCEEDED(nsListApplicationRecord(&currentRecord, 1, entryOffset++, &entryCount)) && entryCount > 0)
    {
        logger::log("Add title 0x%016lX.", currentRecord.application_id);
        s_TitleMap.emplace(std::make_pair(currentRecord.application_id, data::titleInfo(currentRecord.application_id)));
    }

    data::loadUserSaveInfo();

    return true;
}

void data::loadUserSaveInfo(void)
{
    // Clear userSaveInfo before JIC
    for(auto &u : s_UserVector)
    {
        u.second.clearUserSaveInfo();
    }

    // Originally, I went with fsOpenSaveDataInfoReaderWithFilter in the user class itself, but then I couldn't catch system saves for the title list
    FsSaveDataInfoReader saveDataInfoReader;
    FsSaveDataInfo saveDataInfo;
    int64_t entries = 0;

    Result openSaveInfoReader = fsOpenSaveDataInfoReader(&saveDataInfoReader, FsSaveDataSpaceId_All);
    if(R_FAILED(openSaveInfoReader))
    {
        logger::log("fsOpenSaveDataInfoReader failed: 0x%X", openSaveInfoReader);
        return;
    }

    // Loop and load all saves and push them to users
    while(R_SUCCEEDED(fsSaveDataInfoReaderRead(&saveDataInfoReader, &saveDataInfo, 1, &entries)) && entries > 0)
    {
        // Title id depends on save type
        uint64_t titleID = 0;
        if(saveDataInfo.save_data_type == FsSaveDataType_System)
        {
            titleID = saveDataInfo.system_save_data_id;
        }
        else
        {
            titleID = saveDataInfo.application_id;
        }

        // Just in case it doesn't have and entry
        if(titleIsLoadedInMap(titleID) == false)
        {
            s_TitleMap.emplace(std::make_pair(titleID, data::titleInfo(titleID)));
        }

        // Handle system users
        switch (saveDataInfo.save_data_type)
        {
            case FsSaveDataType_Device:
            {
                saveDataInfo.uid = u128ToAccountUID(3);
            }
            break;

            case FsSaveDataType_Bcat:
            {
                saveDataInfo.uid = u128ToAccountUID(2);
            }
            break;

            case FsSaveDataType_Cache:
            {
                saveDataInfo.uid = u128ToAccountUID(5);
            }
            break;
        }

        // Play stats
        PdmPlayStatistics playStatistics;
        if(saveDataInfo.save_data_type == FsSaveDataType_Account || saveDataInfo.save_data_type == FsSaveDataType_Device)
        {
            pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(titleID, saveDataInfo.uid, false, &playStatistics);
        }
        else
        {
            std::memset(&playStatistics, 0x00, sizeof(PdmPlayStatistics));
        }

        // Finally push it to user
        data::user *targetUser = data::getUserByAccountID(accountUIDToU128(saveDataInfo.uid));
        if(targetUser != NULL)
        {
            targetUser->addNewUserSaveInfo(titleID, saveDataInfo, playStatistics);
        }
    }

    // Run sort
    data::sortUserSaveInfo();
}

void data::sortUserSaveInfo(void)
{
    for(auto &u : s_UserVector)
    {
        u.second.sortUserSaveInfo();
    }
}

int data::getTotalUsers(void)
{
    return s_UserVector.size();
}

data::user *data::getUserByAccountID(const u128 &accountID)
{
    auto userPosition = std::find_if(s_UserVector.begin(), s_UserVector.end(), [accountID](const std::pair<u128, data::user> &u){ return u.first == accountID; });
    if(userPosition != s_UserVector.end())
    {
        return &userPosition->second;
    }
    return NULL;
}

data::user *data::getUserAtPosition(const int &position)
{
    if(position >= 0 && position < static_cast<int>(s_UserVector.size()))
    {
        return &s_UserVector.at(position).second;
    }
    return NULL;
}

data::titleInfo *data::getTitleInfoByTitleID(const uint64_t &titleID)
{
    if(s_TitleMap.find(titleID) != s_TitleMap.end())
    {
        return &s_TitleMap.at(titleID);
    }
    return NULL;
}
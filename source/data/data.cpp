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
#include "config.hpp"
#include "jksv.hpp"
#include "log.hpp"

namespace
{
    // This is to help make some stuff more readable... I hope
    using userIDPair = std::pair<u128, data::user>;
    // Using vector for Users to preserve order and access by index
    std::vector<userIDPair> s_UserVector;
    // Map of title data
    data::titleMap s_TitleMap;
    // Names of ui strings needed
    const std::string SAVE_TYPE_STRINGS = "saveDataTypeText";
    // This is to make stuff slightly easier to read. In the same order as ui::strings. 1 is account so missing here.
    constexpr int SYSTEM_SAVE_USER_ID = 0;
    constexpr int BCAT_SAVE_USER_ID = 2;
    constexpr int DEVICE_SAVE_USER_ID = 3;
    constexpr int TEMPORARY_SAVE_USER_ID = 4;
    constexpr int CACHE_SAVE_USER_ID = 5;
    constexpr int SYSTEM_BCAT_USER_ID = 6;
}

// Converts AccountUid to uint128
static u128 accountUIDToU128(AccountUid accountID)
{
    return ((u128)accountID.uid[0] << 64 | accountID.uid[1]);
}

// Same as above, but reversed
static AccountUid u128ToAccountUID(u128 accountID)
{
    return (AccountUid){(uint64_t)(accountID >> 64), (uint64_t)accountID};
}

// Checks if user exists in vector
static bool userExistsInVector(u128 accountID)
{
    // This is kind of hard to follow, but search the vector and see if accountID exists
    auto findUser = std::find_if(s_UserVector.begin(), s_UserVector.end(), [accountID](const userIDPair &userPair)
                                 { return userPair.first == accountID; });
    return findUser != s_UserVector.end();
}

// Checks if title exists in map
static bool titleIsLoadedInMap(uint64_t titleID)
{
    return s_TitleMap.find(titleID) != s_TitleMap.end();
}

// This is for adding the 'system' type users if/when they're found.
static void createAddSystemUser(int userTypeID)
{
    // Grab the string for save type
    std::string systemTypeString = ui::strings::getString(SAVE_TYPE_STRINGS, userTypeID);
    // Add it to vector.
    AccountUid accountUID = u128ToAccountUID(userTypeID);
    s_UserVector.push_back(std::make_pair(userTypeID, data::user(accountUID, systemTypeString, systemTypeString, data::userType::TYPE_SYSTEM)));
}

// This is for filtering system saves that are tied to accounts
static bool isAccountSystemSave(const FsSaveDataInfo &saveDataInfo)
{
    uint8_t saveDataType = saveDataInfo.save_data_type;
    u128 accountID = accountUIDToU128(saveDataInfo.uid);
    if (saveDataType == FsSaveDataType_System && accountID != 0)
    {
        return true;
    }
    return false;
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
        u128 userID128 = accountUIDToU128(accountIDs[i]);
        s_UserVector.push_back(std::make_pair(userID128, data::user(accountIDs[i], data::userType::TYPE_USER)));
    }

    // Add Initial system users there will probably be saves for
    std::string deviceUserString = ui::strings::getString(SAVE_TYPE_STRINGS, DEVICE_SAVE_USER_ID);
    std::string bcatUserString = ui::strings::getString(SAVE_TYPE_STRINGS, BCAT_SAVE_USER_ID);
    std::string cacheUserString = ui::strings::getString(SAVE_TYPE_STRINGS, CACHE_SAVE_USER_ID);
    std::string systemUserString = ui::strings::getString(SAVE_TYPE_STRINGS, SYSTEM_SAVE_USER_ID);
    s_UserVector.push_back(std::make_pair(DEVICE_SAVE_USER_ID, data::user(u128ToAccountUID(DEVICE_SAVE_USER_ID), deviceUserString, "Device", data::userType::TYPE_SYSTEM)));
    s_UserVector.push_back(std::make_pair(BCAT_SAVE_USER_ID, data::user(u128ToAccountUID(BCAT_SAVE_USER_ID), bcatUserString, "BCAT", data::userType::TYPE_SYSTEM)));
    s_UserVector.push_back(std::make_pair(CACHE_SAVE_USER_ID, data::user(u128ToAccountUID(CACHE_SAVE_USER_ID), cacheUserString, "Cache", data::userType::TYPE_SYSTEM)));
    s_UserVector.push_back(std::make_pair(SYSTEM_SAVE_USER_ID, data::user(u128ToAccountUID(SYSTEM_SAVE_USER_ID), systemUserString, "System", data::userType::TYPE_SYSTEM)));

    // Do the same except for title records
    NsApplicationRecord currentRecord;
    int entryCount = 0;
    int entryOffset = 0;
    while (R_SUCCEEDED(nsListApplicationRecord(&currentRecord, 1, entryOffset++, &entryCount)) && entryCount > 0)
    {
        // This is logged for when JKSV gets stuck on things
        logger::log("Add title 0x%016lX.", currentRecord.application_id);
        s_TitleMap.emplace(std::make_pair(currentRecord.application_id, data::titleInfo(currentRecord.application_id)));
    }
    data::loadUserSaveInfo();
    return true;
}

void data::loadUserSaveInfo(void)
{
    // Clear userSaveInfo before JIC
    for (auto &u : s_UserVector)
    {
        u.second.clearUserSaveInfo();
    }

    // Originally, I went with fsOpenSaveDataInfoReaderWithFilter in the user class itself, but then I couldn't catch system saves for the title list
    FsSaveDataInfoReader saveDataInfoReader;
    FsSaveDataInfo saveDataInfo;
    int64_t entries = 0;

    Result openSaveInfoReader = fsOpenSaveDataInfoReader(&saveDataInfoReader, FsSaveDataSpaceId_All);
    if (R_FAILED(openSaveInfoReader))
    {
        logger::log("fsOpenSaveDataInfoReader failed: 0x%X", openSaveInfoReader);
        return;
    }

    // Loop and load all saves and push them to users
    while (R_SUCCEEDED(fsSaveDataInfoReaderRead(&saveDataInfoReader, &saveDataInfo, 1, &entries)) && entries > 0)
    {
        // Title id depends on save type
        uint64_t titleID = 0;
        if (saveDataInfo.save_data_type == FsSaveDataType_System || saveDataInfo.save_data_type == FsSaveDataType_SystemBcat)
        {
            titleID = saveDataInfo.system_save_data_id;
        }
        else
        {
            titleID = saveDataInfo.application_id;
        }

        // Just in case it doesn't have and entry
        if (titleIsLoadedInMap(titleID) == false)
        {
            logger::log("Save found for for title not in records: 0x%016lX", titleID);
            s_TitleMap.emplace(std::make_pair(titleID, data::titleInfo(titleID)));
        }

        // Just skip this stuff. Comes after above JIC we need to load something.
        if (isAccountSystemSave(saveDataInfo) && config::getByKey(CONFIG_LIST_ACCOUNT_SYSTEM_SAVES) == false)
        {
            continue;
        }

        // This is so things get sorted right into the correct system users
        switch (saveDataInfo.save_data_type)
        {
            case FsSaveDataType_Bcat:
            {
                saveDataInfo.uid = u128ToAccountUID(BCAT_SAVE_USER_ID);
            }
            break;

            case FsSaveDataType_Device:
            {
                saveDataInfo.uid = u128ToAccountUID(DEVICE_SAVE_USER_ID);
            }
            break;

            case FsSaveDataType_Temporary:
            {
                // This is special for compatibility just in case.
                if (userExistsInVector(TEMPORARY_SAVE_USER_ID) == false)
                {
                    createAddSystemUser(TEMPORARY_SAVE_USER_ID);
                }
                saveDataInfo.uid = u128ToAccountUID(TEMPORARY_SAVE_USER_ID);
            }
            break;

            case FsSaveDataType_Cache:
            {
                saveDataInfo.uid = u128ToAccountUID(CACHE_SAVE_USER_ID);
            }
            break;

            // I've never seen this used before...
            case FsSaveDataType_SystemBcat:
            {
                if (userExistsInVector(SYSTEM_BCAT_USER_ID) == false)
                {
                    createAddSystemUser(SYSTEM_BCAT_USER_ID);
                }
                saveDataInfo.uid = u128ToAccountUID(SYSTEM_BCAT_USER_ID);
            }
            break;
        }

        // Play stats
        PdmPlayStatistics playStatistics;
        if (saveDataInfo.save_data_type == FsSaveDataType_Account || saveDataInfo.save_data_type == FsSaveDataType_Device)
        {
            pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(titleID, saveDataInfo.uid, false, &playStatistics);
        }
        else
        {
            std::memset(&playStatistics, 0x00, sizeof(PdmPlayStatistics));
        }

        // Finally push it to user
        data::user *targetUser = data::getUserByAccountID(accountUIDToU128(saveDataInfo.uid));
        if (targetUser != NULL)
        {
            targetUser->addNewUserSaveInfo(titleID, saveDataInfo, playStatistics);
        }
    }

    // Run sort
    data::sortUserSaveInfo();
}

void data::sortUserSaveInfo(void)
{
    for (userIDPair &userPair : s_UserVector)
    {
        userPair.second.sortUserSaveInfo();
    }
}

int data::getTotalUsers(void)
{
    return s_UserVector.size();
}

data::user *data::getUserByAccountID(u128 accountID)
{
    auto userPosition = std::find_if(s_UserVector.begin(), s_UserVector.end(), [accountID](const userIDPair &u)
                                     { return u.first == accountID; });
    if (userPosition != s_UserVector.end())
    {
        return &userPosition->second;
    }
    return NULL;
}

data::user *data::getUserAtPosition(int position)
{
    if (position >= 0 && position < static_cast<int>(s_UserVector.size()))
    {
        return &s_UserVector.at(position).second;
    }
    return NULL;
}

data::titleMap &data::getTitleMap(void)
{
    return s_TitleMap;
}

int data::getTotalTitleCount(void)
{
    return s_TitleMap.size();
}

data::titleInfo *data::getTitleInfoByTitleID(uint64_t titleID)
{
    if (s_TitleMap.find(titleID) != s_TitleMap.end())
    {
        return &s_TitleMap.at(titleID);
    }
    return NULL;
}
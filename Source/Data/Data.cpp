#include "Data/Data.hpp"
#include "Config.hpp"
#include "Data/AccountUID.hpp"
#include "FsLib.hpp"
#include "Logger.hpp"
#include "Strings.hpp"
#include <algorithm>
#include <array>
#include <switch.h>
#include <unordered_map>
#include <vector>

namespace
{
    // This is easer to read imo
    using UserIDPair = std::pair<AccountUid, Data::User>;
    // User vector to preserve order.
    std::vector<UserIDPair> s_UserVector;
    // Map of Title info paired with its title/application
    std::unordered_map<uint64_t, Data::TitleInfo> s_TitleInfoMap;
    // Array of SaveDataSpaceIDs - SaveDataSpaceAll doesn't seem to work as it should...
    constexpr std::array<FsSaveDataSpaceId, 7> s_SaveDataSpaceOrder = {FsSaveDataSpaceId_System,
                                                                       FsSaveDataSpaceId_User,
                                                                       FsSaveDataSpaceId_SdSystem,
                                                                       FsSaveDataSpaceId_Temporary,
                                                                       FsSaveDataSpaceId_SdUser,
                                                                       FsSaveDataSpaceId_ProperSystem,
                                                                       FsSaveDataSpaceId_SafeMode};

} // namespace

bool Data::Initialize(void)
{
    // Switch can only have up to 8 accounts.
    int TotalAccountsRead = 0;
    AccountUid AccountIDs[8];
    if (R_FAILED(accountListAllUsers(AccountIDs, 8, &TotalAccountsRead)))
    {
        Logger::Log("Error getting user list: 0x%X.", TotalAccountsRead);
        return false;
    }

    // Loop through and load all users found.
    for (int i = 0; i < TotalAccountsRead; i++)
    {
        Data::User NewUser(AccountIDs[i]);
        s_UserVector.push_back(std::make_pair(AccountIDs[i], std::move(NewUser)));
    }

    // "System" users.
    AccountUid DeviceID = {FsSaveDataType_Device};
    AccountUid BCATID = {FsSaveDataType_Bcat};
    AccountUid CacheID = {FsSaveDataType_Cache};
    AccountUid SystemID = {FsSaveDataType_System};

    s_UserVector.push_back(
        std::make_pair(DeviceID, Data::User({FsSaveDataType_Device}, Strings::GetByName(Strings::Names::SaveDataTypes, 3), "Device")));
    s_UserVector.push_back(
        std::make_pair(BCATID, Data::User({FsSaveDataType_Bcat}, Strings::GetByName(Strings::Names::SaveDataTypes, 2), "BCAT")));
    s_UserVector.push_back(
        std::make_pair(CacheID, Data::User({FsSaveDataType_Cache}, Strings::GetByName(Strings::Names::SaveDataTypes, 5), "Cache")));
    s_UserVector.push_back(
        std::make_pair(SystemID, Data::User({FsSaveDataType_System}, Strings::GetByName(Strings::Names::SaveDataTypes, 0), "System")));

    NsApplicationRecord CurrentRecord = {0};
    int EntryCount = 0, EntryOffset = 0;
    while (R_SUCCEEDED(nsListApplicationRecord(&CurrentRecord, 1, EntryOffset++, &EntryCount)) && EntryCount > 0)
    {
        s_TitleInfoMap.emplace(std::make_pair(CurrentRecord.application_id, Data::TitleInfo(CurrentRecord.application_id)));
    }

    for (int i = 0; i < 7; i++)
    {
        FsSaveDataInfo SaveInfo;
        FsSaveDataInfoReader SaveInfoReader;
        int64_t TotalEntries = 0;

        Result FsError = fsOpenSaveDataInfoReader(&SaveInfoReader, s_SaveDataSpaceOrder[i]);
        if (R_FAILED(FsError))
        {
            Logger::Log("Error opening save data reader with space ID %i.", i);
            continue;
        }

        while (R_SUCCEEDED(fsSaveDataInfoReaderRead(&SaveInfoReader, &SaveInfo, 1, &TotalEntries)) && TotalEntries > 0)
        {
            auto FindUser = std::find_if(s_UserVector.begin(), s_UserVector.end(), [&SaveInfo](UserIDPair &IDPair) {
                return IDPair.first == SaveInfo.uid;
            });

            // To do: Handle this right.
            if (FindUser == s_UserVector.end())
            {
                continue;
            }

            // This is for if we have system save data. It has no application ID.
            uint64_t ApplicationID = (SaveInfo.save_data_type == FsSaveDataType_System || SaveInfo.save_data_type == FsSaveDataType_SystemBcat)
                                         ? SaveInfo.system_save_data_id
                                         : SaveInfo.application_id;

            // Just in case.
            if (s_TitleInfoMap.find(ApplicationID) == s_TitleInfoMap.end())
            {
                s_TitleInfoMap.emplace(std::make_pair(ApplicationID, Data::TitleInfo(ApplicationID)));
            }

            PdmPlayStatistics PlayStats = {0};
            Result PDMError = pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(ApplicationID, SaveInfo.uid, false, &PlayStats);
            if (R_FAILED(PDMError))
            {
                // Logged, but not fatal.
                Logger::Log("Error getting play stats for %016llX: 0x%X", ApplicationID, PDMError);
            }

            FindUser->second.AddToMap(SaveInfo, PlayStats);
        }
    }
    return true;
}

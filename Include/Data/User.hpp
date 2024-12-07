#pragma once
#include "SDL.hpp"
#include <string>
#include <string_view>
#include <switch.h>
#include <unordered_map>

namespace Data
{
    class User
    {
        public:
            // This is for normal use accounts.
            User(AccountUid AccountID);
            // This is for system type accounts.
            User(AccountUid AccountID, std::string_view Nickname, std::string_view PathSafeNickname);

            // Adds Data to UserDataMap
            void AddToMap(const FsSaveDataInfo &SaveInfo, const PdmPlayStatistics &PlayStats);
            // Returns FsSaveDataInfo by ApplicationID or SystemSaveID.
            FsSaveDataInfo *GetSaveInfoByID(uint64_t ApplicationID);
            // Returns PlayStats according to ^
            PdmPlayStatistics *GetPlayStatsByID(uint64_t ApplicationID);

        private:
            // User's ID.
            AccountUid m_AccountID;
            // Nickname
            char m_Nickname[0x20];
            // Path safe nickname.
            char m_PathSafeNickname[0x20];
            // User's icon
            SDL::SharedTexture m_Icon = nullptr;
            // Map of FsSaveInfo and play stats
            std::unordered_map<uint64_t, std::pair<FsSaveDataInfo, PdmPlayStatistics>> m_UserDataMap;
            // Loads account using profile structs.
            void LoadAccount(AccountProfile &Profile, AccountProfileBase &ProfileBase);
            // Creates a placeholder since something went wrong getting profile info.
            void CreateAccount(void);
    };
} // namespace Data

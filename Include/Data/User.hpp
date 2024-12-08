#pragma once
#include "SDL.hpp"
#include <string>
#include <string_view>
#include <switch.h>
#include <vector>

namespace Data
{
    // This should make things fun to read :^)
    using UserDataEntry = std::pair<uint64_t, std::pair<FsSaveDataInfo, PdmPlayStatistics>>;

    class User
    {
        public:
            // This is for normal use accounts.
            User(AccountUid AccountID);
            // This is for system type accounts.
            User(AccountUid AccountID, std::string_view PathSafeNickname, std::string_view IconPath);

            // Adds Data to UserDataMap
            void AddData(const FsSaveDataInfo &SaveInfo, const PdmPlayStatistics &PlayStats);
            // Runs the title sorting algorithm.
            void SortData(void);
            // Returns nickname
            const char *GetNickname(void) const;
            // Returns "path safe" nickname
            const char *GetPathSafeNickname(void) const;
            // Returns total entries.
            size_t GetTotalDataEntries(void) const;
            // Returns application ID at index
            uint64_t GetApplicationIDAt(int Index) const;
            // Returns FsSaveDataInfo by index.
            FsSaveDataInfo *GetSaveInfoAt(int Index);
            // Returns PlayStats by ^
            PdmPlayStatistics *GetPlayStatsAt(int Index);
            // Returns FsSaveDataInfo by ApplicationID or SystemSaveID.
            FsSaveDataInfo *GetSaveInfoByID(uint64_t ApplicationID);
            // Returns PlayStats according to ^
            PdmPlayStatistics *GetPlayStatsByID(uint64_t ApplicationID);
            // Returns raw pointer to icon.
            SDL_Texture *GetIcon(void);
            // Returns shared pointer to icon and increases reference count.
            SDL::SharedTexture GetSharedIcon(void);

        private:
            // User's ID.
            AccountUid m_AccountID;
            // Nickname
            char m_Nickname[0x20] = {0};
            // Path safe nickname.
            char m_PathSafeNickname[0x20] = {0};
            // User's icon
            SDL::SharedTexture m_Icon = nullptr;
            // Map of FsSaveInfo and play stats
            std::vector<UserDataEntry> m_UserData;
            // Loads account using profile structs.
            void LoadAccount(AccountProfile &Profile, AccountProfileBase &ProfileBase);
            // Creates a placeholder since something went wrong getting profile info.
            void CreateAccount(void);
    };
} // namespace Data

#include "Data/User.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Data/Data.hpp"
#include "Logger.hpp"
#include "SDL.hpp"
#include "StringUtil.hpp"
#include <algorithm>
#include <cstring>

namespace
{
    constexpr int ICON_FONT_SIZE = 50;
}

// Function used to sort user data.
static bool SortUserData(const Data::UserDataEntry &EntryA, const Data::UserDataEntry &EntryB)
{
    auto &[AppIDA, DataA] = EntryA;
    auto &[AppIDB, DataB] = EntryB;
    auto &[SaveInfoA, PlayStatsA] = DataA;
    auto &[SaveInfoB, PlayStatsB] = DataB;

    // Favorites over all.
    if (Config::IsFavorite(AppIDA) != Config::IsFavorite(AppIDB))
    {
        return Config::IsFavorite(AppIDA);
    }

    Data::TitleInfo *TitleInfoA = Data::GetTitleInfoByID(AppIDA);
    Data::TitleInfo *TitleInfoB = Data::GetTitleInfoByID(AppIDB);
    switch (Config::GetByKey(Config::Keys::TitleSortType))
    {
        // Alpha
        case 0:
        {
            // Get titles
            const char *TitleA = TitleInfoA->GetTitle();
            const char *TitleB = TitleInfoB->GetTitle();

            // Get the shortest of the two.
            size_t TitleALength = std::char_traits<char>::length(TitleA);
            size_t TitleBLength = std::char_traits<char>::length(TitleB);
            size_t ShortestTitle = TitleALength < TitleBLength ? TitleALength : TitleBLength;
            // Loop and compare codepoints.
            for (size_t i = 0, j = 0; i < ShortestTitle;)
            {
                // Decode UTF-8
                uint32_t CodepointA = 0;
                uint32_t CodepointB = 0;
                ssize_t UnitCountA = decode_utf8(&CodepointA, reinterpret_cast<const uint8_t *>(&TitleA[i]));
                ssize_t UnitCountB = decode_utf8(&CodepointB, reinterpret_cast<const uint8_t *>(&TitleB[j]));

                // Lower so case doesn't screw with it.
                int CharA = std::tolower(CodepointA);
                int CharB = std::tolower(CodepointB);
                if (CharA != CharB)
                {
                    return CharA < CharB;
                }

                i += UnitCountA;
                j += UnitCountB;
            }
        }
        break;

        // Most played.
        case 1:
        {
            return PlayStatsA.playtime > PlayStatsB.playtime;
        }
        break;

        // Last played.
        case 2:
        {
            return PlayStatsA.last_timestamp_user > PlayStatsB.last_timestamp_user;
        }
        break;
    }
    return false;
}

Data::User::User(AccountUid AccountID) : m_AccountID(AccountID)
{
    AccountProfile Profile;
    AccountProfileBase ProfileBase = {0};

    // Whoever named these needs some help. What the hell?
    Result ProfileError = accountGetProfile(&Profile, m_AccountID);
    Result ProfileBaseError = accountProfileGet(&Profile, NULL, &ProfileBase);
    if (R_FAILED(ProfileError) || R_FAILED(ProfileBaseError))
    {
        User::CreateAccount();
    }
    else
    {
        User::LoadAccount(Profile, ProfileBase);
    }
    accountProfileClose(&Profile);
}

Data::User::User(AccountUid AccountID, std::string_view PathSafeNickname, std::string_view IconPath)
    : m_AccountID(AccountID), m_Icon(SDL::TextureManager::CreateLoadTexture(PathSafeNickname, IconPath.data()))
{
    std::memcpy(m_PathSafeNickname, PathSafeNickname.data(), PathSafeNickname.length());
}

void Data::User::AddData(const FsSaveDataInfo &SaveInfo, const PdmPlayStatistics &PlayStats)
{
    uint64_t ApplicationID = SaveInfo.application_id == 0 ? SaveInfo.system_save_data_id : SaveInfo.application_id;

    m_UserData.push_back(std::make_pair(ApplicationID, std::make_pair(SaveInfo, PlayStats)));
}

void Data::User::SortData(void)
{
    std::sort(m_UserData.begin(), m_UserData.end(), SortUserData);
}

const char *Data::User::GetNickname(void) const
{
    return m_Nickname;
}

const char *Data::User::GetPathSafeNickname(void) const
{
    return m_PathSafeNickname;
}

size_t Data::User::GetTotalDataEntries(void) const
{
    return m_UserData.size();
}

uint64_t Data::User::GetApplicationIDAt(int Index) const
{
    if (Index < 0 || Index >= static_cast<int>(m_UserData.size()))
    {
        return 0;
    }
    return m_UserData.at(Index).first;
}

FsSaveDataInfo *Data::User::GetSaveInfoAt(int Index)
{
    if (Index < 0 || Index >= static_cast<int>(m_UserData.size()))
    {
        return nullptr;
    }
    return &m_UserData.at(Index).second.first;
}

PdmPlayStatistics *Data::User::GetPlayStatsAt(int Index)
{
    if (Index < 0 || Index >= static_cast<int>(m_UserData.size()))
    {
        return nullptr;
    }
    return &m_UserData.at(Index).second.second;
}

FsSaveDataInfo *Data::User::GetSaveInfoByID(uint64_t ApplicationID)
{
    auto FindTitle = std::find_if(m_UserData.begin(), m_UserData.end(), [ApplicationID](Data::UserDataEntry &Entry) {
        return Entry.first == ApplicationID;
    });

    if (FindTitle == m_UserData.end())
    {
        return nullptr;
    }
    return &FindTitle->second.first;
}

PdmPlayStatistics *Data::User::GetPlayStatsByID(uint64_t ApplicationID)
{
    auto FindTitle = std::find_if(m_UserData.begin(), m_UserData.end(), [ApplicationID](Data::UserDataEntry &Entry) {
        return Entry.first == ApplicationID;
    });

    if (FindTitle == m_UserData.end())
    {
        return nullptr;
    }
    return &FindTitle->second.second;
}

SDL_Texture *Data::User::GetIcon(void)
{
    return m_Icon->Get();
}

SDL::SharedTexture Data::User::GetSharedIcon(void)
{
    return m_Icon;
}

void Data::User::LoadAccount(AccountProfile &Profile, AccountProfileBase &ProfileBase)
{
    // Try to load icon.
    uint32_t IconSize = 0;
    Result AccountError = accountProfileGetImageSize(&Profile, &IconSize);
    if (R_FAILED(AccountError))
    {
        Logger::Log("Error getting user icon size: 0x%X.", AccountError);
        User::CreateAccount();
        return;
    }

    std::unique_ptr<unsigned char[]> IconBuffer(new unsigned char[IconSize]);
    AccountError = accountProfileLoadImage(&Profile, IconBuffer.get(), IconSize, &IconSize);
    if (R_FAILED(AccountError))
    {
        Logger::Log("Error loading user icon: 0x%08X.", AccountError);
        User::CreateAccount();
        return;
    }

    // We should be good at this point.
    m_Icon = SDL::TextureManager::CreateLoadTexture(ProfileBase.nickname, IconBuffer.get(), IconSize);

    // Memcpy the nickname.
    std::memcpy(m_Nickname, &ProfileBase.nickname, 0x20);

    if (!StringUtil::SanitizeStringForPath(m_Nickname, m_PathSafeNickname, 0x20))
    {
        std::string AccountIDString = StringUtil::GetFormattedString("Account_%08X", m_AccountID.uid[0] & 0xFFFFFFFF);
        std::memcpy(m_PathSafeNickname, AccountIDString.c_str(), AccountIDString.length());
    }
}

void Data::User::CreateAccount(void)
{
    // This is needed a lot here.
    std::string AccountIDString = StringUtil::GetFormattedString("Acc_%08X", m_AccountID.uid[0] & 0xFFFFFFFF);

    // Create icon
    int TextX = 128 - (SDL::Text::GetWidth(ICON_FONT_SIZE, AccountIDString.c_str()) / 2);
    m_Icon = SDL::TextureManager::CreateLoadTexture(AccountIDString, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    SDL::Text::Render(m_Icon->Get(),
                      TextX,
                      128 - (ICON_FONT_SIZE / 2),
                      ICON_FONT_SIZE,
                      SDL::Text::NO_TEXT_WRAP,
                      Colors::White,
                      AccountIDString.c_str());

    // Memcpy the id string for both nicknames
    std::memcpy(m_Nickname, AccountIDString.c_str(), AccountIDString.length());
    std::memcpy(m_PathSafeNickname, AccountIDString.c_str(), AccountIDString.length());
}

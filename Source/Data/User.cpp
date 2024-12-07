#include "Data/User.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "SDL.hpp"
#include "StringUtil.hpp"
#include <cstring>

namespace
{
    constexpr int ICON_FONT_SIZE = 42;
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

Data::User::User(AccountUid AccountID, std::string_view Nickname, std::string_view PathSafeNickname) : m_AccountID(AccountID)
{
    // Memcpy nicknames. This is actually really unsafe, but I know what's going to be copied here.
    std::memcpy(m_Nickname, Nickname.data(), Nickname.length());
    std::memcpy(m_PathSafeNickname, PathSafeNickname.data(), PathSafeNickname.length());

    // Create Icon
    int TextX = 128 - (SDL::Text::GetWidth(32, m_Nickname) / 2);
    m_Icon = SDL::TextureManager::CreateLoadTexture(m_Nickname, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    SDL::Text::Render(m_Icon->Get(), TextX, 112, 32, SDL::Text::NO_TEXT_WRAP, Colors::White, m_Nickname);
}

void Data::User::AddToMap(const FsSaveDataInfo &SaveInfo, const PdmPlayStatistics &PlayStats)
{
    uint64_t ApplicationID = SaveInfo.application_id == 0 ? SaveInfo.system_save_data_id : SaveInfo.application_id;
    std::memcpy(&m_UserDataMap[ApplicationID].first, &SaveInfo, sizeof(FsSaveDataInfo));
    std::memcpy(&m_UserDataMap[ApplicationID].second, &PlayStats, sizeof(PdmPlayStatistics));
}

FsSaveDataInfo *Data::User::GetSaveInfoByID(uint64_t ApplicationID)
{
    if (m_UserDataMap.find(ApplicationID) == m_UserDataMap.end())
    {
        return nullptr;
    }
    return &m_UserDataMap.at(ApplicationID).first;
}

PdmPlayStatistics *Data::User::GetPlayStatsByID(uint64_t ApplicationID)
{
    if (m_UserDataMap.find(ApplicationID) == m_UserDataMap.end())
    {
        return nullptr;
    }
    return &m_UserDataMap.at(ApplicationID).second;
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
    std::string AccountIDString = StringUtil::GetFormattedString("Account_%08X", m_AccountID.uid[0] & 0xFFFFFFFF);

    // Create icon
    int TextX = 128 - (SDL::Text::GetWidth(32, AccountIDString.c_str()) / 2);
    m_Icon = SDL::TextureManager::CreateLoadTexture(AccountIDString, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    SDL::Text::Render(m_Icon->Get(), TextX, 112, 32, SDL::Text::NO_TEXT_WRAP, Colors::White, AccountIDString.c_str());

    // Memcpy the id string for both nicknames
    std::memcpy(m_Nickname, AccountIDString.c_str(), AccountIDString.length());
    std::memcpy(m_PathSafeNickname, AccountIDString.c_str(), AccountIDString.length());
}

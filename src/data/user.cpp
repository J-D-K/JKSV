#include <algorithm>
#include "data/data.hpp"
#include "graphics/graphics.hpp"
#include "config.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

// This is for sorting titles according to config
bool compareTitles(const data::userSaveInfo &a, const data::userSaveInfo &b)
{
    // Favorites over all
    bool aIsFavorite = config::titleIsFavorite(a.getTitleID());
    bool bIsFavorite = config::titleIsFavorite(b.getTitleID());
    if(aIsFavorite != bIsFavorite)
    {
        return aIsFavorite;
    }

    // Needed for sorting
    data::titleInfo *titleInfoA = data::getTitleInfoByTitleID(a.getTitleID());
    data::titleInfo *titleInfoB = data::getTitleInfoByTitleID(b.getTitleID());
    switch(config::getByKey(CONFIG_TITLE_SORT_TYPE))
    {
        case config::SORT_TYPE_ALPHA:
        {
            uint32_t codepointA, codepointB;
            std::string titleA = titleInfoA->getTitle();
            std::string titleB = titleInfoB->getTitle();
            int titleLength = titleA.length();
            for(int i = 0, j = 0; i < titleLength; )
            {
                ssize_t aCount = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titleA.c_str()[i]));
                ssize_t bCount = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titleB.c_str()[j]));
                codepointA = std::tolower(codepointA);
                codepointB = std::tolower(codepointB);
                if(codepointA != codepointB)
                {
                    return codepointA < codepointB;
                }
                i += aCount;
                j += bCount;
            }
        }
        break;

        case config::SORT_TYPE_MOST_PLAYED:
        {
            return a.getPlayStatistics().playtime > b.getPlayStatistics().playtime;
        }
        break;

        case config::SORT_TYPE_LAST_PLAYED:
        {
            return a.getPlayStatistics().last_timestamp_user > b.getPlayStatistics().last_timestamp_user;
        }
        break;
    }
    return false;
}

static SDL_Texture *loadUserIcon(AccountProfile *profile, AccountProfileBase *profileBase)
{
    unsigned int iconSize = 0;
    uint8_t *iconData = NULL;
    SDL_Texture *returnTexture = NULL;

    accountProfileGetImageSize(profile, &iconSize);
    iconData = new uint8_t[iconSize];
    accountProfileLoadImage(profile, iconData, iconSize, &iconSize);
    returnTexture = graphics::textureLoadFromMem(profileBase->nickname, graphics::IMG_TYPE_JPEG, iconData, iconSize);

    delete[] iconData;
    return returnTexture;
}

data::user::user(const AccountUid &accountID) : m_AccountID(accountID)
{
    AccountProfile profile;
    AccountProfileBase profileBase;

    Result profileError = accountGetProfile(&profile, accountID);
    Result profileBaseError = accountProfileGet(&profile, NULL, &profileBase);
    if(R_SUCCEEDED(profileError) && R_SUCCEEDED(profileBaseError))
    {
        m_Icon = loadUserIcon(&profile, &profileBase);
        m_Username = profileBase.nickname;
        m_PathSafeUsername = stringUtil::getPathSafeString(m_Username);
        if(m_PathSafeUsername.empty())
        {
            m_PathSafeUsername = stringUtil::getFormattedString("Acc_0x%08X", static_cast<uint32_t>(getAccountIDU128()));
        }
    }
    else
    {
        m_Icon = graphics::createIcon(m_Username, m_Username, 42);
        m_Username = stringUtil::getFormattedString("Account_0x%08X", static_cast<uint32_t>(getAccountIDU128()));
        m_PathSafeUsername = m_Username;
        logger::log("Error getting profile for user 0x%X", getAccountIDU128());
    }

    logger::log("%s @ %p", m_Username.c_str(), this);
}

data::user::user(const AccountUid &accountID, const std::string &username, const std::string &pathSafeUsername) : m_AccountID(accountID), m_Username(username), m_PathSafeUsername(pathSafeUsername)
{
    // Create icon for user
    m_Icon = graphics::createIcon(m_Username, m_Username, 42);
}

void data::user::addNewUserSaveInfo(const uint64_t &titleID, const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats)
{
    m_UserSaveInfo.emplace_back(titleID, saveInfo, playStats);
}

void data::user::sortUserSaveInfo(void)
{
    std::sort(m_UserSaveInfo.begin(), m_UserSaveInfo.end(), compareTitles);
}

void data::user::clearUserSaveInfo(void)
{
    m_UserSaveInfo.clear();
}

AccountUid data::user::getAccountID(void) const
{
    return m_AccountID;
}

u128 data::user::getAccountIDU128(void) const
{
    return ((u128)m_AccountID.uid[0] << 64 | m_AccountID.uid[1]);
}

std::string data::user::getUsername(void) const
{
    return m_Username;
}

std::string data::user::getPathSafeUsername(void) const
{
    return m_PathSafeUsername;
}

SDL_Texture *data::user::getUserIcon(void) const
{
    return m_Icon;
}

data::userSaveInfo *data::user::getUserSaveInfoAt(const int &index)
{
    if(index >= 0 && index < static_cast<int>(m_UserSaveInfo.size()))
    {
        return &m_UserSaveInfo.at(index);
    }
    return NULL;
}

int data::user::getTotalUserSaveInfo(void) const
{
    return m_UserSaveInfo.size();
}
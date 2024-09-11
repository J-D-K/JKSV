#include <algorithm>
#include <vector>

#include "data/data.hpp"

#include "graphics/graphics.hpp"
#include "config.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

namespace
{
    // Font size for creating icons for users with no icon
    const int ICON_FONT_SIZE = 42;
}

// This is for sorting titles according to config
bool compareTitles(const data::userSaveInfo &a, const data::userSaveInfo &b)
{
    // Favorites over all
    bool aIsFavorite = config::titleIsFavorite(a.getTitleID());
    bool bIsFavorite = config::titleIsFavorite(b.getTitleID());
    if (aIsFavorite != bIsFavorite)
    {
        return aIsFavorite;
    }

    // Needed for sorting
    data::titleInfo *titleInfoA = data::getTitleInfoByTitleID(a.getTitleID());
    data::titleInfo *titleInfoB = data::getTitleInfoByTitleID(b.getTitleID());
    switch (config::getByKey(CONFIG_TITLE_SORT_TYPE))
    {
        case config::SORT_TYPE_ALPHA:
        {
            uint32_t codepointA, codepointB;
            std::string titleA = titleInfoA->getTitle();
            std::string titleB = titleInfoB->getTitle();
            int titleLength = titleA.length();
            for (int i = 0, j = 0; i < titleLength;)
            {
                ssize_t aCount = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titleA.c_str()[i]));
                ssize_t bCount = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titleB.c_str()[j]));
                codepointA = std::tolower(codepointA);
                codepointB = std::tolower(codepointB);
                if (codepointA != codepointB)
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

static graphics::sdlTexture loadUserIcon(AccountProfile *profile, AccountProfileBase *profileBase)
{
    // This is the iconSize that's returned.
    unsigned int iconSize = 0;
    // This is the texture being returned
    graphics::sdlTexture returnTexture;

    // Get profile jpeg's size.
    accountProfileGetImageSize(profile, &iconSize);
    // Vector for buffer
    std::vector<uint8_t> iconData(iconSize);
    // Get icon data from system
    accountProfileLoadImage(profile, iconData.data(), iconSize, &iconSize);

    // Load it and return
    returnTexture = graphics::textureManager::createTextureFromMem(profileBase->nickname, iconData.data(), iconSize, graphics::IMAGE_TYPE_JPEG);

    return returnTexture;
}

data::user::user(AccountUid accountID, data::userType userType) : m_AccountID(accountID),
                                                                  m_UserType(userType)
{
    AccountProfile profile;
    AccountProfileBase profileBase;

    Result profileError = accountGetProfile(&profile, accountID);
    Result profileBaseError = accountProfileGet(&profile, NULL, &profileBase);
    if (R_SUCCEEDED(profileError) && R_SUCCEEDED(profileBaseError))
    {
        m_Icon = loadUserIcon(&profile, &profileBase);
        m_Username = profileBase.nickname;
        m_PathSafeUsername = stringUtil::getPathSafeString(m_Username);
        if (m_PathSafeUsername.empty())
        {
            m_PathSafeUsername = stringUtil::getFormattedString("Acc_%08X", static_cast<uint32_t>(getAccountIDU128()));
        }
    }
    else
    {
        m_Icon = graphics::createIcon(m_Username, m_Username, 42);
        m_Username = stringUtil::getFormattedString("Account_%08X", static_cast<uint32_t>(getAccountIDU128()));
        m_PathSafeUsername = m_Username;
        logger::log("Error getting profile for user %08X", getAccountIDU128());
    }
}

data::user::user(AccountUid accountID, const std::string &username, const std::string &pathSafeUsername, data::userType userType) : m_AccountID(accountID),
                                                                                                                                    m_Username(username),
                                                                                                                                    m_PathSafeUsername(pathSafeUsername),
                                                                                                                                    m_Icon(graphics::createIcon(m_Username, m_Username, 42)),
                                                                                                                                    m_UserType(userType) {}

void data::user::addNewUserSaveInfo(uint64_t titleID, const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats)
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
    return static_cast<u128>(m_AccountID.uid[0]) << 64 | m_AccountID.uid[1];
}

std::string data::user::getUsername(void) const
{
    return m_Username;
}

std::string data::user::getPathSafeUsername(void) const
{
    return m_PathSafeUsername;
}

graphics::sdlTexture data::user::getUserIcon(void) const
{
    return m_Icon;
}

data::userType data::user::getUserType(void) const
{
    return m_UserType;
}

data::userSaveInfo *data::user::getUserSaveInfoAt(int index)
{
    if (index >= 0 && index < static_cast<int>(m_UserSaveInfo.size()))
    {
        return &m_UserSaveInfo.at(index);
    }
    return NULL;
}

int data::user::getTotalUserSaveInfo(void) const
{
    return m_UserSaveInfo.size();
}
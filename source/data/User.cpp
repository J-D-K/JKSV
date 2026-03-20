#include "data/User.hpp"

#include "config/config.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "graphics/colors.hpp"
#include "graphics/gfxutil.hpp"
#include "logging/logger.hpp"
#include "sdl.hpp"
#include "stringutil.hpp"

#include <algorithm>
#include <cstring>

namespace
{
    /// @brief Font size for rendering text to icons.
    constexpr int SIZE_ICON_FONT = 50;

    /// @brief This is the number of FsSaveDataInfo entries to allocate and try to read.
    constexpr size_t SIZE_SAVE_INFO_BUFFER = 128;

    // Array of SaveDataSpaceIDs - SaveDataSpaceAll doesn't seem to work as it should...
    constexpr std::array<FsSaveDataSpaceId, 6> SAVE_DATA_SPACE_ORDER = {FsSaveDataSpaceId_System,
                                                                        FsSaveDataSpaceId_User,
                                                                        FsSaveDataSpaceId_SdSystem,
                                                                        FsSaveDataSpaceId_Temporary,
                                                                        FsSaveDataSpaceId_SdUser,
                                                                        FsSaveDataSpaceId_SafeMode};
} // namespace

// Function used to sort user data. Definition at the bottom.
static bool sort_user_data(const data::UserDataEntry &entryA, const data::UserDataEntry &entryB);

//                      ---- Construction ----

data::User::User(AccountUid accountID, FsSaveDataType saveType) noexcept
    : m_accountID(accountID)
    , m_saveType(saveType)
{
    AccountProfile profile{};
    AccountProfileBase profileBase{};

    const bool profileError = error::libnx(accountGetProfile(&profile, m_accountID));
    const bool baseError    = !profileError && error::libnx(accountProfileGet(&profile, nullptr, &profileBase));
    if (profileError || baseError) { User::create_account(); }
    else
    {
        User::load_account(profile, profileBase);
    }
    accountProfileClose(&profile);
}

data::User::User(AccountUid accountID,
                 std::string_view nickname,
                 std::string_view pathSafeNickname,
                 FsSaveDataType saveType) noexcept
    : m_accountID(accountID)
    , m_saveType(saveType)
{
    std::strncpy(m_nickname, nickname.data(), nickname.length());
    std::strncpy(m_pathSafeNickname, pathSafeNickname.data(), pathSafeNickname.length());
}

//                      ---- Move constructor & operator ----

data::User::User(data::User &&user) noexcept { *this = std::move(user); }

data::User &data::User::operator=(data::User &&user) noexcept
{
    static constexpr size_t SIZE_NICKNAME = 0x20;

    m_accountID = user.m_accountID;
    m_saveType  = user.m_saveType;
    std::strncpy(m_nickname, user.m_nickname, SIZE_NICKNAME);
    std::strncpy(m_pathSafeNickname, user.m_pathSafeNickname, SIZE_NICKNAME);
    m_icon     = user.m_icon;
    m_userData = std::move(user.m_userData);

    user.m_accountID = {0};
    user.m_saveType  = static_cast<FsSaveDataType>(0);
    user.m_icon      = nullptr;

    return *this;
}

//                      ---- Public functions ----

void data::User::add_data(uint64_t applicationID, const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats)
{
    auto dataPair   = std::make_pair(saveInfo, playStats);
    auto vectorPair = std::make_pair(applicationID, std::move(dataPair));
    m_userData.push_back(std::move(vectorPair));
}

void data::User::clear_data_entries() noexcept { m_userData.clear(); }

void data::User::erase_data(int index) { m_userData.erase(m_userData.begin() + index); }

void data::User::sort_data() noexcept { std::sort(m_userData.begin(), m_userData.end(), sort_user_data); }

AccountUid data::User::get_account_id() const noexcept { return m_accountID; }

FsSaveDataType data::User::get_account_save_type() const noexcept { return m_saveType; }

const char *data::User::get_nickname() const noexcept { return m_nickname; }

const char *data::User::get_path_safe_nickname() const noexcept { return m_pathSafeNickname; }

size_t data::User::get_total_data_entries() const noexcept { return m_userData.size(); }

uint64_t data::User::get_application_id_at(int index) const noexcept
{
    if (!User::index_check(index)) { return 0; }
    return m_userData.at(index).first;
}

FsSaveDataInfo *data::User::get_save_info_at(int index) noexcept
{
    if (!User::index_check(index)) { return nullptr; }
    return &m_userData.at(index).second.first;
}

PdmPlayStatistics *data::User::get_play_stats_at(int index) noexcept
{
    if (!User::index_check(index)) { return nullptr; }
    return &m_userData.at(index).second.second;
}

FsSaveDataInfo *data::User::get_save_info_by_id(uint64_t applicationID) noexcept
{
    auto target = User::find_title_by_id(applicationID);
    if (target == m_userData.end()) { return nullptr; }
    return &target->second.first;
}

data::UserSaveInfoList &data::User::get_user_save_info_list() noexcept { return m_userData; }

PdmPlayStatistics *data::User::get_play_stats_by_id(uint64_t applicationID) noexcept
{
    auto target = User::find_title_by_id(applicationID);
    if (target == m_userData.end()) { return nullptr; }
    return &target->second.second;
}

void data::User::erase_save_info(const FsSaveDataInfo *saveInfo)
{
    auto find_save_info = [&](const data::UserDataEntry &entry) { return &entry.second.first == saveInfo; };

    auto findInfo = std::find_if(m_userData.begin(), m_userData.end(), find_save_info);
    if (findInfo == m_userData.end()) { return; }

    m_userData.erase(findInfo);
}

void data::User::erase_save_info_by_id(uint64_t applicationID)
{
    auto target = User::find_title_by_id(applicationID);
    if (target == m_userData.end()) { return; }
    m_userData.erase(target);
}

//                      ---- Private functions ----

void data::User::load_user_data()
{
    if (!m_userData.empty()) { m_userData.clear(); }
    const bool accountSys    = config::get_by_key(config::keys::LIST_ACCOUNT_SYS_SAVES);
    const bool enforceMount  = config::get_by_key(config::keys::ONLY_LIST_MOUNTABLE);
    const bool isAccountUser = m_saveType != FsSaveDataType_System && m_saveType != FsSaveDataType_SystemBcat;

    for (int i = 0; i < 6; i++)
    {
        fslib::SaveInfoReader infoReader{};
        if (m_saveType == FsSaveDataType_Account)
        {
            infoReader.open(SAVE_DATA_SPACE_ORDER[i], m_accountID, SIZE_SAVE_INFO_BUFFER);
        }
        else
        {
            infoReader.open(SAVE_DATA_SPACE_ORDER[i], m_saveType, SIZE_SAVE_INFO_BUFFER);
        }
        if (!infoReader.is_open()) { continue; }

        while (infoReader.read())
        {
            for (const FsSaveDataInfo &saveInfo : infoReader)
            {
                const uint64_t saveInfoAppID = saveInfo.application_id;
                const uint64_t saveInfoSysID = saveInfo.system_save_data_id;
                const uint64_t applicationID = saveInfoAppID != 0 ? saveInfoAppID : saveInfoSysID;

                const uint8_t saveDataType = saveInfo.save_data_type;
                const bool isSystemSave    = saveDataType == FsSaveDataType_System || saveDataType == FsSaveDataType_SystemBcat;

                const bool isBlacklisted = config::is_blacklisted(applicationID);
                const bool systemFilter  = (!accountSys && isAccountUser && isSystemSave);

                bool mounted{};
                if (!isBlacklisted && !systemFilter && enforceMount)
                {
                    fs::ScopedSaveMount saveMount{fs::DEFAULT_SAVE_MOUNT, &saveInfo};
                    mounted = saveMount.is_open();
                }

                if (isBlacklisted || systemFilter || (enforceMount && !mounted)) { continue; }

                const bool titleFound = data::title_exists_in_map(applicationID);
                if (!titleFound) { data::load_title_to_map(applicationID); }

                // I don't really care about this failing.
                PdmPlayStatistics playStats{};
                pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(saveInfo.application_id,
                                                                         m_accountID,
                                                                         false,
                                                                         &playStats);

                User::add_data(applicationID, saveInfo, playStats);
            }
        }
    }

    User::sort_data();
}

void data::User::load_icon(sdl2::Renderer &renderer)
{
    const std::string iconName =
        stringutil::get_formatted_string("%016llX%016llX", m_nickname, m_accountID.uid[0], m_accountID.uid[1]);
    if (m_saveType == FsSaveDataType_Account)
    {
        uint32_t iconSize{};
        AccountProfile profile{};
        const bool profileError = error::libnx(accountGetProfile(&profile, m_accountID));
        const bool sizeError    = !profileError && error::libnx(accountProfileGetImageSize(&profile, &iconSize));
        if (profileError || sizeError)
        {
            m_icon = gfxutil::create_generic_icon(renderer, m_nickname, SIZE_ICON_FONT, colors::DIALOG_DARK, colors::WHITE);
            return;
        }

        auto iconBuffer      = std::make_unique<char[]>(iconSize);
        const bool loadError = error::libnx(accountProfileLoadImage(&profile, iconBuffer.get(), iconSize, &iconSize));
        if (loadError) { return; }

        accountProfileClose(&profile);
        m_icon = sdl2::TextureManager::create_load_resource(iconName, iconBuffer.get(), iconSize);
    }
    else
    {
        m_icon = gfxutil::create_generic_icon(renderer, m_nickname, SIZE_ICON_FONT, colors::DIALOG_DARK, colors::WHITE);
    }
}

void data::User::load_account(AccountProfile &profile, AccountProfileBase &profileBase)
{
    static constexpr size_t NICKNAME_BUFFER = 0x20;

    std::strncpy(m_nickname, profileBase.nickname, NICKNAME_BUFFER);

    const bool sanitizeError = !stringutil::sanitize_string_for_path(m_nickname, m_pathSafeNickname, NICKNAME_BUFFER);
    if (sanitizeError)
    {
        const std::string idString = stringutil::get_formatted_string("Acc_%04X", m_accountID.uid[0] & 0xFFFF);
        std::memcpy(m_pathSafeNickname, idString.c_str(), idString.length());
    }
}

void data::User::create_account()
{
    const std::string idString = stringutil::get_formatted_string("Acc_%04X", m_accountID.uid[0] & 0xFFFF);
    std::memcpy(m_nickname, idString.c_str(), idString.length());
    std::memcpy(m_pathSafeNickname, idString.c_str(), idString.length());
}

data::UserSaveInfoList::iterator data::User::find_title_by_id(uint64_t applicationID)
{
    return std::find_if(m_userData.begin(), m_userData.end(), [&](const auto &entry) { return entry.first == applicationID; });
}

//                      ---- Static functions ----

static bool sort_user_data(const data::UserDataEntry &entryA, const data::UserDataEntry &entryB)
{
    // Structured bindings to make this slightly more readable.
    auto &[applicationIDA, dataA] = entryA;
    auto &[applicationIDB, dataB] = entryB;
    auto &[saveInfoA, playStatsA] = dataA;
    auto &[saveInfoB, playStatsB] = dataB;

    // Favorites over all.
    if (config::is_favorite(applicationIDA) != config::is_favorite(applicationIDB))
    {
        return config::is_favorite(applicationIDA);
    }

    data::TitleInfo *titleInfoA = data::get_title_info_by_id(applicationIDA);
    data::TitleInfo *titleInfoB = data::get_title_info_by_id(applicationIDB);
    switch (config::get_by_key(config::keys::TITLE_SORT_TYPE))
    {
        // Alpha
        case 0:
        {
            // Get titles
            const char *titleA = titleInfoA->get_title();
            const char *titleB = titleInfoB->get_title();

            // Get the shortest of the two.
            size_t titleALength  = std::char_traits<char>::length(titleA);
            size_t titleBLength  = std::char_traits<char>::length(titleB);
            size_t shortestTitle = titleALength < titleBLength ? titleALength : titleBLength;
            // Loop and compare codepoints.
            for (size_t i = 0, j = 0; i < shortestTitle;)
            {
                // Decode UTF-8
                uint32_t codepointA = 0;
                uint32_t codepointB = 0;
                ssize_t unitCountA  = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titleA[i]));
                ssize_t unitCountB  = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titleB[j]));

                // Lower so case doesn't screw with it.
                int charA = std::tolower(codepointA);
                int charB = std::tolower(codepointB);
                if (charA != charB) { return charA < charB; }

                i += unitCountA;
                j += unitCountB;
            }
        }
        break;

        // Most played.
        case 1: return playStatsA.playtime > playStatsB.playtime; break;

        // Last played.
        case 2: return playStatsA.last_timestamp_user > playStatsB.last_timestamp_user; break;
    }
    return false;
}

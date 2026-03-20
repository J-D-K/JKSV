#include "data/DataContext.hpp"

#include "config/config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

namespace
{
    /// @brief This is the path to the cache file.
    constexpr std::string_view PATH_CACHE_FILE = "sdmc:/config/JKSV/cache.zip";

    /// @brief This is used in multiple places.
    constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);
}

//                      ---- Public functions ----

bool data::DataContext::load_create_users(sys::Task *task)
{
    static constexpr int32_t MAX_SWITCH_ACCOUNTS = 8;

    static constexpr AccountUid ID_SYSTEM_USER = {FsSaveDataType_System};
    static constexpr AccountUid ID_BCAT_USER   = {FsSaveDataType_Bcat};
    static constexpr AccountUid ID_DEVICE_USER = {FsSaveDataType_Device};
    static constexpr AccountUid ID_CACHE_USER  = {FsSaveDataType_Cache};

    static constexpr const char *systemSafe = "System";
    static constexpr const char *bcatSafe   = "BCAT";
    static constexpr const char *deviceSafe = "Device";
    static constexpr const char *cacheSafe  = "Cache";

    if (error::is_null(task) || !m_users.empty()) { return false; }

    const bool emplaceDevice = config::get_by_key(config::keys::SHOW_DEVICE_USER);
    const bool emplaceBCAT   = config::get_by_key(config::keys::SHOW_BCAT_USER);
    const bool emplaceCache  = config::get_by_key(config::keys::SHOW_CACHE_USER);
    const bool emplaceSystem = config::get_by_key(config::keys::SHOW_SYSTEM_USER);

    const char *statusLoading  = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 0);
    const char *statusCreating = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 1);
    const char *systemName     = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 0);
    const char *bcatName       = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 2);
    const char *deviceName     = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 3);
    const char *cacheName      = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 5);
    task->set_status(statusLoading);

    int total{};
    AccountUid accounts[8]{};
    const bool accountError = error::libnx(accountListAllUsers(accounts, MAX_SWITCH_ACCOUNTS, &total));
    const bool noAccounts   = total <= 0;
    if (accountError || noAccounts) { return false; }

    {
        std::lock_guard userGuard{m_userMutex};
        for (int i = 0; i < total; i++) { m_users.emplace_back(accounts[i], FsSaveDataType_Account); }

        task->set_status(statusCreating);
        if (emplaceDevice) { m_users.emplace_back(ID_DEVICE_USER, deviceName, deviceSafe, FsSaveDataType_Device); }
        if (emplaceBCAT) { m_users.emplace_back(ID_BCAT_USER, bcatName, bcatSafe, FsSaveDataType_Bcat); }
        if (emplaceCache) { m_users.emplace_back(ID_CACHE_USER, cacheName, cacheSafe, FsSaveDataType_Cache); }
        if (emplaceSystem) { m_users.emplace_back(ID_SYSTEM_USER, systemName, systemSafe, FsSaveDataType_System); }
    }

    std::lock_guard iconGuard{m_iconQueueMutex};
    for (data::User &user : m_users) { m_iconQueue.push_back(&user); }

    return true;
}

void data::DataContext::load_user_save_info(sys::Task *task)
{
    if (error::is_null(task)) { return; }

    const char *statusLoadingUserInfo = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 5);

    for (data::User &user : m_users)
    {
        std::lock_guard userGuard{m_userMutex};
        {
            const char *nickname = user.get_nickname();
            std::string status   = stringutil::get_formatted_string(statusLoadingUserInfo, nickname);
            task->set_status(status);
        }
        user.load_user_data();
    }
}

void data::DataContext::get_users(data::UserList &listOut)
{
    std::lock_guard userGuard{m_userMutex};
    for (data::User &user : m_users) { listOut.push_back(&user); }
}

void data::DataContext::load_application_records(sys::Task *task)
{
    if (error::is_null(task)) { return; }

    const char *statusLoadingRecords = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 2);

    int offset{}, count{};
    NsApplicationRecord record{};

    bool listError{};
    do
    {
        listError = error::libnx(nsListApplicationRecord(&record, 1, offset++, &count)) || count <= 0;
        if (listError) { break; }
        if (DataContext::title_is_loaded(record.application_id)) { continue; }

        {
            std::string status = stringutil::get_formatted_string(statusLoadingRecords, record.application_id);
            task->set_status(status);
        }
        DataContext::load_title(record.application_id);
    } while (!listError);
}

bool data::DataContext::title_is_loaded(uint64_t applicationID) noexcept
{
    const bool isSystem = applicationID & 0x8000000000000000;

    std::lock_guard titleGuard{m_titleMutex};
    const bool loaded = m_titleInfo.find(applicationID) != m_titleInfo.end();
    if (!isSystem && !loaded) { m_cacheIsValid = false; }
    return loaded;
}

void data::DataContext::load_title(uint64_t applicationID)
{
    std::scoped_lock titleGuard{m_titleMutex, m_iconQueueMutex};
    m_titleInfo.try_emplace(applicationID, applicationID);
    m_iconQueue.push_back(&m_titleInfo.at(applicationID));
}

data::TitleInfo *data::DataContext::get_title_by_id(uint64_t applicationID) noexcept
{
    std::lock_guard titleGuard{m_titleMutex};
    auto findTitle = m_titleInfo.find(applicationID);
    if (findTitle == m_titleInfo.end()) { return nullptr; }
    return &findTitle->second;
}

void data::DataContext::get_title_info_list(data::TitleInfoList &listOut)
{
    std::lock_guard titleGuard{m_titleMutex};
    for (auto &[applicationID, titleInfo] : m_titleInfo) { listOut.push_back(&titleInfo); }
}

void data::DataContext::get_title_info_list_by_type(FsSaveDataType type, data::TitleInfoList &listOut)
{
    std::lock_guard titleGuard{m_titleMutex};
    for (auto &[application, titleInfo] : m_titleInfo)
    {
        if (titleInfo.has_save_data_type(type)) { listOut.push_back(&titleInfo); }
    }
}

void data::DataContext::import_svi_files(sys::Task *task)
{
    static constexpr size_t SIZE_UINT32 = sizeof(uint32_t);
    static constexpr size_t SIZE_UINT64 = sizeof(uint64_t);
    static constexpr size_t SIZE_SVI    = SIZE_UINT32 + SIZE_UINT64 + SIZE_CTRL_DATA;

    if (error::is_null(task)) { return; }

    const char *statusLoadingSvi = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 3);

    const fslib::Path sviPath{config::get_working_directory() / "svi"};
    fslib::Directory sviDir{sviPath};
    if (error::fslib(sviDir.is_open())) { return; }

    task->set_status(statusLoadingSvi);

    // auto controlData = std::make_unique<NsApplicationControlData>();
    NsApplicationControlData controlData{};
    for (const fslib::DirectoryEntry &entry : sviDir)
    {
        const fslib::Path target{sviPath / entry};
        fslib::File sviFile{target, FsOpenMode_Read};
        const bool validSvi = sviFile.is_open() && sviFile.get_size() == SIZE_SVI;
        if (!validSvi) { continue; }

        uint32_t magic{};
        uint64_t applicationID{};
        const bool magicRead = sviFile.read(&magic, SIZE_UINT32) == SIZE_UINT32;
        const bool idRead    = sviFile.read(&applicationID, SIZE_UINT64) == SIZE_UINT64;
        const bool exists    = DataContext::title_is_loaded(applicationID);
        if (!magicRead || magic != fs::SAVE_META_MAGIC || !idRead || exists) { continue; }

        const bool dataRead = sviFile.read(&controlData, SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!dataRead) { continue; }

        std::scoped_lock multiGuard{m_iconQueueMutex, m_titleMutex};
        m_titleInfo.try_emplace(applicationID, applicationID, controlData);
        m_iconQueue.push_back(&m_titleInfo.at(applicationID));
    }
}

void data::DataContext::delete_cache()
{
    const fslib::Path cachePath{PATH_CACHE_FILE};
    const bool cacheExists = fslib::file_exists(cachePath);
    if (cacheExists) { fslib::delete_file(cachePath); };
}

bool data::DataContext::read_cache(sys::Task *task)
{
    if (error::is_null(task)) { return false; }

    m_cacheIsValid = false;
    fs::MiniUnzip cacheZip{PATH_CACHE_FILE};
    if (!cacheZip.is_open()) { return false; }

    const char *statusLoadingCache = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 4);
    task->set_status(statusLoadingCache);

    NsApplicationControlData controlData{};
    do
    {
        const bool dataRead = cacheZip.read(&controlData, SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!dataRead) { continue; }

        std::string_view filename{cacheZip.get_filename()};
        const size_t nameBegin = filename.find_first_not_of('/');
        if (nameBegin == filename.npos) { continue; }

        filename                     = filename.substr(nameBegin);
        const uint64_t applicationID = std::strtoull(filename.data(), nullptr, 16);

        std::scoped_lock multiGuard{m_iconQueueMutex, m_titleMutex};

        m_titleInfo.try_emplace(applicationID, applicationID, controlData);
        m_iconQueue.push_back(&m_titleInfo.at(applicationID));
    } while (cacheZip.next_file());
    m_cacheIsValid = true;
    return true;
}

bool data::DataContext::write_cache(sys::Task *task)
{
    if (error::is_null(task)) { return false; }

    const fslib::Path cachePath{PATH_CACHE_FILE};
    const bool cacheExists = fslib::file_exists(cachePath);
    if (cacheExists && m_cacheIsValid) { return true; }

    fs::MiniZip cacheZip{cachePath};
    if (!cacheZip.is_open()) { return false; }

    const char *statusWritingCache = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 7);
    task->set_status(statusWritingCache);

    std::lock_guard titleGuard{m_titleMutex};
    for (auto &[applicationID, titleInfo] : m_titleInfo)
    {
        if (!titleInfo.has_control_data()) { continue; }

        const NsApplicationControlData *controlData = titleInfo.get_control_data();
        const std::string cacheName                 = stringutil::get_formatted_string("//%016llX", applicationID);
        const bool opened                           = cacheZip.open_new_file(cacheName);
        if (!opened) { continue; }

        const bool controlWritten = cacheZip.write(controlData, SIZE_CTRL_DATA);
        if (!controlWritten) { logger::log("Error writing control data to cache!"); }
        cacheZip.close_current_file();
    }
    m_cacheIsValid = true;

    return true;
}

void data::DataContext::process_icon_queue(sdl2::Renderer &renderer)
{
    std::lock_guard multiGuard{m_iconQueueMutex};
    for (data::DataCommon *common : m_iconQueue) { common->load_icon(renderer); }
    m_iconQueue.clear();
}

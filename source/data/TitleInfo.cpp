#include "data/TitleInfo.hpp"

#include "config/config.hpp"
#include "error.hpp"
#include "graphics/colors.hpp"
#include "graphics/gfxutil.hpp"
#include "logging/logger.hpp"
#include "stringutil.hpp"

#include <cstring>
#include <memory>
#include <zlib.h>

namespace
{
    /// @brief Offset of the TitlesDataFormat byte within the NACP. [21.0.0+]
    /// @note Addressed by raw offset so this works against libnx versions that predate the field.
    constexpr size_t OFFSET_TITLES_DATA_FORMAT = 0x3215;

    /// @brief Value of TitlesDataFormat meaning the language entry block is raw-deflate compressed.
    constexpr uint8_t TITLES_DATA_FORMAT_COMPRESSED = 0x01;

    /// @brief Size of the language entry block that fits in NacpStruct (16 entries).
    constexpr size_t SIZE_LANGUAGE_BLOCK = 0x3000;

    /// @brief Size of the decompressed language entry block (32 entries).
    constexpr size_t SIZE_LANGUAGE_BLOCK_FULL = 0x6000;

    /// @brief Decompresses the NACP language entry block in place if it's in the [21.0.0+] compressed format.
    /// @param controlData Control data to fix up.
    /// @return True if the data is usable afterwards, false if decompression was needed but failed.
    /// @note Firmware 21.0.0 added a compressed layout: when TitlesDataFormat is 1, the u16 at NACP+0x0 is the
    /// compressed size and a raw deflate stream follows at NACP+0x2, inflating to NacpLanguageEntry[32]. libnx does
    /// not decompress it, so nacpGetLanguageEntry would otherwise hand back compressed bytes as the title string.
    bool decompress_language_entries(NsApplicationControlData &controlData) noexcept
    {
        uint8_t *nacpBytes        = reinterpret_cast<uint8_t *>(&controlData.nacp);
        const uint8_t titlesFormat = nacpBytes[OFFSET_TITLES_DATA_FORMAT];

        // 0 is the classic, uncompressed layout. Nothing to do.
        if (titlesFormat == 0x00) { return true; }

        // Anything other than 1 is a format we don't know how to read.
        if (titlesFormat != TITLES_DATA_FORMAT_COMPRESSED) { return false; }

        uint16_t compressedSize{};
        std::memcpy(&compressedSize, nacpBytes, sizeof(compressedSize));
        if (compressedSize == 0 || compressedSize > SIZE_LANGUAGE_BLOCK - sizeof(compressedSize)) { return false; }

        // The full block is twice what NacpStruct holds, so it needs to land somewhere else first.
        std::unique_ptr<uint8_t[]> decompressed = std::make_unique<uint8_t[]>(SIZE_LANGUAGE_BLOCK_FULL);
        if (!decompressed) { return false; }

        z_stream stream{};
        // Negative window bits: raw deflate stream, no zlib or gzip header.
        if (inflateInit2(&stream, -15) != Z_OK) { return false; }

        stream.next_in   = nacpBytes + sizeof(compressedSize);
        stream.avail_in  = compressedSize;
        stream.next_out  = decompressed.get();
        stream.avail_out = SIZE_LANGUAGE_BLOCK_FULL;

        const int inflated       = inflate(&stream, Z_FINISH);
        const size_t inflatedSize = stream.total_out;
        inflateEnd(&stream);

        // A stream that exactly fills the output buffer reports Z_BUF_ERROR instead of Z_STREAM_END, because the
        // end-of-stream marker can't be consumed with no room left. Having the whole block is what actually matters.
        if (inflated != Z_STREAM_END && inflatedSize != SIZE_LANGUAGE_BLOCK_FULL) { return false; }

        // A short read would leave the tail of the block as uninitialized garbage.
        if (inflatedSize < SIZE_LANGUAGE_BLOCK) { return false; }

        // Only the first 16 entries fit in NacpStruct. The rest are languages libnx has no index for anyway.
        std::memcpy(&controlData.nacp, decompressed.get(), SIZE_LANGUAGE_BLOCK);

        // Mark it as the classic layout now that it actually is one.
        nacpBytes[OFFSET_TITLES_DATA_FORMAT] = 0x00;

        return true;
    }
} // namespace

//                      ---- Construction ----

data::TitleInfo::TitleInfo(uint64_t applicationID) noexcept
    : m_applicationID(applicationID)
{
    static constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);

    uint64_t controlSize{};

    // This will filter from even trying to fetch control data for system titles.
    const bool isSystem   = applicationID & 0x8000000000000000;
    const bool getError   = !isSystem && error::libnx(nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                                                m_applicationID,
                                                                                &m_data,
                                                                                SIZE_CTRL_DATA,
                                                                                &controlSize));
    const bool nacpError  = !getError && !decompress_language_entries(m_data);
    const bool entryError = !getError && !nacpError && error::libnx(nacpGetLanguageEntry(&m_data.nacp, &m_entry));
    if (isSystem || getError || nacpError)
    {
        const std::string appIDHex = stringutil::get_formatted_string("%04X", m_applicationID & 0xFFFF);
        m_entry                    = &m_data.nacp.lang[SetLanguage_ENUS]; // I'm hoping this is enough?

        std::snprintf(m_entry->name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
        TitleInfo::get_create_path_safe_title();
    }
    else if (!getError && !nacpError && !entryError)
    {
        m_hasData = true;
        TitleInfo::get_create_path_safe_title();
    }
}

// To do: Make this safer...
data::TitleInfo::TitleInfo(uint64_t applicationID, NsApplicationControlData &controlData) noexcept
    : m_applicationID(applicationID)
    , m_data(controlData)
    , m_hasData(true)
{
    const bool nacpError  = !decompress_language_entries(m_data);
    const bool entryError = nacpError || error::libnx(nacpGetLanguageEntry(&m_data.nacp, &m_entry));
    if (entryError)
    {
        m_entry = &m_data.nacp.lang[SetLanguage_ENUS];
        std::snprintf(m_entry->name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
    }

    TitleInfo::get_create_path_safe_title();
}

//                      ---- Public functions ----

uint64_t data::TitleInfo::get_application_id() const noexcept { return m_applicationID; }

const NsApplicationControlData *data::TitleInfo::get_control_data() const noexcept { return &m_data; }

bool data::TitleInfo::has_control_data() const noexcept { return m_hasData; }

const char *data::TitleInfo::get_title() const noexcept { return m_entry->name; }

const char *data::TitleInfo::get_path_safe_title() const noexcept { return m_pathSafeTitle; }

const char *data::TitleInfo::get_publisher() const noexcept { return m_entry->author; }

uint64_t data::TitleInfo::get_save_data_owner_id() const noexcept { return m_data.nacp.save_data_owner_id; }

int64_t data::TitleInfo::get_save_data_size(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:   return nacp.user_account_save_data_size;
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return nacp.device_save_data_size;
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:     return nacp.cache_storage_size;
    }

    return 0;
}

int64_t data::TitleInfo::get_save_data_size_max(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:   return std::max(nacp.user_account_save_data_size, nacp.user_account_save_data_size_max);
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return std::max(nacp.device_save_data_size, nacp.device_save_data_size_max);
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:     return std::max(nacp.cache_storage_size, nacp.cache_storage_data_and_journal_size_max);
    }

    return 0;
}

int64_t data::TitleInfo::get_journal_size(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:   return nacp.user_account_save_data_journal_size;
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return nacp.device_save_data_journal_size;
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:     return nacp.cache_storage_journal_size;
    }

    return 0;
}

int64_t data::TitleInfo::get_journal_size_max(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:
            return std::max(nacp.user_account_save_data_journal_size, nacp.user_account_save_data_journal_size_max);
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return std::max(nacp.device_save_data_journal_size, nacp.device_save_data_journal_size_max);
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:
            return std::max(nacp.cache_storage_journal_size, nacp.cache_storage_data_and_journal_size_max);
    }

    return 0;
}

bool data::TitleInfo::has_save_data_type(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account: return nacp.user_account_save_data_size > 0 || nacp.user_account_save_data_size_max > 0;
        case FsSaveDataType_Bcat:    return nacp.bcat_delivery_cache_storage_size > 0;
        case FsSaveDataType_Device:  return nacp.device_save_data_size > 0 || nacp.device_save_data_size_max > 0;
        case FsSaveDataType_Cache:   return nacp.cache_storage_size > 0 || nacp.cache_storage_data_and_journal_size_max > 0;
    }

    return false;
}

sdl::SharedTexture data::TitleInfo::get_icon() const noexcept { return m_icon; }

void data::TitleInfo::set_path_safe_title(const char *newPathSafe) noexcept
{
    const size_t length = std::char_traits<char>::length(newPathSafe);
    if (length >= TitleInfo::SIZE_PATH_SAFE) { return; }

    std::memset(m_pathSafeTitle, 0x00, TitleInfo::SIZE_PATH_SAFE);
    std::memcpy(m_pathSafeTitle, newPathSafe, length);
}

void data::TitleInfo::load_icon()
{
    // This is taken from the NacpStruct.
    static constexpr size_t SIZE_ICON = 0x20000;

    if (m_hasData)
    {
        const std::string textureName = stringutil::get_formatted_string("%016llX", m_applicationID);
        m_icon                        = sdl::TextureManager::load(textureName, m_data.icon, SIZE_ICON);
    }
    else
    {
        const std::string text = stringutil::get_formatted_string("%04X", m_applicationID & 0xFFFF);
        m_icon                 = gfxutil::create_generic_icon(text, 48, colors::DIALOG_DARK, colors::WHITE);
    }
}

//                      ---- Private functions ----

void data::TitleInfo::get_create_path_safe_title() noexcept
{
    // Check if the title has a custom output path instead.
    const bool hasCustom = config::has_custom_path(m_applicationID);
    if (hasCustom)
    {
        // Read it into our local buffer.
        config::get_custom_path(m_applicationID, m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE);
        return;
    }

    // This is whether or not to use the title ID. It's used to bypass the English check.
    const bool useTitleId = config::get_by_key(config::keys::USE_TITLE_IDS);
    const bool useEnglish = config::get_by_key(config::keys::ENGLISH_SAFE_TITLES);

    // Grab the English title in case it's needed.
    const char *englishTitle{};
    const bool hasEnglish = TitleInfo::get_english_title(&englishTitle);

    // Final condition for override.
    const bool englishSafeTitle = useEnglish && hasEnglish;

    // This is our final string to use.
    const char *safeTarget = englishSafeTitle ? englishTitle : m_entry->name;

    const bool sanitized = !useTitleId && stringutil::sanitize_string_for_path(safeTarget, m_pathSafeTitle, SIZE_PATH_SAFE);
    if (useTitleId || !sanitized) { std::snprintf(m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID); }
}

bool data::TitleInfo::get_english_title(const char **titleOut) const noexcept
{
    // These are the indexes for the english titles.
    static constexpr uint8_t ENUS_INDEX = 0;
    static constexpr uint8_t ENGB_INDEX = 1;

    // Grab the pointers to them.
    const char *enUs = m_data.nacp.lang[ENUS_INDEX].name;
    const char *enGb = m_data.nacp.lang[ENGB_INDEX].name;

    // If they're both empty, return false.
    if (enUs[0] == 0x00 && enGb[0] == 0x00) { return false; }

    // Get the length. Assign whichever is longer.
    const size_t enUsLength = std::char_traits<char>::length(enUs);
    const size_t enGbLength = std::char_traits<char>::length(enGb);

    // Assign whichever is longer. EnUS wins on match.
    *titleOut = enUsLength >= enGbLength ? enUs : enGb;

    return true;
}

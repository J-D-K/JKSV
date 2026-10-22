#pragma once
#include "data/TitleInfo.hpp"
#include "fslib.hpp"

#include <cstdint>
#include <switch.h>

namespace fs
{
    // clang-format off
    struct SaveMetaData
    {
        uint32_t    magic{};
        uint8_t     revision{};
        uint64_t    applicationID{};
        AccountUid  accountID{};
        uint64_t    systemSaveID{};
        uint8_t     saveDataType{};
        uint8_t     saveDataRank{};
        uint16_t    saveDataIndex{};
        uint64_t    ownerID{};
        uint64_t    timestamp{};
        uint32_t    flags{};
        int64_t     saveDataSize{};
        int64_t     journalSize{};
        uint64_t    commitID{};
        uint8_t     saveDataSpaceID{};
    } __attribute__((packed));
    // clang-format on

    /// @brief This is the magic value written to the beginning.
    constexpr uint32_t SAVE_META_MAGIC = 0x56534B4A;

    /// @brief This is the filename used for the save data meta info.
    inline constexpr std::string_view NAME_SAVE_META = ".nx_save_meta.bin";

    inline constexpr ssize_t SIZE_SAVE_META = sizeof(fs::SaveMetaData);

    /// @brief Reads the save metadata from a backup folder or ZIP.
    /// @param path Path to the backup folder or ZIP.
    /// @param metaOut Struct to write the metadata to.
    /// @return True if valid metadata was read. False otherwise.
    bool read_save_meta(const fslib::Path &path, SaveMetaData &metaOut) noexcept;

    /// @brief Didn't feel like a whole new file just for this. Fills an fs::SaveMetaData struct.
    bool fill_save_meta_data(const FsSaveDataInfo *saveInfo, SaveMetaData &meta) noexcept;

    /// @brief Processes the save meta data and applies it to the passed saveInfo pointer.
    /// @param saveInfo FsSaveDataInfo to apply the meta to.
    /// @param meta Save meta data to apply.
    bool process_save_meta_data(const FsSaveDataInfo *saveInfo, const SaveMetaData &meta) noexcept;
} // namespace fs

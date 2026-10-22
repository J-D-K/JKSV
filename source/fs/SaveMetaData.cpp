#include "fs/SaveMetaData.hpp"

#include "error.hpp"
#include "fs/MiniUnzip.hpp"
#include "fs/directory_functions.hpp"
#include "fs/save_data_functions.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"
#include "logging/logger.hpp"

namespace
{
    constexpr size_t SIZE_EXTRA_DATA = sizeof(FsSaveDataExtraData);
}

bool fs::fill_save_meta_data(const FsSaveDataInfo *saveInfo, fs::SaveMetaData &meta) noexcept
{
    FsSaveDataExtraData extraData{};
    const bool extraRead = fs::read_save_extra_data(saveInfo, extraData);
    if (!extraRead) { return false; }

    meta = {.magic           = fs::SAVE_META_MAGIC,
            .revision        = 0x01,
            .applicationID   = extraData.attr.application_id,
            .accountID       = extraData.attr.uid,
            .systemSaveID    = extraData.attr.system_save_data_id,
            .saveDataType    = extraData.attr.save_data_type,
            .saveDataRank    = extraData.attr.save_data_rank,
            .saveDataIndex   = extraData.attr.save_data_index,
            .ownerID         = extraData.owner_id,
            .timestamp       = extraData.timestamp,
            .flags           = extraData.flags,
            .saveDataSize    = extraData.data_size,
            .journalSize     = extraData.journal_size,
            .commitID        = extraData.commit_id,
            .saveDataSpaceID = saveInfo->save_data_space_id};

    return true;
}

bool fs::process_save_meta_data(const FsSaveDataInfo *saveInfo, const SaveMetaData &meta) noexcept
{
    FsSaveDataExtraData extraData{};
    const bool extraRead = fs::read_save_extra_data(saveInfo, extraData);
    if (!extraRead) { return false; }

    const bool needsExtend = extraData.data_size < meta.saveDataSize;
    const bool extended    = needsExtend && fs::extend_save_data(saveInfo, meta.saveDataSize, meta.journalSize);
    if (needsExtend && !extended) { return false; }

    return true;
}

bool fs::read_save_meta(const fslib::Path &path, SaveMetaData &metaOut) noexcept
{
    if (fslib::directory_exists(path))
    {
        const fslib::Path metaPath{path / fs::NAME_SAVE_META};
        if (!fslib::file_exists(metaPath)) { return false; }

        fslib::File metaFile{metaPath, FsOpenMode_Read};
        const bool metaRead = metaFile.is_open() && metaFile.get_size() <= fs::SIZE_SAVE_META &&
                              metaFile.read(&metaOut, fs::SIZE_SAVE_META) <= fs::SIZE_SAVE_META;
        return metaRead && metaOut.magic == fs::SAVE_META_MAGIC;
    }

    fs::MiniUnzip zip{path};
    if (!zip.is_open()) { return false; }

    const bool located    = zip.locate_file(fs::NAME_SAVE_META);
    const bool sizeMatch  = located && zip.get_uncompressed_size() <= fs::SIZE_SAVE_META;
    const bool metaRead   = sizeMatch && zip.read(&metaOut, fs::SIZE_SAVE_META) <= fs::SIZE_SAVE_META;
    return metaRead && metaOut.magic == fs::SAVE_META_MAGIC;
}

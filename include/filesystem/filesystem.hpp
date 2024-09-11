#pragma once
#include <string>

#include <switch.h>

#include "filesystem/directoryListing.hpp"
#include "filesystem/fileParser.hpp"
#include "filesystem/zip.hpp"
#include "filesystem/io.hpp"

namespace fs
{
    // Default device name used for the currently open save filesystem
    extern const std::string DEFAULT_SAVE_MOUNT_DEVICE;
    // Inits fs, creates folders, etc
    bool init(void);
    // Mounts save from saveInfo
    bool mountSaveData(const FsSaveDataInfo &saveInfo);
    void unmountSaveData(void);
    // Commits to default save path
    void commitSaveData(void);
    // Creates save data filesystem. Might move this elsewhere later.
    bool createSaveDataFileSystem(FsSaveDataType saveType, uint64_t titleID, AccountUid accountID, uint16_t cacheSaveIndex);
    // Deletes save from system
    bool deleteSaveDataFileSystem(uint64_t saveDataID);
    // Creates directory for title using title ID
    void createTitleDirectoryByTID(uint64_t titleID);
    // Checks if folder actually contains data.
    bool directoryContainsFiles(const std::string &path);
}
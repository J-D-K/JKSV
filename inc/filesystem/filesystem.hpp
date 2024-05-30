#pragma once
#include <switch.h>
#include <string>
#include "filesystem/directoryListing.hpp"
#include "filesystem/fileParser.hpp"
#include "filesystem/zip.hpp"
#include "filesystem/io.hpp"

namespace fs
{
    // Default device name used for the currently open save filesystem
    extern const char *DEFAULT_SAVE_MOUNT_DEVICE;
    // Inits fs, creates folders, etc
    bool init(void);
    // Mounts save from saveInfo
    bool mountSaveData(const FsSaveDataInfo &saveInfo);
    void unmountSaveData(void);
    // Commits to default save path
    void commitSaveData(void);
    // Erases current save data from device
    void eraseSaveData(void);
}
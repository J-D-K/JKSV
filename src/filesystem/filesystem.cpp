#include <filesystem>
#include "filesystem/filesystem.hpp"
#include "config.hpp"
#include "log.hpp"

const char *fs::DEFAULT_SAVE_MOUNT_DEVICE = "save:/";

bool fs::init(void)
{
    // Create working directory
    std::string workingDirectory = config::getWorkingDirectory();
    if(std::filesystem::create_directories(workingDirectory) == false)
    {
        logger::log("fs::init(): Failed to create working directory. Might exist already.");
    }
    logger::log("fs::init(): Succeeded.");
    return true;
}

bool fs::mountSaveData(const FsSaveDataInfo &saveInfo)
{
    Result saveOpened = 0;
    FsFileSystem saveFileSystem;
    FsSaveDataAttribute saveAttribute;
    switch (saveInfo.save_data_type)
    {
        case FsSaveDataType_System:
        {
            saveAttribute =
            {
                .uid = saveInfo.uid, 
                .system_save_data_id = saveInfo.system_save_data_id, 
                .save_data_type = saveInfo.save_data_type
            };
            saveOpened = fsOpenSaveDataFileSystemBySystemSaveDataId(&saveFileSystem, static_cast<FsSaveDataSpaceId>(saveInfo.save_data_space_id), &saveAttribute);
        }
        break;

        case FsSaveDataType_Account:
        {
            saveAttribute = 
            {
                .application_id = saveInfo.application_id,
                .uid = saveInfo.uid,
                .save_data_type = saveInfo.save_data_type,
                .save_data_rank = saveInfo.save_data_rank,
                .save_data_index = saveInfo.save_data_index
            };
            saveOpened = fsOpenSaveDataFileSystem(&saveFileSystem, static_cast<FsSaveDataSpaceId>(saveInfo.save_data_space_id), &saveAttribute);
        }
        break;

        case FsSaveDataType_Device:
        {
            saveAttribute = 
            {
                .application_id = saveInfo.application_id,
                .save_data_type = FsSaveDataType_Device
            };
            saveOpened = fsOpenSaveDataFileSystem(&saveFileSystem, static_cast<FsSaveDataSpaceId>(saveInfo.save_data_space_id), &saveAttribute);
        }
        break;

        case FsSaveDataType_Bcat:
        {
            saveAttribute = 
            {
                .application_id = saveInfo.application_id,
                .save_data_type = FsSaveDataType_Bcat
            };
            saveOpened = fsOpenSaveDataFileSystem(&saveFileSystem, static_cast<FsSaveDataSpaceId>(saveInfo.save_data_space_id), &saveAttribute);
        }
        break;

        case FsSaveDataType_Cache:
        {
            saveAttribute = 
            {
                .application_id = saveInfo.application_id,
                .save_data_type = FsSaveDataType_Cache,
                .save_data_index = saveInfo.save_data_index
            };
            saveOpened = fsOpenSaveDataFileSystem(&saveFileSystem, static_cast<FsSaveDataSpaceId>(saveInfo.save_data_space_id), &saveAttribute);
        };
        break;

        default:
        {
            saveOpened = 1;
        }
        break;
    }
    return R_SUCCEEDED(saveOpened) && fsdevMountDevice("save", saveFileSystem) != -1;
}

void fs::unmountSaveData(void)
{
    fsdevUnmountDevice(fs::DEFAULT_SAVE_MOUNT_DEVICE);
}

void fs::commitSaveData(void)
{
    fsdevCommitDevice(DEFAULT_SAVE_MOUNT_DEVICE);
}

void fs::eraseSaveData(void)
{
    // Delete root of save container
    fs::io::deleteDirectoryRecursively(fs::DEFAULT_SAVE_MOUNT_DEVICE);
    // Commit changes
    fs::commitSaveData();
}
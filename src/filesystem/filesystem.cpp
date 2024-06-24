#include <filesystem>
#include "filesystem/filesystem.hpp"
#include "data/data.hpp"
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
    Result commitError = fsdevCommitDevice(fs::DEFAULT_SAVE_MOUNT_DEVICE);
    if(R_SUCCEEDED(commitError))
    {
        logger::log("Commit succeeded.");
    }
    else
    {
        logger::log("Commit failed. %X", commitError);
    }
}

void fs::createTitleDirectoryByTID(const uint64_t &titleID)
{
    // Get title info needed
    data::titleInfo *workingTitleInfo = data::getTitleInfoByTitleID(titleID);
    // Full path 
    std::filesystem::path fullTitlePath = config::getWorkingDirectory() + workingTitleInfo->getPathSafeTitle();
    // Create it
    std::filesystem::create_directories(fullTitlePath);
}

bool fs::directoryContainsFiles(const std::string &path)
{
    // Get a test listing.
    fs::directoryListing testListing(path);
    // If the list has files, true.
    if(testListing.getListingCount() > 0)
    {
        return true;
    }
    return false;
}
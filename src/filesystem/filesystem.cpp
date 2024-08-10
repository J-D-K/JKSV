#include <filesystem>

#include "filesystem/filesystem.hpp"
#include "data/data.hpp"
#include "config.hpp"
#include "log.hpp"

const std::string fs::DEFAULT_SAVE_MOUNT_DEVICE = "save:/";

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
    fsdevUnmountDevice(fs::DEFAULT_SAVE_MOUNT_DEVICE.c_str());
}

void fs::commitSaveData(void)
{
    Result commitError = fsdevCommitDevice(fs::DEFAULT_SAVE_MOUNT_DEVICE.c_str());
    if(R_SUCCEEDED(commitError))
    {
        logger::log("Commit succeeded.");
    }
    else
    {
        logger::log("Commit failed. %X", commitError);
    }
}

bool fs::createSaveDataFileSystem(FsSaveDataType saveType, uint64_t titleID, AccountUid accountID, uint16_t cacheSaveIndex)
{
    // Get working title 
    data::titleInfo *workingTitleInfo = data::getTitleInfoByTitleID(titleID);

    // Save attributes needed.
    FsSaveDataAttribute saveDataAttributes = 
    {
        .application_id = titleID,
        .uid = accountID,
        .system_save_data_id = 0,
        .save_data_type = saveType,
        .save_data_rank = 0,
        .save_data_index = cacheSaveIndex
    };

    // Creation info
    FsSaveDataCreationInfo saveCreationInfo = 
    {
        .save_data_size = workingTitleInfo->getSaveDataSize(saveType),
        .journal_size = workingTitleInfo->getJournalSize(saveType),
        .available_size = 0x4000,
        .owner_id = saveType == FsSaveDataType_Bcat ? 0x010000000000000C : workingTitleInfo->getSaveDataOwnerID(),
        .flags = 0,
        .save_data_space_id = FsSaveDataSpaceId_User
    };

    FsSaveDataMetaInfo saveMetaInfo = 
    {
        .size = saveType == FsSaveDataType_Bcat ? static_cast<uint32_t>(0) : static_cast<uint32_t>(0x40060),
        .type = saveType == FsSaveDataType_Bcat ? static_cast<uint8_t>(0) : static_cast<uint8_t>(FsSaveDataMetaType_Thumbnail)
    };

    Result createSaveResult = fsCreateSaveDataFileSystem(&saveDataAttributes, &saveCreationInfo, &saveMetaInfo);

    if(R_FAILED(createSaveResult))
    {
        logger::log("Error creating save data for 0x%016lX -> 0x%08X.", titleID, createSaveResult);
    }

    return R_SUCCEEDED(createSaveResult);
}

bool fs::deleteSaveDataFileSystem(uint64_t saveDataID)
{
    Result saveDeleteResult = fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId_User, saveDataID);

    if(R_FAILED(saveDeleteResult))
    {
        logger::log("Error deleting save data file system 0x%016lX -> 0x%08X", saveDataID, saveDeleteResult);
    }

    return R_SUCCEEDED(saveDeleteResult);
}

void fs::createTitleDirectoryByTID(uint64_t titleID)
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
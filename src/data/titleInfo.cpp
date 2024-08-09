#include <memory>
#include <cstring>

#include <switch.h>

#include "filesystem/filesystem.hpp"
#include "data/data.hpp"
#include "graphics/graphics.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

data::titleInfo::titleInfo(uint64_t titleID) : 
m_TitleID(titleID)
{
    // This is the size of the control data. Used for getting icon size
    uint64_t nsAppControlSize = 0;
    // Icon size
    size_t appIconSize = 0;
    // Actual control data.
    std::unique_ptr<NsApplicationControlData> nsAppConrolData = std::make_unique<NsApplicationControlData>();
    // Title ID in hex format
    std::string titleIDHexString = stringUtil::getFormattedString("%016lX", m_TitleID);

    Result nsAppControlRes = nsGetApplicationControlData(NsApplicationControlSource_Storage, m_TitleID, nsAppConrolData.get(), sizeof(NsApplicationControlData), &nsAppControlSize);
    if(R_SUCCEEDED(nsAppControlRes) && !(nsAppControlSize < sizeof(nsAppConrolData->nacp)))
    {
        appIconSize = nsAppControlSize - sizeof(nsAppConrolData->nacp);

        // Copy NACP
        std::memcpy(&m_Nacp, &nsAppConrolData->nacp, sizeof(NacpStruct));

        // Load icon
        m_Icon = graphics::textureManager::createTextureFromMem(titleIDHexString, nsAppConrolData->icon, appIconSize, graphics::IMAGE_TYPE_JPEG);
    }
    else
    {
        // Failed so init to hex and all 0's
        std::memset(&m_Nacp, 0x00, sizeof(NacpStruct));

        // Create the icon in this case
        std::string lowerTitleIDHexString = stringUtil::getFormattedString("%08X", static_cast<uint32_t>(m_TitleID));
        m_Icon = graphics::createIcon(lowerTitleIDHexString, lowerTitleIDHexString, 32);
    }
}

std::string data::titleInfo::getTitle(void)
{
    // Try to get title from NACP
    NacpLanguageEntry *entry;
    nacpGetLanguageEntry(&m_Nacp, &entry);
    // If it fails, return title ID as hex string
    if(entry == NULL)
    {
        return stringUtil::getFormattedString("%016lX", m_TitleID);
    }
    // Return the title
    return std::string(entry->name);
}

std::string data::titleInfo::getPathSafeTitle(void)
{
    // Try to get a pathSafeTitle
    std::string pathSafeTitle = stringUtil::getPathSafeString(data::titleInfo::getTitle());
    // If that fails, it will be empty. Just return ID hex string
    if(pathSafeTitle.empty())
    {
        return stringUtil::getFormattedString("%016lX", m_TitleID);
    }
    return pathSafeTitle;
}

graphics::sdlTexture data::titleInfo::getIcon(void)
{
    return m_Icon;
}

uint64_t data::titleInfo::getSaveDataSize(FsSaveDataType saveType)
{
    uint64_t saveDataSize = 0;
    switch(saveType)
    {
        case FsSaveDataType_Account:
        {
            saveDataSize = m_Nacp.user_account_save_data_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            saveDataSize = m_Nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            saveDataSize = m_Nacp.device_save_data_size;
        }
        break;

        default:
        {
            saveDataSize = 0;
        }
    }
    return saveDataSize;
}

uint64_t data::titleInfo::getSaveDataSizeMax(FsSaveDataType saveType)
{
    uint64_t saveSize = 0;
    uint64_t saveSizeMax = 0;

    switch(saveType)
    {
        case FsSaveDataType_Account:
        {
            saveSize = m_Nacp.user_account_save_data_size;
            saveSizeMax = m_Nacp.user_account_save_data_size_max;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            saveSize = m_Nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            saveSize = m_Nacp.device_save_data_size;
            saveSizeMax = m_Nacp.device_save_data_size_max;
        }
        break;

        default:
        {
            saveSize = 0;
            saveSizeMax = 0;
        }
        break;
    }
    return saveSizeMax > saveSize ? saveSizeMax : saveSize;
}

uint64_t data::titleInfo::getJournalSize(FsSaveDataType saveType)
{
    // Journal size to return
    uint64_t journalSize = 0;

    // Different save types have different journal limits
    switch(saveType)
    {
        case FsSaveDataType_Account:
        {
            // Just this is safe for accounts
            journalSize = m_Nacp.user_account_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            // Let's just hope this works all the time
            journalSize = m_Nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            journalSize = m_Nacp.device_save_data_journal_size;
        }
        break;

        default:
        {
            // This should make it commit on every write. Not the fastest, but safest.
            journalSize = 0;
        }
        break;
    }
    return journalSize;
}

uint64_t data::titleInfo::getJournalSizeMax(FsSaveDataType saveType)
{
    uint64_t journalSize = 0;
    uint64_t journalSizeMax = 0;

    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            journalSize = m_Nacp.user_account_save_data_journal_size;
            journalSizeMax = m_Nacp.user_account_save_data_journal_size_max;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            journalSize = m_Nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            journalSize = m_Nacp.device_save_data_journal_size;
            journalSizeMax = m_Nacp.device_save_data_journal_size_max;
        }
        break;

        default:
        {
            journalSize = 0;
            journalSizeMax = 0;
        }
        break;
    }
    return journalSizeMax > journalSize ? journalSizeMax : journalSize;
}

bool data::titleInfo::hasAccountSaveData(void)
{
    return m_Nacp.user_account_save_data_size > 0;
}

bool data::titleInfo::hasBCATSaveData(void)
{
    return m_Nacp.bcat_delivery_cache_storage_size > 0;
}

bool data::titleInfo::hasDeviceSaveData(void)
{
    return m_Nacp.device_save_data_size > 0;
}

bool data::titleInfo::hasCacheSaveData(void)
{
    return m_Nacp.cache_storage_size > 0;
}
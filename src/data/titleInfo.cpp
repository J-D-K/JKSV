#include <memory>
#include <cstring>
#include <switch.h>
#include "filesystem/filesystem.hpp"
#include "data/data.hpp"
#include "graphics/graphics.hpp"
#include "stringUtil.hpp"

#include "log.hpp"

data::titleInfo::titleInfo(const uint64_t &titleID) : m_TitleID(titleID)
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
        m_Icon = graphics::textureLoadFromMem(titleIDHexString, graphics::IMG_TYPE_JPEG, nsAppConrolData->icon, appIconSize);
    }
    else
    {
        // Failed so init to hex and all 0's
        std::memset(&m_Nacp, 0x00, sizeof(NacpStruct));

        // Create the icon in this case
        std::string lowerTitleIDHexString = stringUtil::getFormattedString("%08X", static_cast<uint32_t>(m_TitleID), 32);
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

SDL_Texture *data::titleInfo::getIcon(void)
{
    return m_Icon;
}

uint64_t data::titleInfo::getJournalSize(const FsSaveDataType &saveType)
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
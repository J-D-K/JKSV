#include <memory>
#include <cstring>
#include <switch.h>
#include "data/data.hpp"
#include "graphics/graphics.hpp"
#include "stringUtil.hpp"

#include "log.hpp"

data::titleInfo::titleInfo(const uint64_t &titleID) : m_TitleID(titleID)
{
    uint64_t nsAppControlSize = 0;
    size_t appIconSize = 0;
    std::unique_ptr<NsApplicationControlData> nsAppConrolData = std::make_unique<NsApplicationControlData>();
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
    NacpLanguageEntry *entry;
    nacpGetLanguageEntry(&m_Nacp, &entry);
    if(entry == NULL)
    {
        return stringUtil::getFormattedString("%016lX", m_TitleID);
    }
    return std::string(entry->name);
}

std::string data::titleInfo::getPathSafeTitle(void)
{
    std::string pathSafeTitle = stringUtil::getPathSafeString(data::titleInfo::getTitle());
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
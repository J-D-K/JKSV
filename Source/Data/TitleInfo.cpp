#include "Data/TitleInfo.hpp"
#include "Colors.hpp"
#include "StringUtil.hpp"
#include <cstring>

Data::TitleInfo::TitleInfo(uint64_t ApplicationID)
{
    // Used to calculate icon size.
    uint64_t NsAppControlSize = 0;
    // Actual control data.
    NsApplicationControlData NsControlData = {0};
    // Language entry
    NacpLanguageEntry *LanguageEntry = nullptr;

    Result NsError = nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                 ApplicationID,
                                                 &NsControlData,
                                                 sizeof(NsApplicationControlData),
                                                 &NsAppControlSize);

    if (R_FAILED(NsError) || NsAppControlSize < sizeof(NsControlData.nacp))
    {
        std::string ApplicationIDHex = StringUtil::GetFormattedString("%04X", ApplicationID & 0xFFFF);
        // Blank the nacp just to be sure.
        std::memset(&m_NACP, 0x00, sizeof(NacpStruct));
        // Create a place holder icon.
        int TextX = 128 - (SDL::Text::GetWidth(32, ApplicationIDHex.c_str()) / 2);
        m_Icon = SDL::TextureManager::CreateLoadTexture(ApplicationIDHex, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        SDL::Text::Render(m_Icon->Get(), TextX, 112, 32, SDL::Text::NO_TEXT_WRAP, Colors::White, ApplicationIDHex.c_str());
    }
    else if (R_SUCCEEDED(NsError) && R_SUCCEEDED(nacpGetLanguageEntry(&NsControlData.nacp, &LanguageEntry)))
    {
        // Memcpy the NACP since it has all the good stuff.
        std::memcpy(&m_NACP, &NsControlData.nacp, sizeof(NacpStruct));
        // Load the icon.
        m_Icon = SDL::TextureManager::CreateLoadTexture(LanguageEntry->name, NsControlData.icon, NsAppControlSize - sizeof(NacpStruct));
    }
}

const char *Data::TitleInfo::GetTitle(void)
{
    NacpLanguageEntry *Entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_NACP, &Entry)))
    {
        return nullptr;
    }
    return Entry->name;
}

const char *Data::TitleInfo::GetPublisher(void)
{
    NacpLanguageEntry *Entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_NACP, &Entry)))
    {
        return nullptr;
    }
    return Entry->author;
}

SDL::SharedTexture Data::TitleInfo::GetIcon(void) const
{
    return m_Icon;
}

#include "Data/TitleInfo.hpp"
#include "Colors.hpp"
#include "Logger.hpp"
#include "StringUtil.hpp"
#include <cstring>

Data::TitleInfo::TitleInfo(uint64_t ApplicationID)
{
    // Used to calculate icon size.
    uint64_t NsAppControlSize = 0;
    // Actual control data.
    NsApplicationControlData NsControlData;
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

        // Sprintf title ids to language entries for safety.
        sprintf(m_NACP.lang[SetLanguage_ENUS].name, "%016lX", ApplicationID);
        sprintf(m_PathSafeTitle, "%016lX", ApplicationID);

        // Create a place holder icon.
        int TextX = 128 - (SDL::Text::GetWidth(48, ApplicationIDHex.c_str()) / 2);
        m_Icon = SDL::TextureManager::CreateLoadTexture(ApplicationIDHex, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        m_Icon->Clear(Colors::DialogBox);
        SDL::Text::Render(m_Icon->Get(), TextX, 104, 48, SDL::Text::NO_TEXT_WRAP, Colors::White, ApplicationIDHex.c_str());
    }
    else if (R_SUCCEEDED(NsError) && R_SUCCEEDED(nacpGetLanguageEntry(&NsControlData.nacp, &LanguageEntry)))
    {
        // Memcpy the NACP since it has all the good stuff.
        std::memcpy(&m_NACP, &NsControlData.nacp, sizeof(NacpStruct));
        // Get a path safe version of the title.
        if (!StringUtil::SanitizeStringForPath(LanguageEntry->name, m_PathSafeTitle, 0x200))
        {
            std::sprintf(m_PathSafeTitle, "%016lX", ApplicationID);
        }
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

const char *Data::TitleInfo::GetPathSafeTitle(void)
{
    return m_PathSafeTitle;
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

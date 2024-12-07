#include "JKSV.hpp"
#include "AppStates/MainMenuState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Data/Data.hpp"
#include "FsLib.hpp"
#include "Input.hpp"
#include "Logger.hpp"
#include "SDL.hpp"
#include "Strings.hpp"
#include <switch.h>

#define ABORT_ON_FAILURE(x)                                                                                                                    \
    if (!x)                                                                                                                                    \
    {                                                                                                                                          \
        return;                                                                                                                                \
    }

namespace
{
    constexpr uint8_t BUILD_MON = 12;
    constexpr uint8_t BUILD_DAY = 5;
    constexpr uint16_t BUILD_YEAR = 2024;
} // namespace

template <typename... Args>
static bool InitializeService(Result (*Function)(Args...), const char *ServiceName, Args... Arguments)
{
    Result Error = (*Function)(Arguments...);
    if (R_FAILED(Error))
    {
        Logger::Log("Error initializing %s: 0x%X.", Error);
        return false;
    }
    return true;
}

JKSV::JKSV(void)
{
    // FsLib
    ABORT_ON_FAILURE(FsLib::Initialize());
    // This doesn't really on stdio or anything.
    Logger::Initialize();
    // Need to init RomFS here for now until I update FsLib to take care of this.
    ABORT_ON_FAILURE(InitializeService(romfsInit, "RomFS"));
    // Let FsLib take care of calls to SDMC instead of fs_dev
    ABORT_ON_FAILURE(FsLib::Dev::InitializeSDMC());

    // SDL
    ABORT_ON_FAILURE(SDL::Initialize("JKSV", 1280, 720));
    ABORT_ON_FAILURE(SDL::Text::Initialize());

    // Services.
    // Using administrator so JKSV can still run in Applet mode.
    ABORT_ON_FAILURE(InitializeService(accountInitialize, "Account", AccountServiceType_Administrator));
    ABORT_ON_FAILURE(InitializeService(nsInitialize, "NS"));
    ABORT_ON_FAILURE(InitializeService(pdmqryInitialize, "PDMQry"));
    ABORT_ON_FAILURE(InitializeService(plInitialize, "PL", PlServiceType_User));
    ABORT_ON_FAILURE(InitializeService(pmshellInitialize, "PMShell"));
    ABORT_ON_FAILURE(InitializeService(setInitialize, "Set"));
    ABORT_ON_FAILURE(InitializeService(setsysInitialize, "SetSys"));
    ABORT_ON_FAILURE(InitializeService(socketInitializeDefault, "Socket"));

    // Input doesn't have anything to return.
    Input::Initialize();

    // Neither does config.
    Config::Initialize();

    // JKSV also has no internal strings anymore. This is FATAL now.
    ABORT_ON_FAILURE(Strings::Initialize());

    if (!Data::Initialize())
    {
        return;
    }

    // Install/setup our color changing characters.
    SDL::Text::AddColorCharacter(L'#', Colors::Blue);
    SDL::Text::AddColorCharacter(L'*', Colors::Red);
    SDL::Text::AddColorCharacter(L'<', Colors::Yellow);
    SDL::Text::AddColorCharacter(L'>', Colors::Green);

    // This is to check whether the author wanted credit for their work.
    m_ShowTranslationInfo = std::char_traits<char>::compare(Strings::GetByName(Strings::Names::TranslationInfo, 1), "NULL", 4) != 0;

    // This can't be in an initializer list because it needs SDL initialized.
    m_HeaderIcon = SDL::TextureManager::CreateLoadTexture("HeaderIcon", "romfs:/Textures/HeaderIcon.png");

    // Push initial main menu state.
    JKSV::PushState(std::make_shared<MainMenuState>());

    m_IsRunning = true;
}

JKSV::~JKSV()
{
    socketExit();
    setsysExit();
    setExit();
    pmshellExit();
    plExit();
    pdmqryExit();
    nsExit();
    accountExit();
    SDL::Text::Exit();
    SDL::Exit();
    FsLib::Exit();
}

bool JKSV::IsRunning(void) const
{
    return m_IsRunning;
}

void JKSV::Update(void)
{
    Input::Update();

    if (Input::ButtonPressed(HidNpadButton_Plus))
    {
        m_IsRunning = false;
    }

    if (!m_StateVector.empty())
    {
        while (!m_StateVector.back()->IsActive())
        {
            m_StateVector.pop_back();
            m_StateVector.back()->GiveFocus();
        }
        m_StateVector.back()->Update();
    }
}

void JKSV::Render(void)
{
    SDL::FrameBegin(Colors::ClearColor);
    // Top and bottom divider lines.
    SDL::RenderLine(NULL, 30, 88, 1250, 88, Colors::White);
    SDL::RenderLine(NULL, 30, 648, 1250, 648, Colors::White);
    // Icon
    m_HeaderIcon->Render(NULL, 66, 27);
    // "JKSV"
    SDL::Text::Render(NULL, 130, 32, 34, SDL::Text::NO_TEXT_WRAP, Colors::White, "JKSV");
    // Translation info in bottom left.
    if (m_ShowTranslationInfo)
    {
        SDL::Text::Render(NULL,
                          8,
                          680,
                          14,
                          SDL::Text::NO_TEXT_WRAP,
                          Colors::White,
                          Strings::GetByName(Strings::Names::TranslationInfo, 0),
                          Strings::GetByName(Strings::Names::TranslationInfo, 1));
    }

    if (!m_StateVector.empty())
    {
        for (auto &CurrentState : m_StateVector)
        {
            CurrentState->Render();
        }
    }

    SDL::Text::Render(NULL, 8, 700, 14, SDL::Text::NO_TEXT_WRAP, Colors::White, "v. %02d.%02d.%04d", BUILD_MON, BUILD_DAY, BUILD_YEAR);

    SDL::FrameEnd();
}

void JKSV::PushState(std::shared_ptr<AppState> NewState)
{
    if (!m_StateVector.empty())
    {
        m_StateVector.back()->TakeFocus();
    }
    NewState->GiveFocus();
    m_StateVector.push_back(NewState);
}

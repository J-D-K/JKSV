#include "AppStates/SettingsState.hpp"
#include "Colors.hpp"
#include "Input.hpp"
#include "Strings.hpp"

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

SettingsState::SettingsState(void)
    : m_SettingsMenu(32, 18, 1002, 32, 4, 555),
      m_RenderTarget(SDL::TextureManager::CreateLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    // Loop through strings and add them until nullptr.
    int CurrentString = 0;
    const char *SettingsString = nullptr;
    while ((SettingsString = Strings::GetByName(Strings::Names::SettingsMenu, CurrentString++)) != nullptr)
    {
        m_SettingsMenu.AddOption(SettingsString);
    }
}

void SettingsState::Update(void)
{
    m_SettingsMenu.Update(AppState::HasFocus());

    if (Input::ButtonPressed(HidNpadButton_B))
    {
        AppState::Deactivate();
    }
}

void SettingsState::Render(void)
{
    m_RenderTarget->Clear(Colors::Transparent);
    m_SettingsMenu.Render(m_RenderTarget->Get(), AppState::HasFocus());
    m_RenderTarget->Render(NULL, 201, 91);
}

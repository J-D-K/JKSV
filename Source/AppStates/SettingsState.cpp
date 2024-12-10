#include "AppStates/SettingsState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "StringUtil.hpp"
#include "Strings.hpp"

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

static inline const char *GetValueText(uint8_t Value)
{
    return Value == 1 ? Strings::GetByName(Strings::Names::OnOff, 0) : Strings::GetByName(Strings::Names::OnOff, 1);
}

SettingsState::SettingsState(void)
    : m_SettingsMenu(32, 8, 1000, 24, 555),
      m_RenderTarget(SDL::TextureManager::CreateLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    // Add the first two, because they don't have values to display.
    m_SettingsMenu.AddOption(Strings::GetByName(Strings::Names::SettingsMenu, 0));
    m_SettingsMenu.AddOption(Strings::GetByName(Strings::Names::SettingsMenu, 1));

    int CurrentString = 2;
    const char *SettingsString = nullptr;
    while (CurrentString < 16 && (SettingsString = Strings::GetByName(Strings::Names::SettingsMenu, CurrentString++)) != nullptr)
    {
        m_SettingsMenu.AddOption(StringUtil::GetFormattedString(SettingsString, GetValueText(Config::GetByIndex(CurrentString - 1))));
    }
    // Add the scaling
    m_SettingsMenu.AddOption(
        StringUtil::GetFormattedString(Strings::GetByName(Strings::Names::SettingsMenu, 17), Config::GetAnimationScaling()));
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

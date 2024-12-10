#include "AppStates/ExtrasMenuState.hpp"
#include "Colors.hpp"
#include "Input.hpp"
#include "Strings.hpp"
#include <string_view>

namespace
{
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
}

ExtrasMenuState::ExtrasMenuState(void)
    : m_ExtrasMenu(32, 8, 1000, 24, 555),
      m_RenderTarget(SDL::TextureManager::CreateLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    const char *ExtrasString = nullptr;
    int CurrentString = 0;
    while ((ExtrasString = Strings::GetByName(Strings::Names::ExtrasMenu, CurrentString++)) != nullptr)
    {
        m_ExtrasMenu.AddOption(ExtrasString);
    }
}

void ExtrasMenuState::Update(void)
{
    m_ExtrasMenu.Update(AppState::HasFocus());

    if (Input::ButtonPressed(HidNpadButton_B))
    {
        AppState::Deactivate();
    }
}

void ExtrasMenuState::Render(void)
{
    m_RenderTarget->Clear(Colors::Transparent);
    m_ExtrasMenu.Render(m_RenderTarget->Get(), AppState::HasFocus());
    m_RenderTarget->Render(NULL, 201, 91);
}

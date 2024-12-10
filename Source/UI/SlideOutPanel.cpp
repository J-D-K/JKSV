#include "UI/SlideOutPanel.hpp"
#include "Config.hpp"

UI::SlideOutPanel::SlideOutPanel(int Width, SlideOutPanel::Side Side) : m_X(Side == Side::Left ? -Width : 1280), m_Side(Side)
{
    static int SlidePanelTargetID = 0;
    std::string PanelTargetName = "PanelTarget_" + std::to_string(SlidePanelTargetID++);
    m_RenderTarget = SDL::TextureManager::CreateLoadTexture(PanelTargetName, Width, 720, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

void UI::SlideOutPanel::Update(bool HasFocus)
{
    double Scaling = Config::GetAnimationScaling();
    if (m_Side == Side::Left && m_X < 0)
    {
        m_X += m_X / Scaling;
    }
    else if (m_Side == Side::Right && m_X > 1280)
    {
        m_X -= m_X / Scaling;
    }
    else
    {
        m_IsOpen = true;
    }

    if (HasFocus && m_IsOpen)
    {
        for (auto &CurrentElement : m_Elements)
        {
            CurrentElement->Update(HasFocus);
        }
    }
}

void UI::SlideOutPanel::Render(SDL_Texture *Target, bool HasFocus)
{
    for (auto &CurrentElement : m_Elements)
    {
        CurrentElement->Render(m_RenderTarget->Get(), HasFocus);
    }
    m_RenderTarget->Render(NULL, m_X, 0);
}

void UI::SlideOutPanel::Close(void)
{
}

bool UI::SlideOutPanel::IsOpen(void) const
{
    return m_IsOpen;
}

void UI::SlideOutPanel::PushNewElement(std::shared_ptr<UI::Element> NewElement)
{
    m_Elements.push_back(NewElement);
}

SDL_Texture *UI::SlideOutPanel::Get(void)
{
    return m_RenderTarget->Get();
}

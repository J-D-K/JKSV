#include "appStates/appState.hpp"

bool appState::isActive(void)
{
    return m_IsActive;
}

void appState::deactivateState(void)
{
    m_IsActive = false;
}

void appState::giveFocus(void)
{
    m_HasFocus = true;
}

void appState::takeFocus(void)
{
    m_HasFocus = false;
}

bool appState::hasFocus(void) const
{
    return m_HasFocus;
}
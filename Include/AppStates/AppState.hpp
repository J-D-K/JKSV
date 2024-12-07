#pragma once
#include "SDL.hpp"
#include <memory>

class AppState
{
    public:
        AppState(void) = default;
        virtual ~AppState() {};

        virtual void Update(void) = 0;
        virtual void Render(void) = 0;

        // Allows the state to be reactivated if needed.
        void Reactivate(void)
        {
            m_IsActive = true;
        }

        // Returns whether or not state is still active or can be purged.
        bool IsActive(void) const
        {
            return m_IsActive;
        }

        void Deactivate(void)
        {
            m_IsActive = false;
        }

        void GiveFocus(void)
        {
            m_HasFocus = true;
        }

        void TakeFocus(void)
        {
            m_HasFocus = false;
        }

        // Returns whether state is at back of vector and has focus.
        bool HasFocus(void) const
        {
            return m_HasFocus;
        }

    private:
        // Whether state is still active or can be purged.
        bool m_IsActive = true;
        // Whether or not state has focus
        bool m_HasFocus = false;
};

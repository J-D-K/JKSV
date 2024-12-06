#pragma once
#include "AppStates/AppState.hpp"
#include "SDL.hpp"
#include <memory>
#include <vector>

class JKSV
{
    public:
        // Initializes JKSV.
        JKSV(void);
        // Exits JKSV
        ~JKSV();
        // Returns whether or not JKSV is actually running.
        bool IsRunning(void) const;
        // Updates input and back of state vector.
        void Update(void);
        // Renders base of app and states of vector.
        void Render(void);
        // Pushes a new state to the back of state vector.
        static void PushState(std::shared_ptr<AppState> NewState);

    private:
        // Whether or not initialization was successful.
        bool m_IsRunning = false;
        // Whether or not translation author wants to be acknowledged.
        bool m_ShowTranslationInfo = false;
        // Header icon
        SDL::SharedTexture m_HeaderIcon = nullptr;
        // Vector of states.
        static inline std::vector<std::shared_ptr<AppState>> m_StateVector;
};

#pragma once
#include <memory>

#include "appStates/appState.hpp"

namespace jksv
{
    // Inits all services/code needed
    bool init(void);
    // Exits and frees memory needed
    void exit(void);
    // Updates logic/states
    void update(void);
    // Renders states
    void render(void);
    // Returns if JKSV is still running
    const bool isRunning(void);
    // Pushes newAppState to back of vector
    void pushNewState(std::unique_ptr<appState> &newAppState);
}
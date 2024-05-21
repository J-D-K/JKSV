#pragma once
#include <memory>
#include "appStates/appState.hpp"

namespace jksv
{
    bool init(void);
    void exit(void);
    void update(void);
    void render(void);
    const bool isRunning(void);
    void pushNewState(std::unique_ptr<appState> &newAppState);
}
#include "appStates/progressState.hpp"

progressState::progressState(sys::taskFunction threadFunction, std::shared_ptr<sys::progressArgs> args, const uint64_t &maxValue)
{
    m_Task = std::make_unique<sys::task>(threadFunction, args);
    m_ProgressBar = std::make_unique<ui::progressBar>(maxValue);
}

progressState::~progressState() { }

void progressState::update(void)
{
}

void progressState::render()
{
    
}
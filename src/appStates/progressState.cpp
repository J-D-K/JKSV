#include "appStates/progressState.hpp"

progressState::progressState(sys::taskFunction function, std::shared_ptr<sys::progressArgs> args, const uint64_t &maxValue)
{
    m_Task = std::make_unique<sys::task>(function, args);
    m_ProgressBar = std::make_unique<ui::progressBar>(maxValue);
}

progressState::~progressState() { }

void progressState::update(void)
{
}

void progressState::render()
{
    
}
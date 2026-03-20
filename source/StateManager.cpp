#include "StateManager.hpp"

void StateManager::update(const sdl2::Input &input)
{
    // Grab the instance.
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    if (stateVector.empty()) { return; }

    {
        auto &back = stateVector.back();
        if (back->is_active() && back->has_focus()) { back->update(input); }
    }

    // Purge uneeded states.
    for (auto current = stateVector.begin(); current != stateVector.end();)
    {
        BaseState *state = current->get();

        if (!state->is_active())
        {
            state->take_focus();
            current = stateVector.erase(current);
            continue;
        }
        ++current;
    }

    if (stateVector.empty()) { return; }

    // Run sub update routines.
    for (auto iter = stateVector.begin(); iter < stateVector.end() - 1; iter++) { (*iter)->sub_update(); }

    {
        // Double check to make sure the back has focus.
        auto &back = stateVector.back();
        if (!back->has_focus()) { back->give_focus(); }
    }
}

void StateManager::render(sdl2::Renderer &renderer) noexcept
{
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    for (std::shared_ptr<BaseState> &state : stateVector) { state->render(renderer); }
}

bool StateManager::back_is_closable() noexcept
{
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    if (stateVector.empty()) { return true; }

    std::shared_ptr<BaseState> &state = stateVector.back();
    return state->is_closable();
}

void StateManager::push_state(std::shared_ptr<BaseState> newState)
{
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    if (!stateVector.empty()) { stateVector.back()->take_focus(); }

    // Give the incoming state focus and then push it.
    newState->give_focus();
    stateVector.push_back(newState);
}

StateManager &StateManager::get_instance()
{
    static StateManager instance;
    return instance;
}

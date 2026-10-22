#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/Menu.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

/// @brief Small picker that returns a selected user through a callback.
class UserSelectState final : public BaseState
{
public:
    /// @brief Callback invoked with the selected user, or nullptr when cancelled.
    using Callback = std::function<void(data::User *)>;

    /// @brief Constructs a new UserSelectState.
    /// @param users Users to present.
    /// @param callback Callback to invoke on selection or cancellation.
    UserSelectState(const std::vector<data::User *> &users, Callback callback);

    /// @brief Returns a new UserSelectState.
    static inline std::shared_ptr<UserSelectState> create(const std::vector<data::User *> &users, Callback callback)
    {
        return std::make_shared<UserSelectState>(users, std::move(callback));
    }

    /// @brief Pushes a picker when more than one user is supplied.
    /// @param users Compatible users to present.
    /// @param callback Callback invoked with the selected user, or nullptr when cancelled.
    /// @return True if a picker was pushed. False if the selection was resolved without a state.
    static bool create_and_push_if_needed(const std::vector<data::User *> &users, Callback callback);

    /// @brief Handles user selection, cancellation, and menu input.
    void update() override;

    /// @brief Updates the control guide animation.
    void sub_update() override;

    /// @brief Draws the picker and control guide.
    void render() override;

private:
    /// @brief Users presented by the picker.
    std::vector<data::User *> m_users{};

    /// @brief Callback invoked with the selected user or nullptr.
    Callback m_callback{};

    /// @brief Menu of user nicknames.
    std::shared_ptr<ui::Menu> m_menu{};

    /// @brief Control guide for the picker.
    std::shared_ptr<ui::ControlGuide> m_controlGuide{};

    /// @brief Render target used to cover the underlying state.
    sdl::SharedTexture m_renderTarget{};

    /// @brief Ensures the selection is only reported once.
    bool m_finished{};

    /// @brief Reports the selection and deactivates the state.
    /// @param user Selected user. nullptr when cancelled.
    void finish(data::User *user);
};

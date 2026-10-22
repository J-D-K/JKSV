#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "peer/ReceivedBackup.hpp"
#include "sdl.hpp"
#include "tasks/saveimport.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/Menu.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ui
{
enum class PopKind : uint8_t;
}

/// @brief Post-receive screen that selects received backups and applies them to one user.
class ReceivedBackupsState final : public BaseState
{
public:
    /// @brief Constructs a new ReceivedBackupsState.
    /// @param backups Backups received in the batch.
    ReceivedBackupsState(std::vector<peer::ReceivedBackup> backups);

    /// @brief Returns a new ReceivedBackupsState.
    /// @param backups Backups received in the batch.
    static inline std::shared_ptr<ReceivedBackupsState> create(std::vector<peer::ReceivedBackup> backups)
    {
        return std::make_shared<ReceivedBackupsState>(std::move(backups));
    }

    /// @brief Creates and pushes a new ReceivedBackupsState.
    /// @param backups Backups received in the batch.
    static void create_and_push(std::vector<peer::ReceivedBackup> backups);

    /// @brief Handles input for the current stage and advances the flow.
    void update() override;

    /// @brief Updates the control guide animation.
    void sub_update() override;

    /// @brief Draws the header, list, and control guide.
    void render() override;

private:
    /// @brief Stages of the post-receive flow.
    enum class Stage : uint8_t
    {
        Start,
        WaitingUser,
        List,
        Applying,
        Done
    };

    /// @brief Backups received in the batch.
    std::vector<peer::ReceivedBackup> m_received{};

    /// @brief Indices into m_received that can be applied to the selected user.
    std::vector<int> m_applicable{};

    /// @brief Base display strings, parallel to m_applicable.
    std::vector<std::string> m_display{};

    /// @brief Menu indices marked by the user.
    std::vector<int> m_marked{};

    /// @brief Target user selected by the user.
    data::User *m_user{};

    /// @brief Set by the user picker callback.
    bool m_userReady{};

    /// @brief Current stage of the flow.
    Stage m_stage{Stage::Start};

    /// @brief Menu of applicable backups.
    std::shared_ptr<ui::Menu> m_menu{};

    /// @brief Control guide.
    std::shared_ptr<ui::ControlGuide> m_controlGuide{};

    /// @brief Render target used to cover the underlying state.
    sdl::SharedTexture m_renderTarget{};

    /// @brief Header status string.
    std::string m_status{};

    /// @brief Batch task data shared with the import task.
    std::shared_ptr<tasks::saveimport::BatchData> m_batchData{};

    /// @brief Builds the applicable list and starts the user selection.
    void start();

    /// @brief Builds the backup menu for the selected user.
    void build_list();

    /// @brief Handles input while the backup list is shown.
    /// @param hasFocus Whether this state has focus.
    void update_list(bool hasFocus);

    /// @brief Applies the marked backups, or the highlighted one when nothing is marked.
    void apply_selected();

    /// @brief Reports a message through the pop manager and header.
    void report(int index, const char *fallback, ui::PopKind kind);

    /// @brief Rewrites the menu option marker for the applicable entry at index.
    void set_option(int index, bool marked);

    /// @brief Toggles the mark of the applicable entry at index.
    /// @note Only one backup per game can stay marked; marking another of the
    ///       same title unmarks the previous one.
    void toggle_mark(int index);

    /// @brief Returns whether the user already has save data for the backup.
    bool save_exists_for_user(data::User *user, const fs::SaveMetaData &meta);

    /// @brief Returns the display title for the backup at the received index.
    std::string title_for(int receivedIndex) const;
};

#pragma once

#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "peer/ReceivedBackup.hpp"
#include "sdl.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/Menu.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ui
{
enum class PopKind : uint8_t;
}

/// @brief Peer save transfer screen (LAN transport).
class PeerState final : public BaseState
{
public:
    /// @brief Builds the peer menu and control guide.
    PeerState();

    /// @brief Cancels and joins any running worker before releasing resources.
    ~PeerState();

    /// @brief Returns a new PeerState.
    static inline std::shared_ptr<PeerState> create() { return std::make_shared<PeerState>(); }

    /// @brief Creates and pushes the peer state.
    /// @return The pushed state.
    static inline std::shared_ptr<PeerState> create_and_push()
    {
        auto s = PeerState::create();
        StateManager::push_state(s);
        return s;
    }

    /// @brief Handles menu input and drains worker events on the UI thread.
    void update() override;

    /// @brief Joins a worker that has finished its transfer.
    void sub_update() override;

    /// @brief Draws the IP header, status line, menu, and control guide.
    void render() override;

private:
    /// @brief A one-shot message to present as a pop on the UI thread.
    struct PeerEvent
    {
        ui::PopKind kind; ///< Severity used by the pop manager.
        std::string text; ///< Message to display.
        bool silent{};    ///< Suppresses the pop sound when true.
    };

    /// @brief Creates the main menu and control guide. UI thread only.
    void initialize_menu();

    /// @brief Rebuilds menu options for the current mode. UI thread only.
    void rebuild_menu();

    /// @brief Handles input while the send selection list is active. UI thread only.
    void update_selecting();

    /// @brief Starts the host worker for the supplied selection.
    /// @param selection Backup paths in title or title/backup form.
    void start_host(std::vector<std::string> selection);

    /// @brief Worker: waits for a peer, pairs, then streams the selected backups.
    /// @note Runs off the UI thread; reports through push_event().
    void do_host();

    /// @brief Worker: discovers or connects to a host, pairs, then receives a batch.
    /// @note Runs off the UI thread; reports through push_event().
    void do_join();

    /// @brief Replaces the control guide text. Safe for the UI thread only.
    void set_control_guide(const char *guide);

    /// @brief Returns whether the backup index is marked for sending.
    bool is_marked(int backupIndex) const;

    /// @brief Toggles the mark of the backup index.
    void toggle_mark(int backupIndex);

    /// @brief Enqueues a pop event. Safe to call from the worker thread.
    void push_event(ui::PopKind kind, std::string text, bool silent = false);

    /// @brief Moves queued events to the pop manager. Must run on the UI thread.
    void drain_events();

    /// @brief Persists the last receiver IP once the worker has finished.
    void flush_pending_ip();

    sdl::SharedTexture m_renderTarget{};                ///< Covers the underlying state.
    std::shared_ptr<ui::Menu> m_menu{};                 ///< Main or selection menu.
    std::shared_ptr<ui::ControlGuide> m_controlGuide{}; ///< Bottom control guide.

    /// @brief High-level input mode.
    enum class Mode
    {
        Menu,      ///< Main Send/Receive menu.
        Selecting, ///< Marking backups for a batch send.
        Busy       ///< A worker is running; only cancellation is accepted.
    };
    std::atomic<Mode> m_mode{Mode::Menu}; ///< Current mode, written by the UI thread.

    std::atomic<bool> m_busy{false};          ///< True while a worker owns the transport.
    std::atomic<bool> m_cancel{false};        ///< Requested cancellation.
    std::atomic<bool> m_confirm{false};       ///< Pairing confirmation from the UI.
    std::atomic<bool> m_batch_pending{false}; ///< A received batch is ready to present.
    std::mutex m_status_mutex{};              ///< Guards m_status and m_pending_last_ip.
    std::string m_status{};                   ///< Header status text.
    std::mutex m_event_mutex{};               ///< Guards m_events.
    std::vector<PeerEvent> m_events{};        ///< Worker-to-UI event queue.
    std::string m_pending_last_ip{};          ///< Last receiver IP, flushed on the UI thread.

    // Host selection. Only touched on the UI thread until the send starts.
    std::vector<std::string> m_backup_options{}; ///< Raw backup paths.
    std::vector<std::string> m_backup_display{}; ///< Menu labels.
    std::vector<int> m_backup_map{};             ///< Menu index to backup index.
    std::vector<int> m_marked{};                 ///< Marked backup indices.
    std::vector<std::string> m_send_list{};      ///< Snapshot handed to the worker.

    // Received batch handed from the worker to the UI thread.
    std::vector<peer::ReceivedBackup> m_received_backups{}; ///< Completed batch.

    std::thread m_worker{}; ///< Host or join worker; joined on the UI thread.
};

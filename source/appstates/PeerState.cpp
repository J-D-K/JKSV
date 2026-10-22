#include "appstates/PeerState.hpp"
#include "appstates/ReceivedBackupsState.hpp"

#include "StateManager.hpp"
#include "config/config.hpp"
#include "config/keys.hpp"
#include "data/data.hpp"
#include "fs/MiniZip.hpp"
#include "fs/SaveMetaData.hpp"
#include "fs/zip.hpp"
#include "fslib.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "keyboard/keyboard.hpp"
#include "peer/LanTransport.hpp"
#include "peer/Protocol.hpp"
#include "peer/Session.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/Menu.hpp"
#include "ui/PopMessageManager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{
constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";

const char *peer_string(int index, const char *fallback)
{
    const char *string = strings::get_by_name(strings::names::PEER, index);
    return string ? string : fallback;
}

/// @brief Returns an unused temporary ZIP path in the working directory.
/// @return A path that does not collide with an existing file or directory.
/// @note The name includes the tick count so a stale archive from an interrupted
///       transfer is never overwritten or mistaken for a user backup.
fslib::Path make_temp_zip_path()
{
    const fslib::Path working = config::get_working_directory();
    const std::string base    = "peer_send_" + std::to_string(SDL_GetTicks64());
    fslib::Path path          = working / (base + ".zip");

    int suffix = 1;
    while (fslib::file_exists(path) || fslib::directory_exists(path))
    {
        path = working / (base + "_" + std::to_string(suffix) + ".zip");
        suffix++;
    }
    return path;
}

/// @brief Returns a non-existing path for the backup in the title directory.
/// @param titleDir Directory the backup belongs to.
/// @param backupName Original backup filename.
/// @return A path that does not collide with an existing file.
/// @note Duplicate names get a _1, _2, ... suffix before the extension so a
///       second transfer never overwrites an earlier backup.
fslib::Path unique_target_path(const fslib::Path &titleDir, const std::string &backupName)
{
    fslib::Path target = titleDir / backupName;
    if (!fslib::file_exists(target) && !fslib::directory_exists(target)) { return target; }

    std::string stem = backupName;
    std::string extension;
    const size_t dot = backupName.find_last_of('.');
    if (dot != std::string::npos && dot > 0)
    {
        stem      = backupName.substr(0, dot);
        extension = backupName.substr(dot);
    }

    int suffix = 1;
    fslib::Path candidate;
    do {
        candidate = titleDir / (stem + "_" + std::to_string(suffix) + extension);
        suffix++;
    } while (fslib::file_exists(candidate) || fslib::directory_exists(candidate));

    return candidate;
}
} // namespace

PeerState::PeerState()
    : m_renderTarget(sdl::TextureManager::load(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_TARGET))
{
    m_controlGuide = ui::ControlGuide::create(peer_string(41, "A: Select  B: Back"));
    PeerState::initialize_menu();
}

PeerState::~PeerState()
{
    m_cancel.store(true);
    if (m_worker.joinable()) m_worker.join();
    PeerState::flush_pending_ip();
}

void PeerState::push_event(ui::PopKind kind, std::string text, bool silent)
{
    std::lock_guard<std::mutex> lk(m_event_mutex);
    m_events.push_back({kind, std::move(text), silent});
}

void PeerState::drain_events()
{
    std::vector<PeerEvent> events;
    {
        std::lock_guard<std::mutex> lk(m_event_mutex);
        events.swap(m_events);
    }

    const int ticks = ui::PopMessageManager::DEFAULT_TICKS;
    for (auto &event : events) { ui::PopMessageManager::push_message(ticks, event.text, event.kind, event.silent); }
}

void PeerState::flush_pending_ip()
{
    std::string ip;
    {
        std::lock_guard<std::mutex> lk(m_status_mutex);
        ip.swap(m_pending_last_ip);
    }
    if (!ip.empty()) { config::set_peer_last_ip(ip); }
}

void PeerState::initialize_menu()
{
    if (!m_menu) { m_menu = ui::Menu::create(32, 10, 1000, 23, 555); }
    rebuild_menu();
}

void PeerState::rebuild_menu()
{
    if (!m_menu) { m_menu = ui::Menu::create(32, 10, 1000, 23, 555); }
    m_menu->reset();

    if (m_mode.load() == Mode::Selecting)
    {
        const int count = static_cast<int>(m_backup_display.size());
        for (int i = 0; i < count; i++)
        {
            const int backupIndex = m_backup_map[i];
            if (backupIndex >= 0)
            {
                const std::string option = (is_marked(backupIndex) ? "[x] " : "[ ] ") + m_backup_display[i];
                m_menu->add_option(option);
            }
            else { m_menu->add_option(m_backup_display[i]); }
        }
        const char *back = strings::get_by_name(strings::names::PEER, 2);
        m_menu->add_option(back ? back : "Back");
    }
    else
    {
        const char *fallback[] = {"Send", "Receive", "Back"};
        for (int i = 0; i < 3; i++)
        {
            const char *opt = strings::get_by_name(strings::names::PEER, i);
            m_menu->add_option(opt ? opt : fallback[i]);
        }
    }
}

void PeerState::set_control_guide(const char *guide) { m_controlGuide = ui::ControlGuide::create(guide); }

bool PeerState::is_marked(int backupIndex) const
{
    return std::find(m_marked.begin(), m_marked.end(), backupIndex) != m_marked.end();
}

void PeerState::toggle_mark(int backupIndex)
{
    auto found = std::find(m_marked.begin(), m_marked.end(), backupIndex);
    if (found == m_marked.end()) { m_marked.push_back(backupIndex); }
    else { m_marked.erase(found); }
}

void PeerState::start_host(std::vector<std::string> selection)
{
    m_send_list = std::move(selection);
    m_backup_options.clear();
    m_backup_display.clear();
    m_backup_map.clear();
    m_marked.clear();

    m_cancel.store(false);
    m_busy.store(true);
    m_mode.store(Mode::Busy);
    PeerState::set_control_guide(peer_string(41, "A: Select  B: Back"));
    rebuild_menu();
    {
        std::lock_guard<std::mutex> lk(m_status_mutex);
        m_status = peer_string(8, "Starting send...");
    }

    m_worker = std::thread(&PeerState::do_host, this);
}

void PeerState::update()
{
    const bool hasFocus = BaseState::has_focus();
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    m_menu->update(hasFocus);
    m_controlGuide->update(hasFocus);

    const bool plusPressed = input::button_pressed(HidNpadButton_Plus);

    if (plusPressed)
    {
        m_cancel.store(true);
        if (m_worker.joinable()) m_worker.join();
        PeerState::flush_pending_ip();
        return;
    }

    // Events are produced by the worker but presented here on the UI thread.
    PeerState::drain_events();

    // Reap finished worker and reset mode so buttons reactivate.
    if (!m_busy.load() && m_worker.joinable())
    {
        m_worker.join();
        PeerState::flush_pending_ip();
        // The worker reports its final result (including the receive-complete
        // pop) after the early drain_events() above, so flush it before the
        // post-receive screen takes focus.
        PeerState::drain_events();
    }
    if (!m_busy.load() && m_mode.load() == Mode::Busy)
    {
        m_mode.store(Mode::Menu);
        m_backup_options.clear();
        m_backup_display.clear();
        m_backup_map.clear();
        m_marked.clear();
        m_send_list.clear();
        PeerState::set_control_guide(peer_string(41, "A: Select  B: Back"));
        rebuild_menu();
    }

    // Hand a completed batch to the post-receive screen (3.8).
    if (m_batch_pending.load() && !m_busy.load())
    {
        m_batch_pending.store(false);
        std::vector<peer::ReceivedBackup> received;
        {
            std::lock_guard<std::mutex> lk(m_status_mutex);
            received.swap(m_received_backups);
        }
        ReceivedBackupsState::create_and_push(std::move(received));
        return;
    }

    if (m_busy.load())
    {
        if (input::button_pressed(HidNpadButton_B)) { m_cancel.store(true); }
        if (input::button_pressed(HidNpadButton_A)) { m_confirm.store(true); }
        return;
    }

    if (m_mode.load() == Mode::Selecting)
    {
        PeerState::update_selecting();
        return;
    }

    const bool aPressed = input::button_pressed(HidNpadButton_A);

    if (aPressed)
    {
        switch (m_menu->get_selected())
        {
            case 0: // Host
            {
                fslib::Directory workDir(config::get_working_directory());
                m_backup_options.clear();
                m_backup_display.clear();
                m_backup_map.clear();
                m_marked.clear();
                if (workDir.is_open())
                {
                    for (int t = 0; t < workDir.get_count(); t++)
                    {
                        const fslib::DirectoryEntry &titleEntry = workDir.get_entry(t);
                        if (!titleEntry.is_directory()) continue;
                        std::string titleName = titleEntry.get_filename();
                        fslib::Directory titleDir(config::get_working_directory() / titleName);
                        if (!titleDir.is_open()) continue;

                        // Read the backups first. An empty title folder (for example,
                        // a game JKSV knows about but has never backed up) must not
                        // appear as a selectable group in the send list.
                        std::vector<std::string> backups;
                        for (int b = 0; b < titleDir.get_count(); b++)
                        {
                            backups.push_back(titleDir.get_entry(b).get_filename());
                        }
                        if (backups.empty()) continue;

                        // Title header (non-selectable).
                        m_backup_display.push_back(titleName);
                        m_backup_map.push_back(-1);

                        for (const std::string &backupName : backups)
                        {
                            int idx = (int)m_backup_options.size();
                            m_backup_options.push_back(titleName + "/" + backupName);
                            m_backup_display.push_back("  " + backupName);
                            m_backup_map.push_back(idx);
                        }
                    }
                }

                if (m_backup_options.empty())
                {
                    const std::string msg = peer_string(18, "No backups found");
                    {
                        std::lock_guard<std::mutex> lk(m_status_mutex);
                        m_status = msg;
                    }
                    PeerState::push_event(ui::PopKind::Warning, msg);
                }
                else if (m_backup_options.size() == 1)
                {
                    // Preserve the single-backup shortcut.
                    PeerState::start_host({m_backup_options.front()});
                }
                else
                {
                    m_mode.store(Mode::Selecting);
                    PeerState::set_control_guide(peer_string(44, "A: Send  Y: Mark  B: Back"));
                    rebuild_menu();
                    {
                        std::lock_guard<std::mutex> lk(m_status_mutex);
                        m_status = stringutil::get_formatted_string(peer_string(55, "%d marked"), 0);
                    }
                }
            }
            break;
            case 1: // Join
                if (!m_worker.joinable())
                {
                    m_cancel.store(false);
                    m_busy.store(true);
                    {
                        std::lock_guard<std::mutex> lk(m_status_mutex);
                        m_status = peer_string(12, "Scanning for peers...");
                    }
                    m_worker = std::thread(&PeerState::do_join, this);
                }
                break;
            case 2:
                BaseState::deactivate();
                break;
        }
    }
    else if (bPressed) { BaseState::deactivate(); }
}

void PeerState::update_selecting()
{
    const int selected    = m_menu->get_selected();
    const int cancelIndex = static_cast<int>(m_backup_display.size());

    if (input::button_pressed(HidNpadButton_Y))
    {
        if (selected >= 0 && selected < cancelIndex)
        {
            const int backupIndex = m_backup_map[selected];
            if (backupIndex >= 0)
            {
                PeerState::toggle_mark(backupIndex);
                const std::string option = (is_marked(backupIndex) ? "[x] " : "[ ] ") + m_backup_display[selected];
                m_menu->edit_option(selected, option);
                {
                    std::lock_guard<std::mutex> lk(m_status_mutex);
                    m_status =
                        stringutil::get_formatted_string(peer_string(55, "%d marked"), static_cast<int>(m_marked.size()));
                }
            }
        }
        return;
    }

    if (input::button_pressed(HidNpadButton_A))
    {
        if (selected == cancelIndex || selected < 0)
        {
            m_marked.clear();
            m_mode.store(Mode::Menu);
            m_backup_options.clear();
            m_backup_display.clear();
            m_backup_map.clear();
            PeerState::set_control_guide(peer_string(41, "A: Select  B: Back"));
            rebuild_menu();
            return;
        }

        if (!m_marked.empty())
        {
            // Marks present: send every marked backup in display order.
            std::vector<std::string> selection;
            for (int d = 0; d < cancelIndex; d++)
            {
                const int backupIndex = m_backup_map[d];
                if (backupIndex >= 0 && is_marked(backupIndex)) { selection.push_back(m_backup_options[backupIndex]); }
            }
            if (!selection.empty()) { PeerState::start_host(std::move(selection)); }
            return;
        }

        const int backupIndex = m_backup_map[selected];
        if (backupIndex >= 0) { PeerState::start_host({m_backup_options[backupIndex]}); }
        return;
    }

    if (input::button_pressed(HidNpadButton_B))
    {
        m_marked.clear();
        m_mode.store(Mode::Menu);
        m_backup_options.clear();
        m_backup_display.clear();
        m_backup_map.clear();
        PeerState::set_control_guide(peer_string(41, "A: Select  B: Back"));
        rebuild_menu();
    }
}

void PeerState::sub_update()
{
    m_controlGuide->sub_update();

    if (m_worker.joinable() && !m_busy.load())
    {
        m_worker.join();
        PeerState::flush_pending_ip();
        PeerState::drain_events();
    }
}

void PeerState::render()
{
    const bool hasFocus = BaseState::has_focus();

    m_renderTarget->clear(colors::CLEAR_COLOR);
    m_menu->render(m_renderTarget, hasFocus);

    // Show this console's IP so the peer can connect manually. If the address
    // is unavailable, leave the line empty instead of showing English text.
    char ipstr[64] = {0};
    u32 ip         = 0;
    if (R_SUCCEEDED(nifmGetCurrentIpAddress(&ip)))
    {
        char ipbuf[32];
        snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
        const char *fmt = strings::get_by_name(strings::names::PEER, 35);
        if (fmt) snprintf(ipstr, sizeof(ipstr), fmt, ipbuf);
        else snprintf(ipstr, sizeof(ipstr), "My IP: %s", ipbuf);
    }

    std::string status;
    {
        std::lock_guard<std::mutex> lk(m_status_mutex);
        status = m_status;
    }
    // IP and status in the header area (top), clear of the menu.
    if (ipstr[0]) { sdl::text::render(sdl::Texture::Null, 320, 32, 20, sdl::text::NO_WRAP, colors::WHITE, ipstr); }
    if (!status.empty())
    {
        sdl::text::render(sdl::Texture::Null, 570, 32, 20, sdl::text::NO_WRAP, colors::WHITE, status.c_str());
    }

    m_renderTarget->render(sdl::Texture::Null, 201, 91);
    m_controlGuide->render(sdl::Texture::Null, hasFocus);
}

void PeerState::do_host()
{
    auto setStatus = [this](const std::string &s)
    {
        std::lock_guard<std::mutex> lk(m_status_mutex);
        m_status = s;
    };
    auto L = [&](int idx, const char *fb) -> const char *
    {
        const char *s = strings::get_by_name(strings::names::PEER, idx);
        return s ? s : fb;
    };
    auto report = [&](ui::PopKind kind, const std::string &text)
    {
        setStatus(text);
        PeerState::push_event(kind, text);
    };

    peer::LanTransport transport;
    transport.initialize();
    transport.set_name("JKSV Peer");

    if (!transport.host(peer::PEER_TCP_PORT))
    {
        report(ui::PopKind::Error, L(10, "Host failed to start"));
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    report(ui::PopKind::Info, L(9, "Waiting for a peer..."));

    // Poll in short slices so cancellation is checked frequently, but keep the
    // total wait bounded by the documented host timeout.
    bool accepted        = false;
    constexpr int kPolls = peer::PEER_HOST_WAIT_MS / peer::PEER_HOST_POLL_MS;
    for (int i = 0; i < kPolls && !m_cancel.load(); i++)
    {
        if (transport.accept_one(peer::PEER_HOST_POLL_MS))
        {
            accepted = true;
            break;
        }
    }

    if (m_cancel.load())
    {
        report(ui::PopKind::Warning, L(38, "Transfer cancelled"));
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    if (!accepted)
    {
        report(ui::PopKind::Warning, L(11, "No peer connected"));
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    // ---- Pairing handshake (3.4) ----
    uint8_t host_nonce[16];
    randomGet(host_nonce, sizeof(host_nonce));

    peer::Session session(transport);

    if (!session.send(peer::MessageType::Hello, host_nonce, sizeof(host_nonce)))
    {
        report(ui::PopKind::Error, L(39, "Connection lost"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    peer::Message msg;
    if (!session.recv(msg) || msg.type != peer::MessageType::Hello || msg.payload.size() < 16)
    {
        report(ui::PopKind::Error, L(5, "Pairing failed (no nonce)"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    // The code is a human confirmation that both consoles saw the same nonces.
    // It is not a secret, an encryption key, or an authentication mechanism.
    uint8_t combined[32];
    memcpy(combined, host_nonce, 16);
    memcpy(combined + 16, msg.payload.data(), 16);
    unsigned code = 0;
    for (int i = 0; i < 32; i++) code = (code * 31u + combined[i]) % 1000000u;

    char codebuf[64];
    const char *codefmt = strings::get_by_name(strings::names::PEER, 3);
    if (codefmt) snprintf(codebuf, sizeof(codebuf), codefmt, code);
    else snprintf(codebuf, sizeof(codebuf), "Pairing code: %06u - Press A on BOTH", code);
    setStatus(codebuf);

    m_confirm.store(false);
    for (int i = 0; i < 600 && !m_cancel.load() && !m_confirm.load(); i++) { svcSleepThread(100000000ULL); }

    if (m_cancel.load() || !m_confirm.load())
    {
        report(ui::PopKind::Warning, L(4, "Pairing cancelled"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    if (!session.send(peer::MessageType::Done, std::string("CONFIRMED")))
    {
        report(ui::PopKind::Error, L(39, "Connection lost"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    peer::Message confirm;
    if (!session.recv(confirm) || confirm.type != peer::MessageType::Done)
    {
        report(ui::PopKind::Error, L(6, "Pairing failed (no peer confirm)"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    report(ui::PopKind::Success, L(7, "Pairing OK"));

    // ---- Sending batch (3.6) ----
    const int total = static_cast<int>(m_send_list.size());
    bool cancelled  = false;
    bool failed     = false;

    for (int i = 0; i < total && !cancelled && !failed; i++)
    {
        const std::string selected    = m_send_list[i];
        const size_t slash            = selected.find('/');
        const std::string titleName   = (slash == std::string::npos) ? selected : selected.substr(0, slash);
        const std::string backupName  = (slash == std::string::npos) ? "" : selected.substr(slash + 1);
        const std::string displayName = backupName.empty() ? selected : backupName;

        setStatus(stringutil::get_formatted_string(L(45, "Sending %d/%d: %s"), i + 1, total, displayName.c_str()));

        fslib::Path sourcePath = config::get_working_directory() / titleName;
        if (!backupName.empty()) sourcePath = sourcePath / backupName;

        fslib::Path streamPath = sourcePath;
        const bool needZip     = fslib::directory_exists(sourcePath);

        // Announce before compressing. Folder backups are streamed as ZIP data,
        // so announce a .zip name; otherwise the receiver would save a zip with
        // a folder's name.
        std::string listSelected = selected;
        if (!backupName.empty())
        {
            std::string receiveName = backupName;
            if (needZip && (receiveName.size() < 4 || receiveName.compare(receiveName.size() - 4, 4, ".zip") != 0))
            {
                receiveName += ".zip";
            }
            listSelected = titleName + "/" + receiveName;
        }
        const std::string listPayload = listSelected + "\n" + std::to_string(i + 1) + "/" + std::to_string(total);
        if (!session.send(peer::MessageType::List, listPayload))
        {
            report(ui::PopKind::Error, L(39, "Connection lost"));
            failed = true;
            break;
        }

        if (needZip)
        {
            fslib::Path zipPath = make_temp_zip_path();
            fs::MiniZip zip(zipPath);
            if (!zip.is_open())
            {
                session.send(peer::MessageType::Error, std::string("ZIP_FAILED"));
                report(ui::PopKind::Error, L(28, "Failed to create zip"));
                failed = true;
                break;
            }

            // Compressing a large save can take longer than the socket
            // timeout. Send a Busy keepalive every five seconds while the
            // archive is prepared so the receiver keeps the session alive.
            std::atomic<bool> keepalive{true};
            std::atomic<bool> keepaliveFailed{false};
            std::thread ping(
                [&]()
                {
                    for (int tick = 0; tick < 50 && keepalive.load(); tick++) { svcSleepThread(100000000ULL); }
                    while (keepalive.load())
                    {
                        if (!session.send(peer::MessageType::Busy, std::string()))
                        {
                            keepaliveFailed.store(true);
                            break;
                        }
                        for (int tick = 0; tick < 50 && keepalive.load(); tick++) { svcSleepThread(100000000ULL); }
                    }
                });

            // Archive the folder contents relative to its root, not the full
            // SD-card path, so the received ZIP restores like a normal backup.
            zip.set_root_prefix(sourcePath.string());
            fs::copy_directory_to_zip(sourcePath, zip, nullptr);
            zip.close();
            keepalive.store(false);
            ping.join();

            if (keepaliveFailed.load())
            {
                fslib::delete_file(zipPath);
                report(ui::PopKind::Error, L(39, "Connection lost"));
                failed = true;
                break;
            }
            streamPath = zipPath;
        }

        fslib::File streamFile(streamPath, FsOpenMode_Read);
        if (!streamFile.is_open())
        {
            session.send(peer::MessageType::Error, std::string("READ_FAILED"));
            if (needZip) fslib::delete_file(streamPath);
            report(ui::PopKind::Error, L(29, "Failed to read source"));
            failed = true;
            break;
        }
        const int64_t fileSize = streamFile.get_size();

        char chunk[4096];
        int64_t sent    = 0;
        int chunks      = 0;
        bool readFailed = false;
        bool sendFailed = false;
        while (sent < fileSize)
        {
            if (m_cancel.load())
            {
                cancelled = true;
                break;
            }

            const size_t toRead = static_cast<size_t>(std::min<int64_t>(4096, fileSize - sent));
            const ssize_t r     = streamFile.read(chunk, toRead);
            if (r <= 0)
            {
                readFailed = true;
                break;
            }
            if (!session.send(peer::MessageType::Data, chunk, static_cast<size_t>(r)))
            {
                sendFailed = true;
                break;
            }
            sent += r;
            if (++chunks % 8 == 0)
            {
                std::lock_guard<std::mutex> lk(m_status_mutex);
                m_status = stringutil::get_formatted_string(L(20, "Sending: %lld / %lld bytes"),
                                                            static_cast<long long>(sent),
                                                            static_cast<long long>(fileSize));
            }
        }
        streamFile.close();
        if (needZip) fslib::delete_file(streamPath);

        if (cancelled)
        {
            session.send(peer::MessageType::Error, std::string("CANCELLED"));
            report(ui::PopKind::Warning, L(38, "Transfer cancelled"));
            break;
        }
        if (readFailed)
        {
            session.send(peer::MessageType::Error, std::string("READ_FAILED"));
            report(ui::PopKind::Error, L(29, "Failed to read source"));
            failed = true;
            break;
        }
        if (sendFailed)
        {
            report(ui::PopKind::Error, L(39, "Connection lost"));
            failed = true;
            break;
        }

        // Announce the per-file size so the receiver can verify this file.
        if (!session.send(peer::MessageType::Done, std::to_string(fileSize)))
        {
            report(ui::PopKind::Error, L(39, "Connection lost"));
            failed = true;
            break;
        }
    }

    if (cancelled || failed)
    {
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    // Signal the end of the batch. Only report success if the receiver can
    // also see the end marker.
    if (!session.send(peer::MessageType::End, std::to_string(total)))
    {
        report(ui::PopKind::Error, L(39, "Connection lost"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }
    report(ui::PopKind::Success, stringutil::get_formatted_string(L(56, "Sent %d backup(s)"), total));
    transport.disconnect();
    transport.shutdown();
    m_busy.store(false);
}

void PeerState::do_join()
{
    auto setStatus = [this](const std::string &s)
    {
        std::lock_guard<std::mutex> lk(m_status_mutex);
        m_status = s;
    };
    auto L = [&](int idx, const char *fb) -> const char *
    {
        const char *s = strings::get_by_name(strings::names::PEER, idx);
        return s ? s : fb;
    };
    auto report = [&](ui::PopKind kind, const std::string &text)
    {
        setStatus(text);
        PeerState::push_event(kind, text);
    };
    auto queueLastIp = [this](const std::string &ip)
    {
        if (ip.empty()) { return; }
        std::lock_guard<std::mutex> lk(m_status_mutex);
        m_pending_last_ip = ip;
    };

    peer::LanTransport transport;
    transport.initialize();
    transport.set_name("JKSV Peer");

    setStatus(L(12, "Scanning for peers..."));

    std::vector<peer::PeerInfo> peers;
    transport.discover(peers, peer::PEER_DISCOVERY_WAIT_MS, &m_cancel);

    if (m_cancel.load())
    {
        report(ui::PopKind::Warning, L(38, "Transfer cancelled"));
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    if (peers.empty())
    {
        // Broadcast can be blocked by AP isolation or a different subnet. Fall
        // back to a manual IPv4 address and remember it for the next attempt.
        setStatus(L(13, "No peers found via broadcast"));
        char ipbuf[64]                   = {0};
        const std::string &lastIp        = config::get_peer_last_ip();
        const std::string_view defaultIp = lastIp.empty() ? std::string_view{"192.168.1.1"} : std::string_view{lastIp};
        bool ok = keyboard::get_input(SwkbdType_QWERTY, defaultIp, L(57, "Enter host IP address"), ipbuf, sizeof(ipbuf));
        if (!ok || ipbuf[0] == '\0')
        {
            report(ui::PopKind::Warning, L(14, "Cancelled"));
            transport.shutdown();
            m_busy.store(false);
            return;
        }
        peer::PeerInfo manual;
        manual.name    = "manual";
        manual.address = ipbuf;
        manual.port    = peer::PEER_TCP_PORT;
        peers.push_back(manual);
        queueLastIp(ipbuf);
    }

    if (m_cancel.load())
    {
        report(ui::PopKind::Warning, L(38, "Transfer cancelled"));
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    setStatus(L(15, "Connecting to peer..."));

    if (!transport.connect(peers.front(), &m_cancel))
    {
        if (m_cancel.load()) { report(ui::PopKind::Warning, L(38, "Transfer cancelled")); }
        else { report(ui::PopKind::Error, L(16, "Connect failed")); }
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    queueLastIp(peers.front().address);

    // ---- Pairing handshake (3.4) ----
    peer::Session session(transport);
    peer::Message host_msg;
    if (!session.recv(host_msg) || host_msg.type != peer::MessageType::Hello || host_msg.payload.size() < 16)
    {
        report(ui::PopKind::Error, L(5, "Pairing failed (no host nonce)"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    uint8_t join_nonce[16];
    randomGet(join_nonce, sizeof(join_nonce));
    if (!session.send(peer::MessageType::Hello, join_nonce, sizeof(join_nonce)))
    {
        report(ui::PopKind::Error, L(39, "Connection lost"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    // Same derivation as the host: host nonce first, then the joining nonce.
    uint8_t combined[32];
    memcpy(combined, host_msg.payload.data(), 16);
    memcpy(combined + 16, join_nonce, 16);
    unsigned code = 0;
    for (int i = 0; i < 32; i++) code = (code * 31u + combined[i]) % 1000000u;

    char codebuf[64];
    const char *codefmt = strings::get_by_name(strings::names::PEER, 3);
    if (codefmt) snprintf(codebuf, sizeof(codebuf), codefmt, code);
    else snprintf(codebuf, sizeof(codebuf), "Pairing code: %06u - Press A on BOTH", code);
    setStatus(codebuf);

    m_confirm.store(false);
    for (int i = 0; i < 600 && !m_cancel.load() && !m_confirm.load(); i++) { svcSleepThread(100000000ULL); }

    if (m_cancel.load() || !m_confirm.load())
    {
        report(ui::PopKind::Warning, L(4, "Pairing cancelled"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    if (!session.send(peer::MessageType::Done, std::string("CONFIRMED")))
    {
        report(ui::PopKind::Error, L(39, "Connection lost"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    peer::Message confirm;
    if (!session.recv(confirm) || confirm.type != peer::MessageType::Done)
    {
        report(ui::PopKind::Error, L(6, "Pairing failed (no host confirm)"));
        transport.disconnect();
        transport.shutdown();
        m_busy.store(false);
        return;
    }

    report(ui::PopKind::Success, L(7, "Pairing OK"));

    // ---- Receiving batch (3.5/3.6/3.7) ----
    std::vector<peer::ReceivedBackup> received;
    bool batchDone  = false;
    bool batchError = false;

    fslib::File outFile;
    fslib::Path partPath;
    fslib::Path finalPath;
    std::string currentTitle;
    int64_t receivedBytes = 0;
    int64_t expectedBytes = -1;
    bool inFlight         = false;

    // Cancellation, disconnect, or a malformed batch can leave a .part file.
    // Always clean it up before returning to the menu.
    auto discardInFlight = [&]()
    {
        if (outFile.is_open()) outFile.close();
        if (inFlight && fslib::file_exists(partPath)) { fslib::delete_file(partPath); }
        inFlight      = false;
        receivedBytes = 0;
        expectedBytes = -1;
    };

    while (!m_cancel.load() && !batchDone && !batchError)
    {
        peer::Message inMsg;
        if (!session.recv(inMsg))
        {
            report(ui::PopKind::Error, L(39, "Connection lost"));
            batchError = true;
            break;
        }

        // The sender compresses folder backups after announcing them. Busy
        // messages keep the socket alive until the first Data frame arrives.
        if (inMsg.type == peer::MessageType::Busy) { continue; }

        if (inMsg.type == peer::MessageType::List)
        {
            discardInFlight();

            std::string listStr(inMsg.payload.begin(), inMsg.payload.end());
            std::string selected = listStr;
            std::string progress;
            const size_t nl = listStr.find('\n');
            if (nl != std::string::npos)
            {
                selected = listStr.substr(0, nl);
                progress = listStr.substr(nl + 1);
            }

            if (selected.empty() || selected == "(no backups)")
            {
                report(ui::PopKind::Warning, L(27, "Sender has no backups"));
                batchError = true;
                break;
            }

            int index       = 1;
            int count       = 1;
            const size_t ps = progress.find('/');
            if (ps != std::string::npos)
            {
                index = std::atoi(progress.substr(0, ps).c_str());
                count = std::atoi(progress.substr(ps + 1).c_str());
            }
            if (count < 1) count = 1;

            const size_t slash           = selected.find('/');
            const std::string titleName  = (slash == std::string::npos) ? selected : selected.substr(0, slash);
            const std::string backupName = (slash == std::string::npos) ? "peer_received" : selected.substr(slash + 1);
            currentTitle                 = titleName;

            // Never let a peer escape the backup directory with . or ..
            // components or an absolute/backslash path.
            const auto safeComponent = [](const std::string &value)
            {
                return !value.empty() && value != "." && value != ".." && value.find('/') == std::string::npos &&
                       value.find('\\') == std::string::npos && value.find('\0') == std::string::npos;
            };
            if (!safeComponent(titleName) || !safeComponent(backupName))
            {
                report(ui::PopKind::Error, L(26, "No backup info received"));
                batchError = true;
                break;
            }

            fslib::Path titleDir = config::get_working_directory() / titleName;
            if (!fslib::directory_exists(titleDir)) fslib::create_directory(titleDir);

            finalPath = unique_target_path(titleDir, backupName);

            // Write to a sibling .part file and rename only after the size check
            // succeeds, so an interrupted transfer never leaves a partial backup.
            partPath = finalPath;
            partPath += ".part";
            if (fslib::file_exists(partPath)) { fslib::delete_file(partPath); }

            outFile = fslib::File(partPath, FsOpenMode_Create | FsOpenMode_Write);
            if (!outFile.is_open())
            {
                report(ui::PopKind::Error, L(30, "Failed to create output file"));
                batchError = true;
                break;
            }

            receivedBytes = 0;
            expectedBytes = -1;
            inFlight      = true;
            setStatus(stringutil::get_formatted_string(L(46, "Receiving %d/%d: %s"), index, count, backupName.c_str()));
            continue;
        }

        if (inMsg.type == peer::MessageType::Data)
        {
            if (!inFlight) continue;

            const ssize_t written = outFile.write(inMsg.payload.data(), inMsg.payload.size());
            if (written < 0 || static_cast<size_t>(written) != inMsg.payload.size())
            {
                report(ui::PopKind::Error, L(30, "Failed to create output file"));
                batchError = true;
                break;
            }
            receivedBytes += static_cast<int64_t>(inMsg.payload.size());
            if (receivedBytes % (4096 * 8) < 4096)
            {
                std::lock_guard<std::mutex> lk(m_status_mutex);
                m_status =
                    stringutil::get_formatted_string(L(23, "Receiving: %lld bytes"), static_cast<long long>(receivedBytes));
            }
            continue;
        }

        if (inMsg.type == peer::MessageType::Done)
        {
            if (!inFlight) continue;

            // The sender announces the exact byte count. Reject a short or long
            // file before it can replace a valid backup.
            outFile.close();
            std::string sizeStr(inMsg.payload.begin(), inMsg.payload.end());
            if (!sizeStr.empty())
            {
                char *end              = nullptr;
                const long long parsed = strtoll(sizeStr.c_str(), &end, 10);
                if (end && *end == '\0' && parsed >= 0) { expectedBytes = static_cast<int64_t>(parsed); }
            }

            if (expectedBytes < 0 || receivedBytes != expectedBytes)
            {
                if (fslib::file_exists(partPath)) { fslib::delete_file(partPath); }
                inFlight = false;
                report(ui::PopKind::Error, L(25, "Receive incomplete"));
                batchError = true;
                break;
            }

            if (!fslib::rename_file(partPath, finalPath))
            {
                if (fslib::file_exists(partPath)) { fslib::delete_file(partPath); }
                inFlight = false;
                report(ui::PopKind::Error, L(30, "Failed to create output file"));
                batchError = true;
                break;
            }

            peer::ReceivedBackup backup;
            backup.path          = finalPath;
            backup.folderName    = currentTitle;
            backup.metadataValid = fs::read_save_meta(finalPath, backup.metadata);
            backup.applicationID = backup.metadataValid ? backup.metadata.applicationID : 0;
            received.push_back(std::move(backup));

            inFlight      = false;
            receivedBytes = 0;
            expectedBytes = -1;
            continue;
        }

        if (inMsg.type == peer::MessageType::End)
        {
            batchDone = true;
            continue;
        }

        if (inMsg.type == peer::MessageType::Error)
        {
            std::string err(inMsg.payload.begin(), inMsg.payload.end());
            report(ui::PopKind::Error, stringutil::get_formatted_string(L(37, "Error: %s"), err.c_str()));
            batchError = true;
            break;
        }
    }

    discardInFlight();

    if (batchDone && !m_cancel.load())
    {
        // The post-receive screen appears immediately, so show a pop as well:
        // a fast transfer otherwise looks like nothing happened after pairing.
        const std::string doneMessage =
            stringutil::get_formatted_string(L(47, "Batch complete: %d received"), static_cast<int>(received.size()));
        report(ui::PopKind::Success, doneMessage);
        {
            std::lock_guard<std::mutex> lk(m_status_mutex);
            m_received_backups = std::move(received);
        }
        m_batch_pending.store(true);
    }
    else if (m_cancel.load()) { report(ui::PopKind::Warning, L(38, "Transfer cancelled")); }
    else if (!batchError) { report(ui::PopKind::Error, L(25, "Receive incomplete")); }

    transport.disconnect();
    transport.shutdown();
    m_busy.store(false);
}

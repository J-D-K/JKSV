#pragma once

#include "peer/Transport.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

namespace peer
{
/// @brief LAN transport: UDP broadcast discovery + TCP transfer (D5).
/// Works in any applet/application mode because it only uses the already
/// initialized socket/nifm services.
class LanTransport final : public Transport
{
public:
    /// @brief Constructs an idle transport; call initialize() before use.
    LanTransport() = default;

    /// @brief Closes sockets and stops the discovery thread.
    ~LanTransport() override;

    /// @brief Prepares socket services and ignores SIGPIPE.
    /// @return True on success.
    bool initialize() override;

    /// @brief Closes the session, listener, and discovery thread.
    void shutdown() override;

    /// @brief Starts the TCP listener and discovery beacon.
    /// @param port TCP port to listen on.
    /// @return True when the listener was created.
    bool host(std::uint16_t port) override;

    /// @brief Non-cancellable discovery wrapper.
    /// @param out Discovered peers.
    /// @param timeout_ms Overall discovery budget.
    /// @return True when the scan ran.
    bool discover(std::vector<PeerInfo> &out, std::uint32_t timeout_ms) override;

    /// @brief Non-cancellable connect wrapper.
    /// @param peer Peer to connect to.
    /// @return True on success.
    bool connect(const PeerInfo &peer) override;

    /// @brief Sends bytes on the active connection.
    /// @param data Buffer to send.
    /// @param len Number of bytes.
    /// @return True when every byte was sent.
    bool send(const void *data, std::size_t len) override;

    /// @brief Receives bytes from the active connection.
    /// @param data Destination buffer.
    /// @param len Maximum bytes to read.
    /// @return Bytes read, 0 on close, or -1 on timeout/error.
    int recv(void *data, std::size_t len) override;

    /// @brief Closes the active connection and stops discovery broadcasting.
    void disconnect() override;

    /// @brief Returns whether a TCP connection is active.
    bool is_connected() const override;

    /// @brief Cancellable discovery. Stops early when @p cancel becomes true.
    /// @param out Discovered peers.
    /// @param timeout_ms Overall discovery budget.
    /// @param cancel Optional cancel flag polled while scanning.
    /// @return True when the scan ran (even if nothing was found).
    bool discover(std::vector<PeerInfo> &out, std::uint32_t timeout_ms, const std::atomic<bool> *cancel);

    /// @brief Cancellable, bounded connect. Aborts when @p cancel becomes true
    ///        or after a fixed timeout, so it never blocks the UI indefinitely.
    /// @param peer Peer to connect to.
    /// @param cancel Optional cancel flag polled while connecting.
    /// @return True on success.
    bool connect(const PeerInfo &peer, const std::atomic<bool> *cancel);

    /// @brief Display name advertised in the discovery beacon.
    void set_name(const std::string &name) { m_name = name; }

    /// @brief (Host only) Accept one incoming connection, blocking up to
    ///        @p timeout_ms. Returns true on success.
    bool accept_one(int timeout_ms = 30000);

private:
    std::string m_name{"JKSV"};
    int m_listen_fd{-1};
    int m_conn_fd{-1};
    std::uint16_t m_port{0};
    bool m_initialized{false};

    std::atomic<bool> m_broadcasting{false};
    std::thread m_broadcast_thread{};
};
} // namespace peer
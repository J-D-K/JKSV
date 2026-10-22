#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace peer
{
/// @brief Information about a discovered peer.
struct PeerInfo
{
    std::string name;     ///< Display name of the peer.
    std::string address;  ///< IPv4 address of the peer.
    std::uint16_t port{}; ///< TCP port the peer is listening on.
};

/// @brief Transport-agnostic interface for a peer session (D1).
/// The shipping implementation is LanTransport; the LDN spike is not built.
class Transport
{
public:
    virtual ~Transport() = default;

    /// @brief Initialize the underlying services (sockets/nifm/ldn).
    virtual bool initialize() = 0;

    /// @brief Release the underlying services.
    virtual void shutdown() = 0;

    /// @brief Start listening for incoming connections on @p port.
    virtual bool host(std::uint16_t port) = 0;

    /// @brief Discover peers on the network (blocking for up to @p timeout_ms).
    virtual bool discover(std::vector<PeerInfo> &out, std::uint32_t timeout_ms) = 0;

    /// @brief Connect to a previously discovered peer.
    virtual bool connect(const PeerInfo &peer) = 0;

    /// @brief Send @p len bytes. Returns true on full send.
    virtual bool send(const void *data, std::size_t len) = 0;

    /// @brief Receive up to @p len bytes. Returns bytes received, 0 on
    ///        clean close, -1 on error.
    virtual int recv(void *data, std::size_t len) = 0;

    /// @brief Close the current connection.
    virtual void disconnect() = 0;

    /// @brief Whether a peer connection is currently established.
    virtual bool is_connected() const = 0;
};
} // namespace peer
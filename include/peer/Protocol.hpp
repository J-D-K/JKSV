#pragma once

#include <cstdint>

namespace peer
{
/// @brief TCP port used for the peer session.
inline constexpr std::uint16_t PEER_TCP_PORT = 55555;

/// @brief UDP port used for discovery beacons.
inline constexpr std::uint16_t PEER_DISCOVERY_PORT = 55556;

/// @brief Total time the host waits for a peer before reporting a timeout.
inline constexpr int PEER_HOST_WAIT_MS = 30000;

/// @brief Poll interval used while waiting for a peer.
inline constexpr int PEER_HOST_POLL_MS = 100;

/// @brief Time spent listening for discovery beacons before falling back to a manual address.
inline constexpr int PEER_DISCOVERY_WAIT_MS = 3000;
} // namespace peer

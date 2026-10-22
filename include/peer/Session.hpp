#pragma once

#include "peer/Transport.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace peer
{
/// @brief Message types exchanged over a peer session (D3).
enum class MessageType : std::uint32_t
{
    Hello = 1, ///< Pairing nonce exchange.
    List  = 2, ///< Announces the next backup and its progress label.
    Get   = 3, ///< Reserved for receiver-initiated requests.
    Data  = 4, ///< Raw file chunk.
    Done  = 5, ///< Per-file completion or pairing confirmation.
    Error = 6, ///< Human-readable error code.
    End   = 7, ///< End of a batch.
    Busy  = 8, ///< Keepalive sent while a folder backup is compressed.
};

/// @brief A single framed message.
struct Message
{
    MessageType type{MessageType::Hello}; ///< Message kind.
    std::vector<std::uint8_t> payload;    ///< Message body.
};

/// @brief Wraps a Transport with length-prefixed message framing.
///
/// Every message starts with two native-endian 32-bit words: the message type
/// and the payload length. The payload follows immediately. Both consoles use
/// the same architecture, so no byte-order conversion is performed.
class Session
{
public:
    /// @brief Largest payload accepted in a single message (1 MiB).
    static constexpr std::size_t MAX_PAYLOAD = 1024 * 1024;

    /// @brief Constructs a session around an already connected transport.
    /// @param transport Transport that owns the connection.
    explicit Session(Transport &transport)
        : m_transport(transport)
    {
    }

    /// @brief Sends one framed message.
    /// @param type Message kind.
    /// @param data Payload bytes, or nullptr when @p len is zero.
    /// @param len Payload length; must not exceed MAX_PAYLOAD.
    /// @return True when the whole frame was written.
    bool send(MessageType type, const void *data, std::size_t len);

    /// @brief Sends one framed message with a string payload.
    /// @param type Message kind.
    /// @param text Payload text.
    /// @return True when the whole frame was written.
    bool send(MessageType type, const std::string &text);

    /// @brief Receives one framed message.
    /// @param out Destination message; cleared before reading.
    /// @return False on disconnect, a malformed header, an unknown type, or an
    ///         oversized payload.
    bool recv(Message &out);

    /// @brief Returns the underlying transport.
    Transport &transport() { return m_transport; }

private:
    Transport &m_transport; ///< Connected transport.
};
} // namespace peer
#include "peer/Session.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// Session framing:
//   [uint32 type][uint32 payload length][payload bytes]
//
// Both consoles run the same architecture, so the header is written in native
// byte order. Every payload is capped at Session::MAX_PAYLOAD and every type is
// validated on receive so a corrupt or hostile peer cannot force an unbounded
// allocation or inject an unknown message.
namespace peer
{
namespace
{
bool isValidType(std::uint32_t raw)
{
    switch (static_cast<MessageType>(raw))
    {
        case MessageType::Hello:
        case MessageType::List:
        case MessageType::Get:
        case MessageType::Data:
        case MessageType::Done:
        case MessageType::Error:
        case MessageType::End:
        case MessageType::Busy:
            return true;
        default:
            return false;
    }
}

bool sendAll(Transport &t, const void *data, std::size_t len)
{
    const std::uint8_t *p = static_cast<const std::uint8_t *>(data);
    std::size_t off       = 0;
    while (off < len)
    {
        // Transport::send returns bool for full send; loop in chunks.
        std::size_t chunk = len - off;
        if (chunk > 4096) chunk = 4096;
        if (!t.send(p + off, chunk)) return false;
        off += chunk;
    }
    return true;
}

bool recvAll(Transport &t, void *data, std::size_t len)
{
    std::uint8_t *p = static_cast<std::uint8_t *>(data);
    std::size_t off = 0;
    while (off < len)
    {
        int n = t.recv(p + off, len - off);
        if (n <= 0) return false;
        off += static_cast<std::size_t>(n);
    }
    return true;
}
} // namespace

bool Session::send(MessageType type, const void *data, std::size_t len)
{
    if (len > MAX_PAYLOAD) return false;

    std::uint32_t header[2];
    header[0] = static_cast<std::uint32_t>(type);
    header[1] = static_cast<std::uint32_t>(len);
    if (!sendAll(m_transport, header, sizeof(header))) return false;
    if (len > 0 && !sendAll(m_transport, data, len)) return false;
    return true;
}

bool Session::send(MessageType type, const std::string &text) { return send(type, text.data(), text.size()); }

bool Session::recv(Message &out)
{
    out.payload.clear();

    std::uint32_t header[2];
    if (!recvAll(m_transport, header, sizeof(header))) return false;
    if (!isValidType(header[0])) return false;
    if (header[1] > MAX_PAYLOAD) return false;

    out.type = static_cast<MessageType>(header[0]);
    out.payload.resize(header[1]);
    if (header[1] > 0 && !recvAll(m_transport, out.payload.data(), header[1]))
    {
        out.payload.clear();
        return false;
    }
    return true;
}
} // namespace peer
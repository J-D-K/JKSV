#include "peer/LanTransport.hpp"
#include "peer/Protocol.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

namespace peer
{
namespace
{
constexpr const char *kMagic = "JKSV";

#ifdef MSG_NOSIGNAL
constexpr int kSendFlags = MSG_NOSIGNAL;
#else
constexpr int kSendFlags = 0;
#endif

/// @brief Wire layout of the UDP discovery packet.
/// The port is stored in host byte order because both consoles use the same
/// architecture. The packet is sent as-is; see docs/PEER_PROTOCOL.md.
struct Beacon
{
    char magic[4];
    std::uint16_t tcp_port;
    char name[32];
};

bool sendAllFd(int fd, const void *buf, std::size_t len)
{
    const char *p   = static_cast<const char *>(buf);
    std::size_t off = 0;
    while (off < len)
    {
        ssize_t n = send(fd, p + off, len - off, kSendFlags);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        off += static_cast<std::size_t>(n);
    }
    return true;
}
} // namespace

LanTransport::~LanTransport() { shutdown(); }

bool LanTransport::initialize()
{
    // A peer can close the socket at any time. Ignore SIGPIPE so send() reports
    // EPIPE instead of terminating the process; MSG_NOSIGNAL is used when the
    // toolchain exposes it.
    signal(SIGPIPE, SIG_IGN);
    m_initialized = true;
    return true;
}

void LanTransport::shutdown()
{
    disconnect();
    if (m_listen_fd >= 0)
    {
        close(m_listen_fd);
        m_listen_fd = -1;
    }
    m_broadcasting.store(false);
    if (m_broadcast_thread.joinable()) m_broadcast_thread.join();
    m_initialized = false;
}

bool LanTransport::host(std::uint16_t port)
{
    if (!m_initialized) return false;

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) return false;

    int on = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in sa{};
    sa.sin_family      = AF_INET;
    sa.sin_port        = __builtin_bswap16(port);
    sa.sin_addr.s_addr = 0; // INADDR_ANY
    if (bind(srv, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
    {
        close(srv);
        return false;
    }
    if (listen(srv, 4) < 0)
    {
        close(srv);
        return false;
    }

    m_listen_fd = srv;
    m_port      = port;

    // Broadcast a discovery beacon so peers can find us.
    m_broadcasting.store(true);
    std::string name       = m_name;
    std::uint16_t tcp_port = port;
    m_broadcast_thread     = std::thread(
        [this, name, tcp_port]()
        {
            int bsock = socket(AF_INET, SOCK_DGRAM, 0);
            if (bsock < 0) return;
            int onb = 1;
            setsockopt(bsock, SOL_SOCKET, SO_BROADCAST, &onb, sizeof(onb));
            // Bind to INADDR_ANY so the socket is properly attached.
            struct sockaddr_in baddr{};
            baddr.sin_family      = AF_INET;
            baddr.sin_port        = 0;
            baddr.sin_addr.s_addr = 0;
            bind(bsock, reinterpret_cast<struct sockaddr *>(&baddr), sizeof(baddr));

            struct sockaddr_in dst{};
            dst.sin_family      = AF_INET;
            dst.sin_port        = __builtin_bswap16(PEER_DISCOVERY_PORT);
            dst.sin_addr.s_addr = htonl(INADDR_BROADCAST);

            Beacon beacon{};
            std::memcpy(beacon.magic, kMagic, 4);
            beacon.tcp_port = tcp_port;
            std::strncpy(beacon.name, name.c_str(), sizeof(beacon.name) - 1);

            while (m_broadcasting.load())
            {
                sendto(bsock, &beacon, sizeof(beacon), 0, reinterpret_cast<struct sockaddr *>(&dst), sizeof(dst));
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            close(bsock);
        });

    return true;
}

bool LanTransport::discover(std::vector<PeerInfo> &out, std::uint32_t timeout_ms) { return discover(out, timeout_ms, nullptr); }

bool LanTransport::discover(std::vector<PeerInfo> &out, std::uint32_t timeout_ms, const std::atomic<bool> *cancel)
{
    out.clear();
    if (!m_initialized) return false;

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return false;

    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in bind_addr{};
    bind_addr.sin_family      = AF_INET;
    bind_addr.sin_port        = __builtin_bswap16(PEER_DISCOVERY_PORT);
    bind_addr.sin_addr.s_addr = 0;
    if (bind(s, reinterpret_cast<struct sockaddr *>(&bind_addr), sizeof(bind_addr)) < 0)
    {
        close(s);
        return false;
    }

    // Receive timeout.
    struct timeval tv{};
    tv.tv_sec  = 0;
    tv.tv_usec = 500000; // 500 ms per recvfrom
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (cancel && cancel->load()) break;

        Beacon beacon{};
        struct sockaddr_in from{};
        socklen_t flen = sizeof(from);
        ssize_t n      = recvfrom(s, &beacon, sizeof(beacon), 0, reinterpret_cast<struct sockaddr *>(&from), &flen);
        if (n < 0) continue;
        if (n < (ssize_t)sizeof(beacon)) continue;
        if (std::memcmp(beacon.magic, kMagic, 4) != 0) continue;

        PeerInfo info;
        size_t nameLen = 0;
        while (nameLen < sizeof(beacon.name) && beacon.name[nameLen] != '\0') { ++nameLen; }
        info.name.assign(beacon.name, nameLen);
        char ipbuf[32];
        inet_ntop(AF_INET, &from.sin_addr, ipbuf, sizeof(ipbuf));
        info.address = ipbuf;
        info.port    = beacon.tcp_port;
        // de-dupe by address
        bool dup = false;
        for (const auto &p : out)
            if (p.address == info.address)
            {
                dup = true;
                break;
            }
        if (!dup) out.push_back(info);
    }

    close(s);
    return true;
}

bool LanTransport::connect(const PeerInfo &peer) { return connect(peer, nullptr); }

bool LanTransport::connect(const PeerInfo &peer, const std::atomic<bool> *cancel)
{
    if (!m_initialized) return false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port   = __builtin_bswap16(peer.port);
    if (inet_pton(AF_INET, peer.address.c_str(), &sa.sin_addr) != 1)
    {
        close(fd);
        return false;
    }

    // Non-blocking connect with a bounded, cancellable wait so a wrong address
    // can never trap the caller (the UI thread joins this worker on cancel/exit).
    const int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    bool connected = false;
    const int rc   = ::connect(fd, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa));
    if (rc == 0) { connected = true; }
    else if (errno != EINPROGRESS)
    {
        close(fd);
        return false;
    }
    else
    {
        constexpr int CONNECT_TIMEOUT_MS = 10000;
        const auto deadline              = std::chrono::steady_clock::now() + std::chrono::milliseconds(CONNECT_TIMEOUT_MS);

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (cancel && cancel->load())
            {
                close(fd);
                return false;
            }

            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(fd, &wfds);

            struct timeval tv{};
            tv.tv_sec  = 0;
            tv.tv_usec = 100000; // 100 ms so cancel is polled frequently.

            const int ready = select(fd + 1, nullptr, &wfds, nullptr, &tv);
            if (ready < 0)
            {
                if (errno == EINTR) { continue; }
                break;
            }
            if (ready == 0) { continue; }

            int err       = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
            {
                close(fd);
                return false;
            }

            connected = true;
            break;
        }

        if (!connected)
        {
            close(fd);
            return false;
        }
    }

    // Restore blocking mode for the transfer.
    fcntl(fd, F_SETFL, flags);

    m_conn_fd = fd;
    // Set a receive timeout so recv() doesn't block forever.
    struct timeval tv{};
    tv.tv_sec = 10;
    setsockopt(m_conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return true;
}

bool LanTransport::send(const void *data, std::size_t len)
{
    if (m_conn_fd < 0) return false;
    return sendAllFd(m_conn_fd, data, len);
}

int LanTransport::recv(void *data, std::size_t len)
{
    if (m_conn_fd < 0) return -1;

    ssize_t n = -1;
    do { n = ::recv(m_conn_fd, data, len, 0); } while (n < 0 && errno == EINTR);
    return static_cast<int>(n);
}

void LanTransport::disconnect()
{
    if (m_conn_fd >= 0)
    {
        close(m_conn_fd);
        m_conn_fd = -1;
    }
    m_broadcasting.store(false);
    if (m_broadcast_thread.joinable()) m_broadcast_thread.join();
    // The listening socket stays open until shutdown() so a host can accept
    // another peer after a completed transfer.
}

bool LanTransport::accept_one(int timeout_ms)
{
    if (m_listen_fd < 0) return false;
    if (timeout_ms < 0) timeout_ms = 0;

    // Wait for the requested interval, then accept without blocking.
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_listen_fd, &rfds);
    struct timeval tv{};
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int ready  = select(m_listen_fd + 1, &rfds, nullptr, nullptr, &tv);
    if (ready < 0) return false;
    if (ready == 0) return false;

    struct sockaddr_in peer{};
    socklen_t plen = sizeof(peer);
    int cl         = accept(m_listen_fd, reinterpret_cast<struct sockaddr *>(&peer), &plen);
    if (cl < 0) return false;
    m_conn_fd = cl;

    struct timeval rtv{};
    rtv.tv_sec = 10;
    setsockopt(m_conn_fd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv));
    return true;
}

bool LanTransport::is_connected() const { return m_conn_fd >= 0; }
} // namespace peer
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_SOCKET_WITH_SONY_SOCKET
#include "platform_socket.h"
#include "socket/platform/receive_flags.h"
#include "socket/platform/socket_error.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

#include "netinet/in.h"
#include <libnetctl.h>
#include <net.h>
#include <utility>

namespace reactormq::socket
{
    bool PlatformSocket::createSocket()
    {
        m_socket = sceNetSocket("reactormq", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);

        if (!isHandleValid())
        {
            const int32_t error = getLastErrorCode();
            REACTORMQ_LOG(logging::LogLevel::Error, getNetworkErrorDescription(error));
            m_socket = kInvalidSocketHandle;
            return false;
        }

        constexpr int32_t param = 1;
        sceNetSetsockopt(m_socket, SCE_NET_IPPROTO_TCP, SCE_NET_TCP_NODELAY, &param, sizeof(param));
        sceNetSetsockopt(m_socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &param, sizeof(param));
        const int result = sceNetSetsockopt(m_socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, &param, sizeof(param));
#ifdef SCE_NET_SO_REUSEPORT
        if (result == 0)
        {
            return sceNetSetsockopt(m_socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEPORT, &param, sizeof(param)) == 0;
        }
#endif

        return true;
    }

    int PlatformSocket::connect(const std::string& host, const std::uint16_t port)
    {
        m_state.store(SocketState::Connecting, std::memory_order_release);

        if (m_socket == kInvalidSocketHandle)
        {
            createSocket();
        }

        if (!isHandleValid())
        {
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return SCE_NET_ENOTSOCK;
        }

        // TODO: Not sure this is a good appoach
        static int32_t MemBlockId = sceNetPoolCreate("ResolverMemBlock", 4 * 1024, 0);
        static SceNetId ResolverId = sceNetResolverCreate("SonyResolver", MemBlockId, 0);

        SceNetResolverInfo AddressResults{};

        if (const SceNetId result = sceNetResolverStartNtoaMultipleRecords(ResolverId, host.c_str(), &AddressResults, 0, 0, 0);
            result < 0 || AddressResults.records <= 0)
        {
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return EADDRNOTAVAIL;
        }

        const SceNetInAddr& resolvedAddr = AddressResults.addrs[0].un.addr;

        SceNetSockaddrIn addr{};
        addr.sin_len = sizeof(addr);
        addr.sin_family = SCE_NET_AF_INET;
        addr.sin_port = sceNetHtons(port);
        addr.sin_addr = resolvedAddr;
        addr.sin_vport = 0;
        std::memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

        const int ret = sceNetConnect(m_socket, reinterpret_cast<SceNetSockaddr*>(&addr), sizeof(addr));

        if (ret == 0)
        {
            m_state.store(SocketState::Connected, std::memory_order_release);
            return 0;
        }

        const int32_t lastErr = getLastErrorCode();
        if (lastErr == SCE_NET_EINPROGRESS || lastErr == SCE_NET_EWOULDBLOCK)
        {
            return 0;
        }

        m_state.store(SocketState::Disconnected, std::memory_order_release);
        return ret;
    }
    bool PlatformSocket::tryReceive(uint8_t* outData, const int bufferSize, size_t& bytesRead) const
    {
        bytesRead = 0;
        if (!isHandleValid())
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSocket::tryReceive() cannot receive: invalid or closed handle");
            return false;
        }

        const int result = sceNetRecv(m_socket, outData, static_cast<size_t>(bufferSize), 0);

        if (result >= 0)
        {
            bytesRead = static_cast<size_t>(result);
            REACTORMQ_LOG_HEX(logging::LogLevel::Trace, outData, bytesRead, "PlatformSocket::tryReceive()");
            return true;
        }

        const SocketError err = getLastError();
        if (err == SocketError::WouldBlock)
        {
            bytesRead = 0;
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::tryReceive() would block");
            return true;
        }

        const int32_t code = getLastErrorCode();
        REACTORMQ_LOG(
            logging::LogLevel::Error,
            "PlatformSocket::tryReceive() recv failed (errorEnum=%d, errorCode=%d: %s)",
            err,
            code,
            getNetworkErrorDescription(code));

        return false;
    }
    bool PlatformSocket::tryReceiveWithFlags(uint8_t* outData, const int bufferSize, const ReceiveFlags flags, size_t& bytesRead) const
    {
        if (!isHandleValid())
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSocket::tryReceiveWithFlags() cannot receive: invalid or closed handle");
            return false;
        }

        int platformFlags = 0;
        if (hasFlag(flags, ReceiveFlags::Peek))
        {
            platformFlags |= SCE_NET_MSG_PEEK;
        }
        if (hasFlag(flags, ReceiveFlags::DontWait))
        {
            platformFlags |= SCE_NET_MSG_DONTWAIT;
        }
        if (hasFlag(flags, ReceiveFlags::WaitAll))
        {
            platformFlags |= SCE_NET_MSG_WAITALL;
        }

        REACTORMQ_LOG(
            logging::LogLevel::Trace,
            "PlatformSocket::tryReceiveWithFlags() bufferSize=%d, platformFlags=0x%x",
            bufferSize,
            platformFlags);

        const int result = sceNetRecv(m_socket, outData, static_cast<size_t>(bufferSize), platformFlags);

        if (result >= 0)
        {
            bytesRead = static_cast<size_t>(result);
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::tryReceiveWithFlags() bytesRead=%zd", bytesRead);
            return true;
        }

        const SocketError err = getLastError();
        if (err == SocketError::None || err == SocketError::WouldBlock || err == SocketError::InProgress
            || err == SocketError::AlreadyInProgress)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::tryReceiveWithFlags() would block");
            bytesRead = 0;
            return true;
        }

        const int32_t code = getLastErrorCode();
        REACTORMQ_LOG(
            logging::LogLevel::Error,
            "PlatformSocket::tryReceiveWithFlags() recv failed (errorEnum=%d, errorCode=%d: %s)",
            err,
            code,
            getNetworkErrorDescription(code));

        return false;
    }
    bool PlatformSocket::trySend(const uint8_t* data, const uint32_t size, size_t& bytesSent) const
    {
        bytesSent = 0;
        if (!isHandleValid())
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSocket::trySend() cannot send: invalid or closed handle");
            return false;
        }

        REACTORMQ_LOG_HEX(logging::LogLevel::Trace, data, size, "PlatformSocket::trySend()");

        const int result = sceNetSend(m_socket, data, static_cast<size_t>(size), 0);
        if (result >= 0)
        {
            bytesSent = static_cast<size_t>(result);
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::trySend() bytesSent=%zd", bytesSent);
            return true;
        }

        const SocketError err = getLastError();
        const int32_t code = getLastErrorCode();
        REACTORMQ_LOG(
            logging::LogLevel::Error,
            "PlatformSocket::trySend() failed (errorEnum=%d, errorCode=%d: %s)",
            err,
            code,
            getNetworkErrorDescription(code));

        return false;
    }
    SocketError PlatformSocket::getLastError()
    {
        return getErrorFromPlatformCode(getLastErrorCode());
    }
    int32_t PlatformSocket::getLastErrorCode()
    {
        return errno;
    }
    bool PlatformSocket::initializeNetworking()
    {
        return true;
    }
    void PlatformSocket::shutdownNetworking()
    {
        // Stub
    }
    void PlatformSocket::close()
    {
        if (m_socket == kInvalidSocketHandle)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::close() called on invalid handle");
            return;
        }

        REACTORMQ_LOG(logging::LogLevel::Debug, "PlatformSocket::close() closing socket (handle=%d)", static_cast<int>(m_socket));

        sceNetSocketClose(m_socket);
        m_socket = kInvalidSocketHandle;
        m_state.store(SocketState::Disconnected, std::memory_order_release);
    }
    bool PlatformSocket::isConnected() const
    {
        if (!isHandleValid())
        {
            return false;
        }

        const SocketState state = m_state.load(std::memory_order_acquire);
        if (state != SocketState::Connecting)
        {
            return state == SocketState::Connected || state == SocketState::SslConnecting;
        }

        fd_set writeSet;
        fd_set exceptSet;
        FD_ZERO(&writeSet);
        FD_ZERO(&exceptSet);
        FD_SET(m_socket, &writeSet);
        FD_SET(m_socket, &exceptSet);

        timeval tv{ 0, 1000 };

        const int ready = select(m_socket + 1, nullptr, &writeSet, &exceptSet, &tv);
        if (ready <= 0 || FD_ISSET(m_socket, &exceptSet))
        {
            return false;
        }

        int soError = 0;
        SceNetSocklen_t optLen = sizeof(soError);
        if (sceNetGetsockopt(m_socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_ERROR, &soError, &optLen) != 0 || soError != 0)
        {
            if (soError != 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::isConnected() SO_ERROR=%d", soError);
            }
            return false;
        }

        m_state.store(SocketState::Connected, std::memory_order_release);
        return true;
    }
    int PlatformSocket::getPendingData() const
    {
        if (SceNetSockInfo info; sceNetGetSockInfo(m_socket, &info, 1, 0) >= 0)
        {
            if (info.recv_queue_length > std::numeric_limits<int>::max())
            {
                return std::numeric_limits<int>::max();
            }
            return static_cast<int>(info.recv_queue_length);
        }

        return 0;
    }
    SocketError PlatformSocket::getErrorFromPlatformCode(const uint32_t platformCode)
    {
        switch (platformCode)
        {
            using enum SocketError;
        case 0:
            return None;
        case SCE_NET_EWOULDBLOCK:
            return WouldBlock;
        case SCE_NET_EINPROGRESS:
            return InProgress;
        case SCE_NET_EALREADY:
            return AlreadyInProgress;
        case SCE_NET_EINTR:
        case SCE_NET_ECANCELED:
        case SCE_NET_ENETINTR:
            return Interrupted;
        case SCE_NET_ECONNRESET:
        case SCE_NET_EPIPE:
            return ConnectionReset;
        case SCE_NET_ECONNABORTED:
        case SCE_NET_ESHUTDOWN:
            return ConnectionAborted;
        case SCE_NET_ETIMEDOUT:
        case SCE_NET_EDESCTIMEDOUT:
        case SCE_NET_RESOLVER_ETIMEDOUT:
            return TimedOut;
        case SCE_NET_ECONNREFUSED:
            return ConnectionRefused;
        case SCE_NET_ENOTCONN:
            return NotConnected;
        case SCE_NET_EADDRINUSE:
            return AddressInUse;
        case SCE_NET_EADDRNOTAVAIL:
            return AddressNotAvailable;
        case SCE_NET_ENETDOWN:
        case SCE_NET_EINACTIVEDISABLED:
            return NetworkDown;
        case SCE_NET_ENETUNREACH:
            return NetworkUnreachable;
        case SCE_NET_EHOSTDOWN:
        case SCE_NET_EHOSTUNREACH:
            return HostUnreachable;
        case SCE_NET_ENODATA:
        case SCE_NET_RESOLVER_ENORECORD:
            return NoData;
        case SCE_NET_RESOLVER_ENOHOST:
        case SCE_NET_RESOLVER_ENOTFOUND:
            return HostNotFound;
        case SCE_NET_RESOLVER_ESERVERFAILURE:
            return TryAgain;
        case SCE_NET_RESOLVER_ENODNS:
        case SCE_NET_RESOLVER_ESERVERREFUSED:
        case SCE_NET_RESOLVER_EINTERNAL:
            return NoRecovery;
        case SCE_NET_EACCES:
        case SCE_NET_EAUTH:
        case SCE_NET_ENEEDAUTH:
            return AccessDenied;
        case SCE_NET_EINVAL:
        case SCE_NET_EFAULT:
        case SCE_NET_EDESTADDRREQ:
        case SCE_NET_E2BIG:
        case SCE_NET_EDOM:
        case SCE_NET_ERANGE:
        case SCE_NET_ENAMETOOLONG:
        case SCE_NET_EFTYPE:
            return InvalidArgument;
        case SCE_NET_ENFILE:
        case SCE_NET_EMFILE:
        case SCE_NET_EPROCLIM:
            return TooManyOpenFiles;
        case SCE_NET_ENOTSOCK:
        case SCE_NET_EBADF:
            return NotSocket;
        case SCE_NET_EOPNOTSUPP:
        case SCE_NET_ENOSYS:
        case SCE_NET_RESOLVER_ENOSUPPORT:
        case SCE_NET_RESOLVER_ENOTIMPLEMENTED:
            return OperationNotSupported;
        case SCE_NET_EPROTO:
        case SCE_NET_EPROTOTYPE:
        case SCE_NET_EBADRPC:
        case SCE_NET_RESOLVER_EPACKET:
        case SCE_NET_RESOLVER_EFORMAT:
            return ProtocolError;
        case SCE_NET_EPROTONOSUPPORT:
        case SCE_NET_EPFNOSUPPORT:
        case SCE_NET_EADHOC:
        case SCE_NET_EPROGUNAVAIL:
        case SCE_NET_EPROGMISMATCH:
        case SCE_NET_EPROCUNAVAIL:
            return ProtocolNotSupported;
        case SCE_NET_EAFNOSUPPORT:
            return AddressFamilyNotSupported;
        case SCE_NET_ENOTINIT:
        case SCE_NET_RESOLVER_ENOTINIT:
            return SystemNotReady;
        case SCE_NET_EMSGSIZE:
        case SCE_NET_EOVERFLOW:
            return MessageTooLong;
        default:
            return Unknown;
        }
    }
    const char* PlatformSocket::getNetworkErrorDescription(const int32_t errorCode)
    {
        static char error_buffer[2000];
        strerror_r(errorCode, error_buffer, sizeof(error_buffer));
        return error_buffer;
    }
    bool PlatformSocket::isHandleValid() const
    {
        return m_socket > SocketHandle{ 0 };
    }
} // namespace reactormq::socket

#endif // REACTORMQ_SOCKET_WITH_SONY_SOCKET
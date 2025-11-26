//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_SOCKET_WITH_POSIX_SOCKET

#include "platform_socket.h"
#include "socket/platform/receive_flags.h"
#include "socket/platform/socket_error.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

#include <csignal>
#include <cstring>

namespace reactormq::socket
{
    bool PlatformSocket::createSocket()
    {
        m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (!isHandleValid())
        {
            const int32_t error = getLastErrorCode();
            REACTORMQ_LOG(logging::LogLevel::Error, getNetworkErrorDescription(error));
            m_socket = kInvalidSocketHandle;
            return false;
        }

        constexpr int param = 1;

        setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &param, sizeof(param));
        if (const int flags = fcntl(m_socket, F_GETFL, 0); flags != -1)
        {
            fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
        }

#ifdef SO_REUSEPORT
        if (const int result = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &param, sizeof(param)); result == 0)
        {
            return setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &param, sizeof(param)) == 0;
        }
#endif

#if REACTORMQ_PLATFORM_DARWIN_FAMILY
        size_t bAllow = 1;
        setsockopt(m_socket, SOL_SOCKET, SO_NOSIGPIPE, &bAllow, sizeof(bAllow));
#endif // REACTORMQ_PLATFORM_DARWIN_FAMILY

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
            return ENOTSOCK;
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        const std::string portStr = std::to_string(port);

        if (const int gaiErr = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result); gaiErr != 0 || nullptr == result)
        {
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return EADDRNOTAVAIL;
        }

        int lastErr = 0;
        int ret = -1;

        for (const addrinfo* rp = result; rp != nullptr; rp = rp->ai_next)
        {
            ret = ::connect(m_socket, rp->ai_addr, rp->ai_addrlen);
            if (ret != 0)
            {
                lastErr = errno;
            }

            if (ret == 0 || lastErr == EINPROGRESS)
            {
                break;
            }
        }

        freeaddrinfo(result);
        if (ret != 0 && errno == 0 && lastErr != 0)
        {
            errno = lastErr;
        }

        if (ret == 0)
        {
            m_state.store(SocketState::Connected, std::memory_order_release);
            return 0;
        }

        if (lastErr == EINPROGRESS)
        {
            return 0;
        }

        m_state.store(SocketState::Disconnected, std::memory_order_release);
        return ret;
    }
    bool PlatformSocket::isConnected() const
    {
        if (!isHandleValid())
        {
            return false;
        }

        if (const SocketState state = m_state.load(std::memory_order_acquire); state != SocketState::Connecting)
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

        if (const int ready = select(m_socket + 1, nullptr, &writeSet, &exceptSet, &tv); ready <= 0 || FD_ISSET(m_socket, &exceptSet))
        {
            return false;
        }

        int soError = 0;
        if (socklen_t optLen = sizeof(soError); getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &soError, &optLen) != 0 || soError != 0)
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
#if REACTORMQ_SOCKET_WITH_IOCTL
        if (u_long pendingBytes = 0; ioctlsocket(m_socket, FIONREAD, &pendingBytes) == 0)
        {
            if (pendingBytes > std::numeric_limits<int>::max())
            {
                return std::numeric_limits<int>::max();
            }
            return static_cast<int>(pendingBytes);
        }
#endif
        return 0;
    }

    bool PlatformSocket::tryReceive(uint8_t* outData, const int bufferSize, size_t& bytesRead) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::tryReceive() bufferSize=%d", bufferSize);

        if (!isHandleValid())
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSocket::tryReceive() cannot receive: invalid or closed handle");
            return false;
        }

        if (const ssize_t result = recv(m_socket, outData, static_cast<size_t>(bufferSize), 0); result >= 0)
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
            platformFlags |= MSG_PEEK;
        }
        if (hasFlag(flags, ReceiveFlags::DontWait))
        {
            platformFlags |= MSG_DONTWAIT;
        }
        if (hasFlag(flags, ReceiveFlags::WaitAll))
        {
            platformFlags |= MSG_WAITALL;
        }

        REACTORMQ_LOG(
            logging::LogLevel::Trace,
            "PlatformSocket::tryReceiveWithFlags() bufferSize=%d, platformFlags=0x%x",
            bufferSize,
            platformFlags);

        if (const ssize_t result = recv(m_socket, outData, static_cast<size_t>(bufferSize), platformFlags); result >= 0)
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

        if (const ssize_t result = send(m_socket, data, size, 0); result >= 0)
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
        return getErrorFromPlatformCode(static_cast<uint32_t>(getLastErrorCode()));
    }
    int32_t PlatformSocket::getLastErrorCode()
    {
        return errno;
    }
    bool PlatformSocket::initializeNetworking()
    {
        signal(SIGPIPE, SIG_IGN);
        return true;
    }
    void PlatformSocket::shutdownNetworking()
    {
        // Not required
    }
    void PlatformSocket::close()
    {
        if (m_socket == kInvalidSocketHandle)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::close() called on invalid handle");
            return;
        }

        REACTORMQ_LOG(logging::LogLevel::Debug, "PlatformSocket::close() closing socket (handle=%d)", m_socket);

        ::close(m_socket);
        m_socket = kInvalidSocketHandle;
        m_state.store(SocketState::Disconnected, std::memory_order_release);
    }
    SocketError PlatformSocket::getErrorFromPlatformCode(const uint32_t platformCode)
    {
        switch (platformCode)
        {
            using enum SocketError;
        case 0:
            return None;

#ifdef EWOULDBLOCK
        case EWOULDBLOCK:
#else
        case EAGAIN:
#endif
            return WouldBlock;
        case EINPROGRESS:
            return InProgress;
        case EALREADY:
            return AlreadyInProgress;
        case EINTR:
            return Interrupted;
        case ECONNRESET:
        case EPIPE:
            return ConnectionReset;
        case ECONNABORTED:
            return ConnectionAborted;
        case ETIMEDOUT:
            return TimedOut;
        case ECONNREFUSED:
            return ConnectionRefused;
        case ENOTCONN:
            return NotConnected;
        case EADDRINUSE:
            return AddressInUse;
        case EADDRNOTAVAIL:
            return AddressNotAvailable;
        case ENETDOWN:
            return NetworkDown;
        case ENETUNREACH:
            return NetworkUnreachable;

#ifdef EHOSTDOWN
        case EHOSTDOWN:
#endif
        case EHOSTUNREACH:
            return HostUnreachable;

#ifdef ENODATA
        case ENODATA:
            return NoData;
#endif

        case EACCES:
            return AccessDenied;
        case EINVAL:
        case EFAULT:
        case EDESTADDRREQ:
        case E2BIG:
        case EDOM:
        case ERANGE:
#ifdef ENAMETOOLONG
        case ENAMETOOLONG:
#endif
            return InvalidArgument;
        case ENFILE:
        case EMFILE:
            return TooManyOpenFiles;
        case ENOTSOCK:
        case EBADF:
            return NotSocket;
        case EOPNOTSUPP:
#ifdef ENOSYS
        case ENOSYS:
#endif
            return OperationNotSupported;
#ifdef EPROTO
        case EPROTO:
#endif
        case EPROTOTYPE:
            return ProtocolError;
        case EPROTONOSUPPORT:
            return ProtocolNotSupported;
        case EAFNOSUPPORT:
            return AddressFamilyNotSupported;
        case EMSGSIZE:
            return MessageTooLong;
        default:
            return Unknown;
        }
    }

    const char* PlatformSocket::getNetworkErrorDescription(const int32_t errorCode)
    {
        static thread_local std::string buffer;
        buffer.assign(512, '\0');
        strerror_r(errorCode, buffer.data(), sizeof(buffer.size()));
        return buffer.c_str();
    }
    bool PlatformSocket::isHandleValid() const
    {
        return m_socket > SocketHandle{ 0 };
    }
} // namespace reactormq::socket
#endif // REACTORMQ_WITH_POSIX_SOCKET
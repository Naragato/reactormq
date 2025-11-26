//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_SOCKET_WITH_WINSOCKET

#include "platform_socket.h"
#include "socket/platform/receive_flags.h"
#include "socket/platform/socket_error.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <format>
#include <utility>

namespace reactormq::socket
{
    bool PlatformSocket::createSocket()
    {
        REACTORMQ_LOG(logging::LogLevel::Debug, "PlatformSocket::createSocket() creating TCP socket");

        m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (!isHandleValid())
        {
            const int32_t error = getLastErrorCode();
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "PlatformSocket::createSocket() failed (error=%d: %s)",
                error,
                getNetworkErrorDescription(error));
            m_socket = kInvalidSocketHandle;
            return false;
        }

        {
            constexpr BOOL param = TRUE;
            const int r = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&param), sizeof(param));
            if (r != 0)
            {
                const int32_t error = getLastErrorCode();
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "PlatformSocket::createSocket() setsockopt(TCP_NODELAY) failed (error=%d: %s)",
                    error,
                    getNetworkErrorDescription(error));
            }
        }

        {
            u_long nonBlocking = 1;
            const int r = ioctlsocket(m_socket, FIONBIO, &nonBlocking);
            if (r != 0)
            {
                const int32_t error = getLastErrorCode();
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "PlatformSocket::createSocket() ioctlsocket(FIONBIO) failed (error=%d: %s)",
                    error,
                    getNetworkErrorDescription(error));
            }
        }

        {
            constexpr BOOL reuse = TRUE;
            int result = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
            if (result != 0)
            {
                const int32_t error = getLastErrorCode();
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "PlatformSocket::createSocket() setsockopt(SO_REUSEADDR) failed (error=%d: %s)",
                    error,
                    getNetworkErrorDescription(error));
            }

#ifdef SO_REUSEPORT
            if (result == 0)
            {
                result = setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
                if (result != 0)
                {
                    const int32_t error = getLastErrorCode();
                    REACTORMQ_LOG(
                        logging::LogLevel::Debug,
                        "PlatformSocket::createSocket() setsockopt(SO_REUSEPORT) failed (error=%d: %s)",
                        error,
                        getNetworkErrorDescription(error));
                }
            }
#endif
        }

        REACTORMQ_LOG(logging::LogLevel::Debug, "PlatformSocket::createSocket() succeeded (handle=%p)", m_socket);
        return true;
    }

    int PlatformSocket::connect(const std::string& host, const std::uint16_t port)
    {
        REACTORMQ_LOG(logging::LogLevel::Info, "PlatformSocket::connect() starting (host=%s, port=%u)", host.c_str(), port);

        m_state.store(SocketState::Connecting, std::memory_order_release);

        if (m_socket == kInvalidSocketHandle)
        {
            if (!createSocket())
            {
                m_state.store(SocketState::Disconnected, std::memory_order_release);
                return WSAENOTSOCK;
            }
        }

        if (!isHandleValid())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSocket::connect() invalid socket handle");
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return WSAENOTSOCK;
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        const std::string portStr = std::to_string(port);

        if (const int gaiErr = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result); gaiErr != 0 || nullptr == result)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "PlatformSocket::connect() getaddrinfo failed (host=%s, port=%s, gaiErr=%d)",
                host.c_str(),
                portStr.c_str(),
                gaiErr);

            m_state.store(SocketState::Disconnected, std::memory_order_release);
            switch (gaiErr)
            {
            case EAI_AGAIN:
                return WSATRY_AGAIN;
            case EAI_NONAME:
                return WSAHOST_NOT_FOUND;
            default:
                return WSAEADDRNOTAVAIL;
            }
        }

        int lastErr = 0;
        int ret = SOCKET_ERROR;

        for (const addrinfo* rp = result; rp != nullptr; rp = rp->ai_next)
        {
            ret = ::connect(m_socket, rp->ai_addr, static_cast<int>(rp->ai_addrlen));
            if (ret != 0)
            {
                lastErr = WSAGetLastError();
            }

            if (ret == 0 || lastErr == WSAEWOULDBLOCK || lastErr == WSAEINPROGRESS)
            {
                break;
            }
        }

        freeaddrinfo(result);

        if (ret == 0 && isConnected())
        {
            m_state.store(SocketState::Connected, std::memory_order_release);
            REACTORMQ_LOG(
                logging::LogLevel::Info,
                "PlatformSocket::connect() established synchronously (host=%s, port=%u)",
                host.c_str(),
                port);
            return 0;
        }

        if (lastErr == WSAEWOULDBLOCK || lastErr == WSAEINPROGRESS)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Debug,
                "PlatformSocket::connect() in progress (non-blocking, error=%d: %s)",
                lastErr,
                getNetworkErrorDescription(lastErr));
            return 0;
        }

        m_state.store(SocketState::Disconnected, std::memory_order_release);
        REACTORMQ_LOG(
            logging::LogLevel::Error,
            "PlatformSocket::connect() failed (host=%s, port=%u, lastErr=%d: %s)",
            host.c_str(),
            port,
            lastErr,
            getNetworkErrorDescription(lastErr));

        return ret;
    }
    bool PlatformSocket::tryReceive(uint8_t* outData, const int bufferSize, size_t& bytesRead) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::tryReceive() bufferSize=%d", bufferSize);

        if (!isHandleValid())
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSocket::tryReceive() cannot receive: invalid or closed handle");
            return false;
        }

        const int result = recv(m_socket, reinterpret_cast<char*>(outData), bufferSize, 0);

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
            platformFlags |= MSG_PEEK;
        }
        if (hasFlag(flags, ReceiveFlags::DontWait))
        {
            // Windows doesn't have MSG_DONTWAIT, but the socket is already non-blocking
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

        const int result = recv(m_socket, reinterpret_cast<char*>(outData), bufferSize, platformFlags);

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

        const int result = send(m_socket, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
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
        return getErrorFromPlatformCode(static_cast<uint32_t>(getLastErrorCode()));
    }
    int32_t PlatformSocket::getLastErrorCode()
    {
        return WSAGetLastError();
    }
    bool PlatformSocket::initializeNetworking()
    {
        static bool s_initialized = false;

        if (s_initialized)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::initializeNetworking() already initialized");
            return true;
        }

        WSADATA wsaData{};

        if (const int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSocket::initializeNetworking() WSAStartup failed (error=%d)", result);
            return false;
        }

        s_initialized = true;
        REACTORMQ_LOG(logging::LogLevel::Info, "PlatformSocket::initializeNetworking() WSAStartup succeeded");
        return true;
    }
    void PlatformSocket::shutdownNetworking()
    {
        REACTORMQ_LOG(logging::LogLevel::Info, "PlatformSocket::shutdownNetworking() WSACleanup");
        WSACleanup();
    }
    void PlatformSocket::close()
    {
        if (m_socket == kInvalidSocketHandle)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::close() called on invalid handle");
            return;
        }

        REACTORMQ_LOG(logging::LogLevel::Debug, "PlatformSocket::close() closing socket (handle=%p)", m_socket);

        closesocket(m_socket);
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

        constexpr timeval tv{ 0, 1000 };

        const int ready = select(0, nullptr, &writeSet, &exceptSet, &tv);
        if (ready <= 0 || FD_ISSET(m_socket, &exceptSet))
        {
            return false;
        }

        int soError = 0;
        int optLen = sizeof(soError);
        if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &optLen) != 0 || soError != 0)
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
            REACTORMQ_LOG(logging::LogLevel::Trace, "PlatformSocket::getPendingData() pendingBytes=%lu", pendingBytes);
            if (pendingBytes > static_cast<u_long>(std::numeric_limits<int>::max()))
            {
                return std::numeric_limits<int>::max();
            }
            return pendingBytes;
        }
#endif
        return 0;
    }

    SocketError PlatformSocket::getErrorFromPlatformCode(const uint32_t platformCode)
    {
        switch (platformCode)
        {
            using enum SocketError;
        case 0:
            return None;

        case WSAEWOULDBLOCK:
            return WouldBlock;

        case WSAEINPROGRESS:
            return InProgress;

        case WSAEALREADY:
            return AlreadyInProgress;

        case WSAEINTR:
            return Interrupted;

        case WSAECONNRESET:
            return ConnectionReset;

        case WSAECONNABORTED:
            return ConnectionAborted;

        case WSAETIMEDOUT:
            return TimedOut;

        case WSAECONNREFUSED:
            return ConnectionRefused;

        case WSAENOTCONN:
            return NotConnected;

        case WSAEADDRINUSE:
            return AddressInUse;

        case WSAEADDRNOTAVAIL:
            return AddressNotAvailable;

        case WSAENETDOWN:
            return NetworkDown;

        case WSAENETUNREACH:
            return NetworkUnreachable;

        case WSAEHOSTDOWN:
        case WSAEHOSTUNREACH:
            return HostUnreachable;

        case WSAHOST_NOT_FOUND:
            return HostNotFound;
        case WSATRY_AGAIN:
            return TryAgain;
        case WSANO_RECOVERY:
            return NoRecovery;
        case WSANO_DATA:
            return NoData;

        case WSAEMSGSIZE:
            return MessageTooLong;

        case WSAEACCES:
            return AccessDenied;

        case WSAEINVAL:
        case WSAEFAULT:
        case WSAEDESTADDRREQ:
        case WSAEDISCON:
            return InvalidArgument;

        case WSAEMFILE:
        case WSAEPROCLIM:
        case WSAEUSERS:
        case WSAEDQUOT:
            return TooManyOpenFiles;

        case WSAENOTSOCK:
            return NotSocket;

        case WSAEOPNOTSUPP:
            return OperationNotSupported;

        case WSAEPROTOTYPE:
            return ProtocolError;

        case WSAEPROTONOSUPPORT:
        case WSAEPFNOSUPPORT:
        case WSAESOCKTNOSUPPORT:
            return ProtocolNotSupported;

        case WSAEAFNOSUPPORT:
            return AddressFamilyNotSupported;

        case WSASYSNOTREADY:
        case WSANOTINITIALISED:
            return SystemNotReady;

        case WSAVERNOTSUPPORTED:
            return VersionNotSupported;

        default:
            return Unknown;
        }
    }
    const char* PlatformSocket::getNetworkErrorDescription(const int32_t errorCode)
    {
        static thread_local std::string buffer;
        buffer.assign(512, '\0');

        constexpr DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        if (const DWORD len = FormatMessageA(
                flags,
                nullptr,
                static_cast<DWORD>(errorCode),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                nullptr);
            len == 0)
        {
            buffer = std::format("Unknown error: {}", errorCode);
        }
        else
        {
            buffer.resize(len);
            while (!buffer.empty() && (buffer.back() == '\r' || buffer.back() == '\n'))
            {
                buffer.pop_back();
            }
        }

        return buffer.c_str();
    }
    bool PlatformSocket::isHandleValid() const
    {
        return m_socket != kInvalidSocketHandle;
    }
} // namespace reactormq::socket

#endif // REACTORMQ_SOCKET_WITH_WINSOCKET
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_WITH_UE5

#include "platform_socket.h"
#include "socket/platform/receive_flags.h"
#include "socket/platform/socket_error.h"
#include "util/logging/logging.h"

#include "SocketSubsystem.h"
#include "Sockets.h"

namespace reactormq::socket
{
    namespace
    {
        ESocketReceiveFlags::Type ToUERecvFlags(const ReceiveFlags flags)
        {
            uint8 bits = 0;
            if (hasFlag(flags, ReceiveFlags::Peek))
            {
                bits |= static_cast<uint8>(ESocketReceiveFlags::Peek);
            }
            if (hasFlag(flags, ReceiveFlags::WaitAll))
            {
                bits |= static_cast<uint8>(ESocketReceiveFlags::WaitAll);
            }
            // DontWait is implicit since we set the socket to non-blocking
            return static_cast<ESocketReceiveFlags::Type>(bits);
        }
    } // namespace

    bool PlatformSocket::createSocket()
    {
        REACTORMQ_LOG(logging::LogLevel::Debug, "UE5 PlatformSocket::createSocket() creating FSocket TCP");

        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (!Subsys)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "UE5 PlatformSocket::createSocket() no socket subsystem");
            return false;
        }

        // Create a stream (TCP) socket. Name is for debug purposes only.
        m_socket = Subsys->CreateSocket(NAME_Stream, TEXT("ReactorMQ-TCP"), false);
        if (!isHandleValid())
        {
            const int32 code = Subsys->GetLastErrorCode();
            REACTORMQ_LOG(logging::LogLevel::Error, "CreateSocket failed: %d", code);
            return false;
        }

        // Non-blocking
        m_socket->SetNonBlocking(true);

        // No delay (Nagle off)
        m_socket->SetNoDelay(true);

        // Reuse address if supported
        m_socket->SetReuseAddr(true);

        REACTORMQ_LOG(logging::LogLevel::Debug, "UE5 PlatformSocket::createSocket() succeeded (%p)", m_socket);
        return true;
    }

    int PlatformSocket::connect(const std::string& host, const std::uint16_t port)
    {
        REACTORMQ_LOG(logging::LogLevel::Info, "UE5 PlatformSocket::connect() host=%s port=%u", host.c_str(), port);

        m_state.store(SocketState::Connecting, std::memory_order_release);

        if (m_socket == kInvalidSocketHandle)
        {
            if (!createSocket())
            {
                m_state.store(SocketState::Disconnected, std::memory_order_release);
                return static_cast<int>(ESocketErrors::SE_ENOTSOCK);
            }
        }

        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (!Subsys)
        {
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return static_cast<int>(ESocketErrors::SE_ENOBUFS);
        }

        FAddressInfoResult AddrInfo = Subsys->GetAddressInfo(
            *FString(UTF8_TO_TCHAR(host.c_str())),
            nullptr,
            EAddressInfoFlags::Default,
            NAME_None,
            SOCKTYPE_Streaming);
        if (AddrInfo.ReturnCode != SE_NO_ERROR || AddrInfo.Results.Num() <= 0)
        {
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return static_cast<int>(AddrInfo.ReturnCode != SE_NO_ERROR ? AddrInfo.ReturnCode : ESocketErrors::SE_HOST_NOT_FOUND);
        }

        const TSharedRef<FInternetAddr> Addr = AddrInfo.Results[0].Address;
        Addr->SetPort(port);

        if (m_socket->Connect(*Addr))
        {
            m_state.store(SocketState::Connected, std::memory_order_release);
            return 0;
        }

        const ESocketErrors SocketError = Subsys->GetLastErrorCode();
        if (SocketError == SE_EWOULDBLOCK || SocketError == SE_EINPROGRESS)
        {
            return 0;
        }

        m_state.store(SocketState::Disconnected, std::memory_order_release);
        return SocketError;
    }

    void PlatformSocket::close()
    {
        if (!isHandleValid())
        {
            return;
        }
        if (ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
        {
            Subsys->DestroySocket(m_socket);
        }
        m_socket = kInvalidSocketHandle;
        m_state.store(SocketState::Disconnected, std::memory_order_release);
    }

    bool PlatformSocket::isConnected() const
    {
        if (!isHandleValid())
        {
            return false;
        }

        return m_state.load(std::memory_order_acquire) == SocketState::Connected;
    }

    int PlatformSocket::getPendingData() const
    {
        if (!isHandleValid())
        {
            return 0;
        }
        uint32 pending = 0;
        if (m_socket->HasPendingData(pending))
        {
            return static_cast<size_t>(pending);
        }
        return 0;
    }

    bool PlatformSocket::tryReceive(uint8_t* outData, const int32_t bufferSize, size_t& bytesRead) const
    {
        bytesRead = 0;
        if (!isHandleValid())
        {
            return false;
        }
        int32 Read = 0;
        if (m_socket->Recv(outData, bufferSize, Read, ESocketReceiveFlags::None))
        {
            bytesRead = static_cast<size_t>(Read);
            return true;
        }

        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        const ESocketErrors se = Subsys ? Subsys->GetLastErrorCode() : SE_NO_ERROR;
        if (se == SE_EWOULDBLOCK)
        {
            bytesRead = 0;
            return true;
        }
        return false;
    }

    bool PlatformSocket::tryReceiveWithFlags(uint8_t* outData, const int32_t bufferSize, const ReceiveFlags flags, size_t& bytesRead) const
    {
        bytesRead = 0;
        if (!isHandleValid())
        {
            return false;
        }
        int32 Read = 0;
        const ESocketReceiveFlags::Type ueFlags = ToUERecvFlags(flags);
        if (m_socket->Recv(outData, bufferSize, Read, ueFlags))
        {
            bytesRead = static_cast<size_t>(Read);
            return true;
        }
        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        const ESocketErrors se = Subsys ? Subsys->GetLastErrorCode() : SE_NO_ERROR;
        if (se == SE_EWOULDBLOCK || se == SE_EINPROGRESS)
        {
            bytesRead = 0;
            return true;
        }
        return false;
    }

    bool PlatformSocket::trySend(const uint8_t* data, const uint32_t size, size_t& bytesSent) const
    {
        bytesSent = 0;
        if (!isHandleValid())
        {
            return false;
        }
        int32 Sent = 0;
        if (m_socket->Send(data, static_cast<int32>(size), Sent))
        {
            bytesSent = static_cast<size_t>(Sent);
            return true;
        }

        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        const ESocketErrors se = Subsys ? Subsys->GetLastErrorCode() : SE_NO_ERROR;
        if (se == SE_EWOULDBLOCK || se == SE_EINPROGRESS)
        {
            bytesSent = 0;
            return true;
        }
        return false;
    }

    SocketError PlatformSocket::getLastError()
    {
        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        const ESocketErrors se = Subsys ? Subsys->GetLastErrorCode() : SE_NO_ERROR;
        return getErrorFromPlatformCode(static_cast<uint32>(se));
    }

    int32_t PlatformSocket::getLastErrorCode()
    {
        ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        return Subsys ? static_cast<int32_t>(Subsys->GetLastErrorCode()) : 0;
    }

    const char* PlatformSocket::getNetworkErrorDescription(const int32_t errorCode)
    {
        switch (static_cast<ESocketErrors>(errorCode))
        {
        case SE_NO_ERROR:
            return "No error";
        case SE_EINTR:
            return "Interrupted system call";
        case SE_EBADF:
            return "Bad file descriptor";
        case SE_EACCES:
            return "Permission denied";
        case SE_EFAULT:
            return "Bad address";
        case SE_EINVAL:
            return "Invalid argument";
        case SE_EMFILE:
            return "Too many open files";
        case SE_EWOULDBLOCK:
            return "Operation would block";
        case SE_EINPROGRESS:
            return "Operation now in progress";
        case SE_EALREADY:
            return "Operation already in progress";
        case SE_ENOTSOCK:
            return "Socket operation on non-socket";
        case SE_EDESTADDRREQ:
            return "Destination address required";
        case SE_EMSGSIZE:
            return "Message too long";
        case SE_EPROTOTYPE:
            return "Protocol wrong type for socket";
        case SE_ENOPROTOOPT:
            return "Protocol not available";
        case SE_EPROTONOSUPPORT:
            return "Protocol not supported";
        case SE_ESOCKTNOSUPPORT:
            return "Socket type not supported";
        case SE_EOPNOTSUPP:
            return "Operation not supported";
        case SE_EAFNOSUPPORT:
            return "Address family not supported";
        case SE_EADDRINUSE:
            return "Address already in use";
        case SE_EADDRNOTAVAIL:
            return "Cannot assign requested address";
        case SE_ENETDOWN:
            return "Network is down";
        case SE_ENETUNREACH:
            return "Network is unreachable";
        case SE_ENETRESET:
            return "Network dropped connection on reset";
        case SE_ECONNABORTED:
            return "Software caused connection abort";
        case SE_ECONNRESET:
            return "Connection reset by peer";
        case SE_ENOBUFS:
            return "No buffer space available";
        case SE_EISCONN:
            return "Transport endpoint is already connected";
        case SE_ENOTCONN:
            return "Transport endpoint is not connected";
        case SE_ESHUTDOWN:
            return "Cannot send after transport endpoint shutdown";
        case SE_ETOOMANYREFS:
            return "Too many references";
        case SE_ETIMEDOUT:
            return "Connection timed out";
        case SE_ECONNREFUSED:
            return "Connection refused";
        case SE_ELOOP:
            return "Too many symbolic links encountered";
        case SE_ENAMETOOLONG:
            return "File name too long";
        case SE_EHOSTDOWN:
            return "Host is down";
        case SE_EHOSTUNREACH:
            return "No route to host";
        case SE_ENOTEMPTY:
            return "Directory not empty";
        case SE_EUSERS:
            return "Too many users";
        case SE_EDQUOT:
            return "Disc quota exceeded";
        case SE_ESTALE:
            return "Stale file handle";
        case SE_EREMOTE:
            return "Object is remote";
        case SE_EDISCON:
            return "Graceful disconnect";
        case SE_SYSNOTREADY:
            return "Network subsystem is unavailable";
        case SE_VERNOTSUPPORTED:
            return "Winsock.dll version out of range";
        case SE_NOTINITIALISED:
            return "Successful WSAStartup not yet performed";
        case SE_HOST_NOT_FOUND:
            return "Host not found";
        case SE_TRY_AGAIN:
            return "Non-authoritative host not found, or server failure";
        case SE_NO_RECOVERY:
            return "Non-recoverable error";
        case SE_NO_DATA:
            return "Valid name, no data record of requested type";
        default:
            return "Unknown socket error";
        }
    }

    bool PlatformSocket::initializeNetworking()
    {
        // UE handles subsystem init; nothing to do.
        return true;
    }

    void PlatformSocket::shutdownNetworking()
    {
        // UE handles subsystem shutdown; nothing to do.
    }

    SocketError PlatformSocket::getErrorFromPlatformCode(const uint32_t platformCode)
    {
        switch (static_cast<ESocketErrors>(platformCode))
        {
        case SE_NO_ERROR:
            return SocketError::None;
        case SE_EWOULDBLOCK:
            return SocketError::WouldBlock;
        case SE_EINPROGRESS:
            return SocketError::InProgress;
        case SE_EALREADY:
            return SocketError::AlreadyInProgress;
        case SE_EINTR:
            return SocketError::Interrupted;
        case SE_ECONNRESET:
            return SocketError::ConnectionReset;
        case SE_ECONNABORTED:
            return SocketError::ConnectionAborted;
        case SE_ETIMEDOUT:
            return SocketError::TimedOut;
        case SE_ECONNREFUSED:
            return SocketError::ConnectionRefused;
        case SE_ENOTCONN:
            return SocketError::NotConnected;
        case SE_EADDRINUSE:
            return SocketError::AddressInUse;
        case SE_EADDRNOTAVAIL:
            return SocketError::AddressNotAvailable;
        case SE_ENETDOWN:
            return SocketError::NetworkDown;
        case SE_ENETUNREACH:
            return SocketError::NetworkUnreachable;
        case SE_EHOSTUNREACH:
            return SocketError::HostUnreachable;
        case SE_HOST_NOT_FOUND:
            return SocketError::HostNotFound;
        case SE_TRY_AGAIN:
            return SocketError::TryAgain;
        case SE_NO_RECOVERY:
            return SocketError::NoRecovery;
        case SE_NO_DATA:
            return SocketError::NoData;
        case SE_EMSGSIZE:
            return SocketError::MessageTooLong;
        case SE_EACCES:
            return SocketError::AccessDenied;
        case SE_EINVAL:
            return SocketError::InvalidArgument;
        case SE_ENOTSOCK:
            return SocketError::NotSocket;
        case SE_EOPNOTSUPP:
            return SocketError::OperationNotSupported;
        case SE_EPROTONOSUPPORT:
            return SocketError::ProtocolNotSupported;
        case SE_EAFNOSUPPORT:
            return SocketError::AddressFamilyNotSupported;
        case SE_EMFILE:
            return SocketError::TooManyOpenFiles;
        case SE_SYSNOTREADY:
            return SocketError::SystemNotReady;
        case SE_VERNOTSUPPORTED:
            return SocketError::VersionNotSupported;
        default:
            return SocketError::Unknown;
        }
    }

    bool PlatformSocket::isHandleValid() const
    {
        return m_socket != nullptr;
    }
} // namespace reactormq::socket

#endif // REACTORMQ_WITH_UE5
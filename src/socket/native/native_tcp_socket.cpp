#include "native_tcp_socket.h"

#include <algorithm>
#include <cstring>

#ifdef REACTORMQ_PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#endif // REACTORMQ_PLATFORM_WINDOWS

namespace reactormq::socket
{
    namespace
    {
        constexpr uint32_t kReceiveBufferSize = 8192;

#ifdef REACTORMQ_PLATFORM_WINDOWS
        class WinsockInitializer
        {
        public:
            WinsockInitializer()
            {
                WSADATA wsaData;
                WSAStartup(MAKEWORD(2, 2), &wsaData);
            }

            ~WinsockInitializer()
            {
                WSACleanup();
            }
        };

        void ensureWinsockInitialized()
        {
            static WinsockInitializer initializer;
        }

        int32_t getLastError()
        {
            return WSAGetLastError();
        }

        bool isWouldBlock(const int32_t error)
        {
            return error == WSAEWOULDBLOCK;
        }

        bool isInProgress(const int32_t error)
        {
            return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
        }
#else
        void ensureWinsockInitialized()
        {
        }

        int32_t getLastError()
        {
            return errno;
        }

        bool isWouldBlock(int32_t error)
        {
            return error == EWOULDBLOCK || error == EAGAIN;
        }

        bool isInProgress(int32_t error)
        {
            return error == EINPROGRESS;
        }
#endif // REACTORMQ_PLATFORM_WINDOWS
    }

    NativeTcpSocket::NativeTcpSocket(const mqtt::ConnectionSettingsPtr& settings)
        : SocketDelegateMixin(settings), m_socketHandle(kInvalidSocketHandle), m_isConnecting(false)
    {
        ensureWinsockInitialized();
    }

    NativeTcpSocket::~NativeTcpSocket()
    {
        closeSocket();
    }

    bool NativeTcpSocket::isEncrypted() const
    {
        return false;
    }

    void NativeTcpSocket::connect()
    {
        std::scoped_lock lock(m_socketMutex);

        if (m_socketHandle != kInvalidSocketHandle)
        {
            closeSocket();
        }

        if (!createSocket())
        {
            invokeOnConnect(false);
            return;
        }

        if (!setNonBlocking() || !setNoDelay())
        {
            closeSocket();
            invokeOnConnect(false);
            return;
        }

        const std::string& host = m_settings->getHost();
        const uint16_t port = m_settings->getPort();

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        const int32_t getAddrInfoResult = getaddrinfo(host.c_str(), nullptr, &hints, &result);

        if (getAddrInfoResult != 0 || result == nullptr)
        {
            closeSocket();
            invokeOnConnect(false);
            return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        std::memcpy(&addr.sin_addr, &reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr, sizeof(addr.sin_addr));

        freeaddrinfo(result);

        const int32_t connectResult = ::connect(m_socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        if (connectResult == 0)
        {
            m_isConnecting = false;
            invokeOnConnect(true);
        }
        else
        {
            const int32_t error = getLastError();
            if (isInProgress(error))
            {
                m_isConnecting = true;
            }
            else
            {
                closeSocket();
                invokeOnConnect(false);
            }
        }
    }

    void NativeTcpSocket::disconnect()
    {
        std::scoped_lock lock(m_socketMutex);
        closeSocket();
        m_isConnecting = false;
        invokeOnDisconnect();
    }

    void NativeTcpSocket::close(int32_t code, const std::string& reason)
    {
        disconnect();
    }

    bool NativeTcpSocket::isConnected() const
    {
        std::scoped_lock lock(m_socketMutex);
        return m_socketHandle != kInvalidSocketHandle && !m_isConnecting;
    }

    void NativeTcpSocket::send(const uint8_t* data, const uint32_t size)
    {
        std::scoped_lock lock(m_socketMutex);

        if (m_socketHandle == kInvalidSocketHandle || m_isConnecting)
        {
            return;
        }

        uint32_t totalSent = 0;
        while (totalSent < size)
        {
            const int32_t sent = sendInternal(data + totalSent, size - totalSent);
            if (sent <= 0)
            {
                break;
            }
            totalSent += static_cast<uint32_t>(sent);
        }
    }

    void NativeTcpSocket::tick()
    {
        {
            std::scoped_lock lock(m_socketMutex);

            if (m_socketHandle == kInvalidSocketHandle)
            {
                return;
            }

            if (m_isConnecting)
            {
                if (checkConnectionProgress())
                {
                    m_isConnecting = false;
                    invokeOnConnect(true);
                }
                return;
            }

            uint8_t buffer[kReceiveBufferSize];
            const int32_t received = receiveInternal(buffer, kReceiveBufferSize);

            if (received > 0)
            {
                m_dataBuffer.insert(m_dataBuffer.end(), buffer, buffer + received);
            }
            else if (received < 0)
            {
                closeSocket();
                m_isConnecting = false;
                invokeOnDisconnect();
                return;
            }
        }

        readPacketsFromBuffer();
    }

    int32_t NativeTcpSocket::sendInternal(const uint8_t* data, uint32_t size) const
    {
#ifdef REACTORMQ_PLATFORM_WINDOWS
        const int32_t sent = ::send(m_socketHandle, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
#else
        const int32_t sent = static_cast<int32_t>(::send(m_socketHandle, data, size, 0));
#endif // REACTORMQ_PLATFORM_WINDOWS

        if (sent < 0)
        {
            const int32_t error = getLastError();
            if (isWouldBlock(error))
            {
                return 0;
            }
            return -1;
        }

        return sent;
    }

    int32_t NativeTcpSocket::receiveInternal(uint8_t* buffer, uint32_t size) const
    {
#ifdef REACTORMQ_PLATFORM_WINDOWS
        const int32_t received = recv(m_socketHandle, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
        const int32_t received = static_cast<int32_t>(::recv(m_socketHandle, buffer, size, 0));
#endif // REACTORMQ_PLATFORM_WINDOWS

        if (received < 0)
        {
            const int32_t error = getLastError();
            if (isWouldBlock(error))
            {
                return 0;
            }
            return -1;
        }

        if (received == 0)
        {
            return -1;
        }

        return received;
    }

    bool NativeTcpSocket::createSocket()
    {
        m_socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (m_socketHandle == kInvalidSocketHandle)
        {
            return false;
        }

        return true;
    }

    void NativeTcpSocket::closeSocket()
    {
        if (m_socketHandle == kInvalidSocketHandle)
        {
            return;
        }

#ifdef REACTORMQ_PLATFORM_WINDOWS
        closesocket(m_socketHandle);
#else
        ::close(m_socketHandle);
#endif // REACTORMQ_PLATFORM_WINDOWS

        m_socketHandle = kInvalidSocketHandle;
    }

    bool NativeTcpSocket::setNonBlocking() const
    {
#ifdef REACTORMQ_PLATFORM_WINDOWS
        u_long mode = 1;
        return ioctlsocket(m_socketHandle, FIONBIO, &mode) == 0;
#else
        const int32_t flags = fcntl(m_socketHandle, F_GETFL, 0);
        if (flags == -1)
        {
            return false;
        }
        return fcntl(m_socketHandle, F_SETFL, flags | O_NONBLOCK) != -1;
#endif // REACTORMQ_PLATFORM_WINDOWS
    }

    bool NativeTcpSocket::setNoDelay() const
    {
        int32_t flag = 1;
#ifdef REACTORMQ_PLATFORM_WINDOWS
        return setsockopt(m_socketHandle, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag)) == 0;
#else
        return setsockopt(m_socketHandle, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == 0;
#endif // REACTORMQ_PLATFORM_WINDOWS
    }

    bool NativeTcpSocket::checkConnectionProgress() const
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_socketHandle, &writeSet);

        fd_set errorSet;
        FD_ZERO(&errorSet);
        FD_SET(m_socketHandle, &errorSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        const int32_t result = select(static_cast<int>(m_socketHandle + 1), nullptr, &writeSet, &errorSet, &timeout);

        if (result > 0)
        {
            if (FD_ISSET(m_socketHandle, &errorSet))
            {
                return false;
            }

            if (FD_ISSET(m_socketHandle, &writeSet))
            {
                int32_t error = 0;
                socklen_t len = sizeof(error);
#ifdef REACTORMQ_PLATFORM_WINDOWS
                getsockopt(m_socketHandle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);
#else
                getsockopt(m_socketHandle, SOL_SOCKET, SO_ERROR, &error, &len);
#endif // REACTORMQ_PLATFORM_WINDOWS
                return error == 0;
            }
        }

        return false;
    }
} // namespace reactormq::socket
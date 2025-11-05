#pragma once

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif // NOMINMAX
#   include <winsock2.h>
#   include <ws2tcpip.h>
using SocketHandle = SOCKET;
static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
#   include <arpa/inet.h>
#   include <netinet/in.h>
#   include <sys/socket.h>
#   include <sys/types.h>
#   include <unistd.h>
using SocketHandle = int;
static constexpr SocketHandle kInvalidSocket = -1;
#endif // _WIN32

namespace reactormq::tests
{
    class EchoServer
    {
    public:
        EchoServer()
            : m_listenSocket(kInvalidSocket)
              , m_clientSocket(kInvalidSocket)
              , m_port(0)
              , m_shouldStop(false)
        {
        }

        ~EchoServer()
        {
            stop();
        }

        uint16_t start(uint16_t port)
        {
#ifdef _WIN32
            static struct WsaInit
            {
                WsaInit()
                {
                    WSADATA wsaData;
                    WSAStartup(MAKEWORD(2, 2), &wsaData);
                }

                ~WsaInit()
                {
                    WSACleanup();
                }
            } wsaInit;
#endif // _WIN32

            stop();

            m_listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
            if (m_listenSocket == kInvalidSocket)
            {
                return 0;
            }

            int enable = 1;
#ifdef _WIN32
            setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &enable, sizeof(enable));
#else
            setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif // _WIN32

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = htons(port);

            if (::bind(m_listenSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
            {
                closeSocket(m_listenSocket);
                m_listenSocket = kInvalidSocket;
                return 0;
            }

            if (::listen(m_listenSocket, 1) != 0)
            {
                closeSocket(m_listenSocket);
                m_listenSocket = kInvalidSocket;
                return 0;
            }

            sockaddr_in sockName{};
            socklen_t sockNameLen = static_cast<socklen_t>(sizeof(sockName));
            if (::getsockname(m_listenSocket, reinterpret_cast<sockaddr *>(&sockName), &sockNameLen) == 0)
            {
                m_port = ntohs(sockName.sin_port);
            } else
            {
                m_port = port;
            }

            m_shouldStop.store(false, std::memory_order_release);
            m_thread = std::thread(&EchoServer::run, this);
            return m_port;
        }

        void stop()
        {
            m_shouldStop.store(true, std::memory_order_release);
            if (m_listenSocket != kInvalidSocket)
            {
#ifdef _WIN32
                ::closesocket(m_listenSocket);
#else
                ::shutdown(m_listenSocket, SHUT_RDWR);
                ::close(m_listenSocket);
#endif // _WIN32
                m_listenSocket = kInvalidSocket;
            }
            if (m_clientSocket != kInvalidSocket)
            {
#ifdef _WIN32
                ::closesocket(m_clientSocket);
#else
                ::shutdown(m_clientSocket, SHUT_RDWR);
                ::close(m_clientSocket);
#endif // _WIN32
                m_clientSocket = kInvalidSocket;
            }
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        uint16_t getPort() const
        {
            return m_port;
        }

    private:
        static void closeSocket(SocketHandle s)
        {
#ifdef _WIN32
            if (s != kInvalidSocket) { ::closesocket(s); }
#else
            if (s != kInvalidSocket) { ::close(s); }
#endif // _WIN32
        }

        void run()
        {
            while (!m_shouldStop.load(std::memory_order_acquire))
            {
                sockaddr_in clientAddr{};
#ifdef _WIN32
                int addrLen = sizeof(clientAddr);
#else
                socklen_t addrLen = sizeof(clientAddr);
#endif // _WIN32
                m_clientSocket = ::accept(m_listenSocket, reinterpret_cast<sockaddr *>(&clientAddr), &addrLen);
                if (m_clientSocket == kInvalidSocket)
                {
                    if (m_shouldStop.load(std::memory_order_acquire))
                    {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                echoLoop(m_clientSocket);
                closeSocket(m_clientSocket);
                m_clientSocket = kInvalidSocket;
            }
        }

        void echoLoop(SocketHandle client)
        {
            constexpr size_t kBufferSize = 64 * 1024;
            std::vector<uint8_t> buffer(kBufferSize);
            for (;;)
            {
#ifdef _WIN32
                int received = ::recv(client, reinterpret_cast<char *>(buffer.data()), static_cast<int>(buffer.size()),
                                      0);
#else
                ssize_t received = ::recv(client, buffer.data(), buffer.size(), 0);
#endif // _WIN32
                if (received <= 0)
                {
                    break;
                }

                const uint8_t* outPtr = buffer.data();
                int remaining = static_cast<int>(received);
                while (remaining > 0)
                {
#ifdef _WIN32
                    int sent = ::send(client, reinterpret_cast<const char *>(outPtr), remaining, 0);
#else
                    ssize_t sent = ::send(client, outPtr, static_cast<size_t>(remaining), 0);
#endif // _WIN32
                    if (sent <= 0)
                    {
                        return;
                    }
                    remaining -= static_cast<int>(sent);
                    outPtr += sent;
                }
            }
        }

        SocketHandle m_listenSocket;
        SocketHandle m_clientSocket;
        uint16_t m_port;
        std::atomic<bool> m_shouldStop;
        std::thread m_thread;
    };
} // namespace reactormq::tests

//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <WS2tcpip.h>
#include <WinSock2.h>
using SocketHandle = SOCKET;
static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
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

        uint16_t start(const uint16_t port)
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

            SocketHandle listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
            if (listenSocket == kInvalidSocket)
            {
                return 0;
            }

            constexpr int enable = 1;
#ifdef _WIN32
            ::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enable), sizeof(enable));
#else
            ::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif // _WIN32

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = htons(port);

            if (::bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
            {
                closeSocket(listenSocket);
                return 0;
            }

            if (::listen(listenSocket, 1) != 0)
            {
                closeSocket(listenSocket);
                return 0;
            }

            m_listenSocket.store(listenSocket, std::memory_order_release);

            sockaddr_in sockName{};
            auto sockNameLen = static_cast<socklen_t>(sizeof(sockName));
            if (::getsockname(listenSocket, reinterpret_cast<sockaddr*>(&sockName), &sockNameLen) == 0)
            {
                m_port = ntohs(sockName.sin_port);
            }
            else
            {
                m_port = port;
            }

            m_shouldStop.store(false, std::memory_order_release);
            m_thread = std::jthread(&EchoServer::run, this);
            return m_port;
        }

        void stop()
        {
            m_shouldStop.store(true, std::memory_order_release);

            const SocketHandle listen = m_listenSocket.load(std::memory_order_acquire);
            if (listen != kInvalidSocket && m_port != 0)
            {
                SocketHandle s = ::socket(AF_INET, SOCK_STREAM, 0);
                if (s != kInvalidSocket)
                {
                    sockaddr_in addr{};
                    addr.sin_family = AF_INET;
                    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                    addr.sin_port = htons(m_port);

                    ::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
                    closeSocket(s);
                }
            }

            if (m_thread.joinable() && std::this_thread::get_id() != m_thread.get_id())
            {
                m_thread.join();
            }
        }

        [[nodiscard]] uint16_t getPort() const
        {
            return m_port;
        }

    private:
        static void setNonBlocking(SocketHandle s)
        {
#ifdef _WIN32
            u_long mode = 1;
            ::ioctlsocket(s, FIONBIO, &mode);
#else
            int flags = ::fcntl(s, F_GETFL, 0);
            ::fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
        }

        static bool isWouldBlock()
        {
#ifdef _WIN32
            return ::WSAGetLastError() == WSAEWOULDBLOCK;
#else
            return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
        }

        static void closeSocket(SocketHandle s)
        {
#ifdef _WIN32
            if (s != kInvalidSocket)
            {
                ::closesocket(s);
            }
#else
            if (s != kInvalidSocket)
            {
                ::shutdown(s, SHUT_RDWR);
                ::close(s);
            }
#endif // _WIN32
        }

        void run()
        {
            const SocketHandle listen = m_listenSocket.load(std::memory_order_acquire);

            while (!m_shouldStop.load(std::memory_order_acquire))
            {
                sockaddr_in clientAddr{};
#ifdef _WIN32
                int addrLen = sizeof(clientAddr);
#else
                socklen_t addrLen = sizeof(clientAddr);
#endif

                if (listen == kInvalidSocket)
                {
                    break;
                }

                const SocketHandle client = ::accept(listen, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

                if (client == kInvalidSocket)
                {
                    if (m_shouldStop.load(std::memory_order_acquire))
                    {
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                m_clientSocket.store(client, std::memory_order_release);

                echoLoop(client);

                closeSocket(client);
                m_clientSocket.store(kInvalidSocket, std::memory_order_release);
            }

            const SocketHandle toClose = m_listenSocket.exchange(kInvalidSocket, std::memory_order_acq_rel);
            closeSocket(toClose);
        }

        void echoLoop(const SocketHandle client)
        {
            setNonBlocking(client);
            constexpr size_t kBufferSize = 64 * 1024;
            std::vector<uint8_t> buffer(kBufferSize);

            while (!m_shouldStop.load(std::memory_order_acquire))
            {
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(client, &readfds);

                timeval tv{};
                tv.tv_usec = 10000; // 10ms

#ifdef _WIN32
                const int ret = ::select(0, &readfds, nullptr, nullptr, &tv);
#else
                const int ret = ::select(client + 1, &readfds, nullptr, nullptr, &tv);
#endif
                if (ret < 0)
                {
                    break;
                }
                if (ret == 0)
                {
                    continue;
                }

                if (FD_ISSET(client, &readfds))
                {
#ifdef _WIN32
                    const int received = ::recv(client, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
                    if (received == 0)
                    {
                        break;
                    }
                    if (received < 0)
                    {
                        if (isWouldBlock())
                        {
                            continue;
                        }
                        break;
                    }
                    int remaining = received;
#else
                    const ssize_t received = ::recv(client, buffer.data(), buffer.size(), 0);
                    if (received == 0)
                    {
                        break;
                    }
                    if (received < 0)
                    {
                        if (errno == EINTR || isWouldBlock())
                        {
                            continue;
                        }
                        break;
                    }
                    int remaining = static_cast<int>(received);
#endif // _WIN32

                    const uint8_t* outPtr = buffer.data();
                    while (remaining > 0)
                    {
#ifdef _WIN32
                        const int sent = ::send(client, reinterpret_cast<const char*>(outPtr), remaining, 0);
                        if (sent < 0)
                        {
                            if (isWouldBlock())
                            {
                                fd_set writefds;
                                FD_ZERO(&writefds);
                                FD_SET(client, &writefds);
                                timeval wtv{};
                                wtv.tv_usec = 10000;
                                ::select(0, nullptr, &writefds, nullptr, &wtv);
                                continue;
                            }
                            return;
                        }
#else
                        const ssize_t sent = ::send(client, outPtr, static_cast<size_t>(remaining), 0);
                        if (sent < 0)
                        {
                            if (errno == EINTR)
                            {
                                continue;
                            }
                            if (isWouldBlock())
                            {
                                fd_set writefds;
                                FD_ZERO(&writefds);
                                FD_SET(client, &writefds);
                                timeval wtv{};
                                wtv.tv_usec = 10000;
                                ::select(client + 1, nullptr, &writefds, nullptr, &wtv);
                                continue;
                            }
                            return;
                        }
#endif // _WIN32
                        remaining -= static_cast<int>(sent);
                        outPtr += sent;
                    }
                }
            }
        }

        std::atomic<SocketHandle> m_listenSocket;
        std::atomic<SocketHandle> m_clientSocket;
        uint16_t m_port;
        std::atomic<bool> m_shouldStop;
        std::jthread m_thread;
    };
} // namespace reactormq::tests
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <optional>
#include <random>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <WS2tcpip.h>
#include <WinSock2.h>
using RmqSocketHandle = SOCKET;
static constexpr RmqSocketHandle kRmqInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using RmqSocketHandle = int;
static constexpr RmqSocketHandle kRmqInvalidSocket = -1;
#endif // _WIN32

namespace reactormq::test
{
    namespace detail
    {
#ifdef _WIN32
        struct WsaInit
        {
            WsaInit()
            {
                WSADATA wsaData{};
                WSAStartup(MAKEWORD(2, 2), &wsaData);
            }

            ~WsaInit()
            {
                WSACleanup();
            }
        };
#endif // _WIN32

        inline void closeSocket(const RmqSocketHandle s)
        {
#ifdef _WIN32
            if (s != kRmqInvalidSocket)
            {
                closesocket(s);
            }
#else
            if (s != kRmqInvalidSocket)
            {
                close(s);
            }
#endif // _WIN32
        }
    } // namespace detail

    /**
     * @brief Try to find a currently free TCP port on localhost within [startPort, endPort].
     * @param startPort the lowest possible port
     * @param endPort the highest possible port
     * @return an available port or std::nullopt if none found.
     */
    inline std::optional<std::uint16_t> findAvailablePort(const std::uint16_t startPort, const std::uint16_t endPort)
    {
        if (startPort == 0 || endPort == 0 || endPort < startPort)
        {
            return std::nullopt;
        }

#ifdef _WIN32
        static detail::WsaInit wsaOnce;
        (void)wsaOnce;
#endif // _WIN32

        const auto range = static_cast<std::uint32_t>(endPort - startPort + 1);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<std::uint32_t> dist(0, range - 1);

        auto candidate = static_cast<std::uint16_t>(startPort + dist(gen));

        for (std::uint32_t attempts = 0; attempts < range; ++attempts)
        {
            const RmqSocketHandle s = ::socket(AF_INET, SOCK_STREAM, 0);
            if (s == kRmqInvalidSocket)
            {
                return std::nullopt;
            }

            int enable = 1;
#ifdef _WIN32
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enable), sizeof(enable));
#else
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif // _WIN32

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = htons(candidate);

            const int bindRes = ::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            detail::closeSocket(s);
            if (bindRes == 0)
            {
                return candidate;
            }

            ++candidate;
            if (candidate > endPort)
            {
                candidate = startPort;
            }
        }

        return std::nullopt;
    }
} // namespace reactormq::test
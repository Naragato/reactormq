#pragma once

#include "socket/socket.h"
#include "socket/platform/platform_socket.h"

#include <chrono>
#include <functional>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

namespace reactormq::tests
{
    class SocketTestEnvironment final : public testing::Environment
    {
    public:
        void SetUp() override
        {
            socket::PlatformSocket::initializeNetworking();
        }

        void TearDown() override
        {
            socket::PlatformSocket::shutdownNetworking();
        }
    };

    inline void waitForCondition(const std::function<bool()>& condition, const int maxAttempts = 100, const int sleepMs = 10)
    {
        for (int i = 0; i < maxAttempts; ++i)
        {
            if (condition())
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }
    }

    inline void tickUntilConnected(const std::shared_ptr<socket::Socket>& sock, const int maxAttempts = 100)
    {
        for (int i = 0; i < maxAttempts; ++i)
        {
            sock->tick();
            if (sock->isConnected())
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    inline std::vector<uint8_t> buildMqttConnectPacket()
    {
        std::vector<uint8_t> packet;
        packet.push_back(0x10);
        packet.push_back(0x0C);
        packet.push_back(0x00);
        packet.push_back(0x04);
        packet.push_back('M');
        packet.push_back('Q');
        packet.push_back('T');
        packet.push_back('T');
        packet.push_back(0x04);
        packet.push_back(0x02);
        packet.push_back(0x00);
        packet.push_back(0x3C);
        packet.push_back(0x00);
        packet.push_back(0x00);
        return packet;
    }
}
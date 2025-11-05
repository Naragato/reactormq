#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

#include "socket/native/native_socket.h"

namespace reactormq::tests
{
    inline void waitForCondition(const std::function<bool()>& condition, int maxAttempts = 100, int sleepMs = 10)
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

    inline void tickUntilConnected(socket::NativeSocket& sock, int maxAttempts = 100)
    {
        for (int i = 0; i < maxAttempts; ++i)
        {
            sock.tick();
            if (sock.isConnected())
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

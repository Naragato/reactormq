//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "fixtures/echo_server.h"
#include "fixtures/test_utils.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/credentials.h"
#include "socket/socket.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace reactormq::socket;
using namespace reactormq::mqtt;
using namespace reactormq::tests;

namespace
{
    class NoOpCredentialsProvider final : public ICredentialsProvider
    {
    public:
        Credentials getCredentials() override
        {
            return Credentials{};
        }
    };
} // namespace

TEST(NativeSocket_MqttFraming, SingleCompletePacket)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    const auto settings
        = ConnectionSettingsBuilder{}
              .setHost("127.0.0.1")
              .setPort(port)
              .setProtocol(ConnectionProtocol::Tcp)
              .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
              .build();

    SocketPtr sock = CreateSocket(settings);

    std::atomic connected{ false };
    std::vector<std::vector<uint8_t>> receivedPackets;
    std::mutex recvMutex;

    auto connectHandle = sock->getOnConnectCallback().add(
        [&connected](const bool success)
        {
            connected.store(success);
        });
    auto dataHandle = sock->getOnDataReceivedCallback().add(
        [&recvMutex, &receivedPackets](const uint8_t* data, const uint32_t size)
        {
            std::scoped_lock lock(recvMutex);
            receivedPackets.emplace_back(data, data + size);
        });

    sock->connect();
    tickUntilConnected(sock, 100);
    ASSERT_TRUE(connected.load());

    const auto packet = buildMqttConnectPacket();
    sock->send(packet.data(), static_cast<uint32_t>(packet.size()));

    for (int i = 0; i < 50; ++i)
    {
        sock->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::scoped_lock lock(recvMutex);
        if (!receivedPackets.empty())
        {
            break;
        }
    }

    {
        std::scoped_lock lock(recvMutex);
        ASSERT_EQ(receivedPackets.size(), 1u);
        EXPECT_EQ(receivedPackets[0].size(), packet.size());
        EXPECT_TRUE(std::memcmp(receivedPackets[0].data(), packet.data(), packet.size()) == 0);
    }

    sock->disconnect();
    server.stop();
}

TEST(NativeSocket_MqttFraming, TwoConsecutivePackets)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    auto settings
        = ConnectionSettingsBuilder{}
              .setHost("127.0.0.1")
              .setPort(port)
              .setProtocol(ConnectionProtocol::Tcp)
              .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
              .build();

    SocketPtr sock = CreateSocket(settings);

    std::atomic connected{ false };
    std::vector<std::vector<uint8_t>> receivedPackets;
    std::mutex recvMutex;

    auto connectHandle = sock->getOnConnectCallback().add(
        [&connected](const bool success)
        {
            connected.store(success);
        });
    auto dataHandle = sock->getOnDataReceivedCallback().add(
        [&recvMutex, &receivedPackets](const uint8_t* data, const uint32_t size)
        {
            std::scoped_lock lock(recvMutex);
            receivedPackets.emplace_back(data, data + size);
        });

    sock->connect();

    tickUntilConnected(sock, 100);
    ASSERT_TRUE(connected.load());

    auto packet1 = buildMqttConnectPacket();
    auto packet2 = buildMqttConnectPacket();

    std::vector<uint8_t> combined;
    combined.insert(combined.end(), packet1.begin(), packet1.end());
    combined.insert(combined.end(), packet2.begin(), packet2.end());
    sock->send(combined.data(), static_cast<uint32_t>(combined.size()));

    for (int i = 0; i < 100; ++i)
    {
        sock->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::scoped_lock lock(recvMutex);
        if (receivedPackets.size() >= 2)
        {
            break;
        }
    }

    {
        std::scoped_lock lock(recvMutex);
        ASSERT_EQ(receivedPackets.size(), 2u);
        EXPECT_EQ(receivedPackets[0].size(), packet1.size());
        EXPECT_EQ(receivedPackets[1].size(), packet2.size());
        EXPECT_TRUE(std::memcmp(receivedPackets[0].data(), packet1.data(), packet1.size()) == 0);
        EXPECT_TRUE(std::memcmp(receivedPackets[1].data(), packet2.data(), packet2.size()) == 0);
    }

    sock->disconnect();
    server.stop();
}

TEST(NativeSocket_MqttFraming, FragmentedPacket)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    const auto settings
        = ConnectionSettingsBuilder{}
              .setHost("127.0.0.1")
              .setPort(port)
              .setProtocol(ConnectionProtocol::Tcp)
              .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
              .build();

    SocketPtr sock = CreateSocket(settings);

    std::atomic connected{ false };
    std::vector<std::vector<uint8_t>> receivedPackets;
    std::mutex recvMutex;

    auto connectHandle = sock->getOnConnectCallback().add(
        [&connected](const bool success)
        {
            connected.store(success);
        });
    auto dataHandle = sock->getOnDataReceivedCallback().add(
        [&recvMutex, &receivedPackets](const uint8_t* data, const uint32_t size)
        {
            std::scoped_lock lock(recvMutex);
            receivedPackets.emplace_back(data, data + size);
        });

    sock->connect();
    tickUntilConnected(sock, 100);
    ASSERT_TRUE(connected.load());

    auto packet = buildMqttConnectPacket();

    constexpr size_t frag1Size = 5;
    constexpr size_t frag2Size = 5;
    const size_t frag3Size = packet.size() - frag1Size - frag2Size;

    sock->send(packet.data(), frag1Size);
    sock->tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    sock->send(packet.data() + frag1Size, frag2Size);
    sock->tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    sock->send(packet.data() + frag1Size + frag2Size, static_cast<uint32_t>(frag3Size));

    for (int i = 0; i < 100; ++i)
    {
        sock->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::scoped_lock lock(recvMutex);
        if (!receivedPackets.empty())
        {
            break;
        }
    }

    {
        std::scoped_lock lock(recvMutex);
        ASSERT_EQ(receivedPackets.size(), 1u);
        EXPECT_EQ(receivedPackets[0].size(), packet.size());
        EXPECT_TRUE(std::memcmp(receivedPackets[0].data(), packet.data(), packet.size()) == 0);
    }

    sock->disconnect();
    server.stop();
}
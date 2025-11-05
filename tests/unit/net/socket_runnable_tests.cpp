#include "fixtures/echo_server.h"
#include "fixtures/socket_runnable.h"
#include "fixtures/test_utils.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/credentials.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using namespace reactormq;
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
}

TEST(SocketRunnable, ConnectsSuccessfully)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    auto settings = ConnectionSettingsBuilder{}
        .setHost("127.0.0.1")
        .setPort(port)
        .setProtocol(ConnectionProtocol::Tcp)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .build();

    std::atomic connected{ false };

    SocketRunnable runnable(settings);
    auto socket = runnable.getSocket();

    auto connectHandle = socket->getOnConnectCallback().add(
        [&](const bool success)
        {
            connected.store(success);
        });
    socket->connect();

    waitForCondition(
        [&]()
        {
            return connected.load();
        });

    EXPECT_TRUE(connected.load());
    EXPECT_TRUE(socket->isConnected());

    runnable.stop();
    server.stop();
}

TEST(SocketRunnable, SendsAndReceivesData)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    auto settings = ConnectionSettingsBuilder{}
        .setHost("127.0.0.1")
        .setPort(port)
        .setProtocol(ConnectionProtocol::Tcp)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .build();

    std::atomic connected{ false };
    std::vector<std::vector<uint8_t>> receivedPackets;
    std::mutex recvMutex;

    SocketRunnable runnable(settings);
    auto socket = runnable.getSocket();

    auto connectHandle = socket->getOnConnectCallback().add(
        [&](const bool success)
        {
            connected.store(success);
        });
    auto dataHandle = socket->getOnDataReceivedCallback().add(
        [&](const uint8_t* data, const uint32_t size)
        {
            std::scoped_lock lock(recvMutex);
            receivedPackets.emplace_back(data, data + size);
        });

    socket->connect();
    waitForCondition(
        [&]()
        {
            return connected.load();
        });
    ASSERT_TRUE(connected.load());

    auto packet = buildMqttConnectPacket();
    socket->send(packet.data(), static_cast<uint32_t>(packet.size()));

    waitForCondition(
        [&]()
        {
            std::scoped_lock lock(recvMutex);
            return !receivedPackets.empty();
        });

    {
        std::scoped_lock lock(recvMutex);
        ASSERT_EQ(receivedPackets.size(), 1u);
        EXPECT_EQ(receivedPackets[0].size(), packet.size());
        EXPECT_TRUE(std::memcmp(receivedPackets[0].data(), packet.data(), packet.size()) == 0);
    }

    runnable.stop();
    server.stop();
}

TEST(SocketRunnable, HandlesMultiplePackets)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    auto settings = ConnectionSettingsBuilder{}
        .setHost("127.0.0.1")
        .setPort(port)
        .setProtocol(ConnectionProtocol::Tcp)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .build();

    std::atomic connected{ false };
    std::vector<std::vector<uint8_t>> receivedPackets;
    std::mutex recvMutex;

    SocketRunnable runnable(settings);
    auto socket = runnable.getSocket();

    auto connectHandle = socket->getOnConnectCallback().add(
        [&](const bool success)
        {
            connected.store(success);
        });
    auto dataHandle = socket->getOnDataReceivedCallback().add(
        [&](const uint8_t* data, const uint32_t size)
        {
            std::scoped_lock lock(recvMutex);
            receivedPackets.emplace_back(data, data + size);
        });

    socket->connect();
    waitForCondition(
        [&]()
        {
            return connected.load();
        });
    ASSERT_TRUE(connected.load());

    auto packet1 = buildMqttConnectPacket();
    auto packet2 = buildMqttConnectPacket();
    auto packet3 = buildMqttConnectPacket();

    socket->send(packet1.data(), static_cast<uint32_t>(packet1.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    socket->send(packet2.data(), static_cast<uint32_t>(packet2.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    socket->send(packet3.data(), static_cast<uint32_t>(packet3.size()));

    waitForCondition(
        [&]()
        {
            std::scoped_lock lock(recvMutex);
            return receivedPackets.size() >= 3;
        }, 200);

    {
        std::scoped_lock lock(recvMutex);
        ASSERT_EQ(receivedPackets.size(), 3u);
        EXPECT_EQ(receivedPackets[0].size(), packet1.size());
        EXPECT_EQ(receivedPackets[1].size(), packet2.size());
        EXPECT_EQ(receivedPackets[2].size(), packet3.size());
    }

    runnable.stop();
    server.stop();
}

TEST(SocketRunnable, DisconnectsCleanly)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    auto settings = ConnectionSettingsBuilder{}
        .setHost("127.0.0.1")
        .setPort(port)
        .setProtocol(ConnectionProtocol::Tcp)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .build();

    std::atomic connected{ false };
    std::atomic disconnected{ false };

    SocketRunnable runnable(settings);
    auto socket = runnable.getSocket();

    auto connectHandle = socket->getOnConnectCallback().add(
        [&](const bool success)
        {
            connected.store(success);
        });
    auto disconnectHandle = socket->getOnDisconnectCallback().add(
        [&]()
        {
            disconnected.store(true);
        });

    socket->connect();
    waitForCondition(
        [&]()
        {
            return connected.load();
        });
    ASSERT_TRUE(connected.load());

    socket->disconnect();
    waitForCondition(
        [&]()
        {
            return disconnected.load();
        });

    EXPECT_TRUE(disconnected.load());
    EXPECT_FALSE(socket->isConnected());

    runnable.stop();
    server.stop();
}

TEST(SocketRunnable, StopsThreadAutomatically)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    auto settings = ConnectionSettingsBuilder{}
        .setHost("127.0.0.1")
        .setPort(port)
        .setProtocol(ConnectionProtocol::Tcp)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .build();

    std::atomic connected{ false };

    SocketRunnable runnable(settings);
    auto socket = runnable.getSocket();

    auto connectHandle = socket->getOnConnectCallback().add(
        [&](const bool success)
        {
            connected.store(success);
        });
    socket->connect();

    waitForCondition(
        [&]()
        {
            return connected.load();
        });
    ASSERT_TRUE(connected.load());
    EXPECT_TRUE(runnable.isRunning());

    socket->disconnect();
    waitForCondition(
        [&]()
        {
            return !runnable.isRunning();
        }, 200);

    EXPECT_FALSE(runnable.isRunning());

    server.stop();
}
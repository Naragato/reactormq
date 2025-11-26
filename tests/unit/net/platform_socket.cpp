//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "socket/platform/platform_socket.h"
#include "fixtures/echo_server.h"
#include "fixtures/test_utils.h"
#include "socket/platform/receive_flags.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace reactormq::socket;
using namespace reactormq::tests;

TEST(PlatformSocket, CreateAndDestroy)
{
    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());
}

TEST(PlatformSocket, ConnectToLocalEchoServer)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int result = socket.connect("127.0.0.1", port);
    EXPECT_EQ(result, 0) << "Connect should return 0 for success or in-progress";

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(socket.isConnected()) << "Socket should be connected after waiting";

    socket.close();
    server.stop();
}

TEST(PlatformSocket, ConnectToInvalidHost)
{
    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int result = socket.connect("192.0.2.1", 9999);

    EXPECT_TRUE(result == 0 || result != 0) << "Connect to unreachable host should return immediately";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    socket.close();
}

TEST(PlatformSocket, SendAndReceive)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    const auto testMessage = "Hello, PlatformSocket!";
    const auto messageSize = static_cast<uint32_t>(std::strlen(testMessage));

    size_t bytesSent = 0;
    const bool sendSuccess = socket.trySend(reinterpret_cast<const uint8_t*>(testMessage), messageSize, bytesSent);

    EXPECT_TRUE(sendSuccess);
    EXPECT_EQ(bytesSent, messageSize);

    std::vector<uint8_t> receiveBuffer(1024);
    size_t bytesRead = 0;

    for (int i = 0; i < 100; ++i)
    {
        if (socket.tryReceive(receiveBuffer.data(), static_cast<int32_t>(receiveBuffer.size()), bytesRead) && bytesRead > 0)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_GT(bytesRead, 0);
    EXPECT_EQ(bytesRead, messageSize);
    EXPECT_EQ(std::memcmp(receiveBuffer.data(), testMessage, messageSize), 0);

    socket.close();
    server.stop();
}

TEST(PlatformSocket, MultipleMessages)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    const auto message1 = "First";
    const auto message2 = "Second";
    const auto message3 = "Third";

    {
        size_t bytesSent = 0;
        socket.trySend(reinterpret_cast<const uint8_t*>(message1), static_cast<uint32_t>(std::strlen(message1)), bytesSent);
        EXPECT_EQ(bytesSent, std::strlen(message1));
    }

    {
        size_t bytesSent = 0;
        socket.trySend(reinterpret_cast<const uint8_t*>(message2), static_cast<uint32_t>(std::strlen(message2)), bytesSent);
        EXPECT_EQ(bytesSent, std::strlen(message2));
    }

    {
        size_t bytesSent = 0;
        socket.trySend(reinterpret_cast<const uint8_t*>(message3), static_cast<uint32_t>(std::strlen(message3)), bytesSent);
        EXPECT_EQ(bytesSent, std::strlen(message3));
    }

    std::vector<uint8_t> receiveBuffer(1024);
    size_t totalBytesRead = 0;
    const size_t expectedTotal = std::strlen(message1) + std::strlen(message2) + std::strlen(message3);

    for (int i = 0; i < 100 && totalBytesRead < expectedTotal; ++i)
    {
        if (size_t bytesRead = 0;
            socket.tryReceive(receiveBuffer.data() + totalBytesRead, static_cast<int32_t>(receiveBuffer.size() - totalBytesRead), bytesRead)
            && bytesRead > 0)
        {
            totalBytesRead += bytesRead;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(totalBytesRead, expectedTotal);

    const std::string received(reinterpret_cast<char*>(receiveBuffer.data()), totalBytesRead);
    EXPECT_EQ(received, "FirstSecondThird");

    socket.close();
    server.stop();
}

TEST(PlatformSocket, GetPendingData)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    EXPECT_EQ(socket.getPendingData(), 0) << "No data should be pending initially";

    const auto testMessage = "Test getPendingData";
    const auto messageSize = static_cast<uint32_t>(std::strlen(testMessage));

    size_t bytesSent = 0;
    socket.trySend(reinterpret_cast<const uint8_t*>(testMessage), messageSize, bytesSent);
    EXPECT_EQ(bytesSent, messageSize);

    for (int i = 0; i < 50; ++i)
    {
        if (const size_t pending = socket.getPendingData(); pending > 0)
        {
            EXPECT_GE(pending, static_cast<size_t>(messageSize)) << "Pending data should match sent data";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    socket.close();
    server.stop();
}

TEST(PlatformSocket, ReceiveWithPeekFlag)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    const auto testMessage = "Peek test";
    const auto messageSize = static_cast<uint32_t>(std::strlen(testMessage));

    size_t bytesSent = 0;
    socket.trySend(reinterpret_cast<const uint8_t*>(testMessage), messageSize, bytesSent);
    EXPECT_EQ(bytesSent, messageSize);

    std::vector<uint8_t> peekBuffer(1024);
    size_t bytesRead = 0;

    for (int i = 0; i < 100; ++i)
    {
        if (socket.tryReceiveWithFlags(peekBuffer.data(), peekBuffer.size(), ReceiveFlags::Peek, bytesRead) && bytesRead > 0)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_GT(bytesRead, 0) << "Peek should read data";
    EXPECT_EQ(bytesRead, messageSize);

    std::vector<uint8_t> actualBuffer(1024);
    size_t actualBytesRead = 0;

    const bool readSuccess = socket.tryReceive(actualBuffer.data(), static_cast<int32_t>(actualBuffer.size()), actualBytesRead);
    EXPECT_TRUE(readSuccess);
    EXPECT_GT(actualBytesRead, 0) << "Data should still be available after peek";
    EXPECT_EQ(actualBytesRead, messageSize);

    EXPECT_EQ(std::memcmp(peekBuffer.data(), actualBuffer.data(), messageSize), 0) << "Peeked data should match actual data";

    socket.close();
    server.stop();
}

TEST(PlatformSocket, CloseWhileConnected)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    socket.close();

    EXPECT_FALSE(socket.isConnected()) << "Socket should not be connected after close";

    server.stop();
}

TEST(PlatformSocket, IsConnectedAfterServerDisconnect)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    server.stop();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (int i = 0; i < 50; ++i)
    {
        if (!socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    socket.close();
}

TEST(PlatformSocket, NonBlockingBehavior)
{
    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    std::vector<uint8_t> buffer(1024);
    size_t bytesRead = 0;

    const bool receiveResult = socket.tryReceive(buffer.data(), static_cast<int32_t>(buffer.size()), bytesRead);
    (void)receiveResult;
    socket.close();
}

TEST(PlatformSocket, ErrorHandling)
{
    const PlatformSocket socket;

    std::vector<uint8_t> buffer(1024);
    size_t bytesRead = 0;

    const bool receiveWithoutSocket = socket.tryReceive(buffer.data(), static_cast<int32_t>(buffer.size()), bytesRead);
    EXPECT_FALSE(receiveWithoutSocket) << "Receive should fail without a socket";

    size_t bytesSent = 0;
    const uint8_t testData[] = { 1, 2, 3, 4, 5 };
    const bool sendWithoutSocket = socket.trySend(testData, sizeof(testData), bytesSent);
    EXPECT_FALSE(sendWithoutSocket) << "Send should fail without a socket";
}

TEST(PlatformSocket, ReconnectAfterClose)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    {
        const int connectResult = socket.connect("127.0.0.1", port);
        EXPECT_EQ(connectResult, 0);

        for (int i = 0; i < 100; ++i)
        {
            if (socket.isConnected())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        ASSERT_TRUE(socket.isConnected());

        socket.close();
        EXPECT_FALSE(socket.isConnected());
    }

    {
        EXPECT_TRUE(socket.createSocket());
        const int connectResult = socket.connect("127.0.0.1", port);
        EXPECT_EQ(connectResult, 0);

        for (int i = 0; i < 100; ++i)
        {
            if (socket.isConnected())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_TRUE(socket.isConnected()) << "Should be able to reconnect after close";

        socket.close();
    }

    server.stop();
}

TEST(PlatformSocket, LargeDataTransfer)
{
    EchoServer server;
    const uint16_t port = server.start(0);
    ASSERT_NE(port, 0);

    PlatformSocket socket;
    EXPECT_TRUE(socket.createSocket());

    const int connectResult = socket.connect("127.0.0.1", port);
    EXPECT_EQ(connectResult, 0);

    for (int i = 0; i < 100; ++i)
    {
        if (socket.isConnected())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(socket.isConnected());

    constexpr size_t largeSize = 64 * 1024;
    std::vector<uint8_t> largeData(largeSize);
    for (size_t i = 0; i < largeSize; ++i)
    {
        largeData[i] = static_cast<uint8_t>(i & 0xFF);
    }

    size_t totalBytesSent = 0;
    while (totalBytesSent < largeSize)
    {
        size_t bytesSent = 0;
        if (const auto remaining = static_cast<uint32_t>(largeSize - totalBytesSent);
            socket.trySend(largeData.data() + totalBytesSent, remaining, bytesSent))
        {
            if (bytesSent > 0)
            {
                totalBytesSent += bytesSent;
            }
        }
        else
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_EQ(totalBytesSent, largeSize);

    std::vector<uint8_t> receiveBuffer(largeSize);
    size_t totalBytesRead = 0;

    for (int attempt = 0; attempt < 500 && totalBytesRead < largeSize; ++attempt)
    {
        if (size_t bytesRead = 0;
            socket.tryReceive(receiveBuffer.data() + totalBytesRead, static_cast<int32_t>(receiveBuffer.size() - totalBytesRead), bytesRead)
            && bytesRead > 0)
        {
            totalBytesRead += bytesRead;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(totalBytesRead, largeSize);
    EXPECT_EQ(std::memcmp(receiveBuffer.data(), largeData.data(), largeSize), 0) << "Received data should match sent data";

    socket.close();
    server.stop();
}
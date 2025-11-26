//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "socket/platform/platform_socket.h"

#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

using namespace reactormq::socket;

namespace
{
    constexpr std::chrono::milliseconds kTestTimeoutMs{ 20000 };
    constexpr std::chrono::milliseconds kPollSleepMs{ 50 };
} // namespace

TEST(PlatformSocket_Tcp, ConnectAndHttpGet_ExampleDotCom)
{
    PlatformSocket sock;
    ASSERT_TRUE(sock.createSocket());

    const int connectResult = sock.connect("example.com", 80);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate TCP connect to example.com:80";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    while (std::chrono::steady_clock::now() < deadline && !sock.isConnected())
    {
        std::this_thread::sleep_for(kPollSleepMs);
    }

    if (!sock.isConnected())
    {
        GTEST_SKIP() << "TCP connection did not complete within timeout. Network may be unavailable.";
    }

    const auto httpRequest
        = "GET / HTTP/1.1\r\n"
          "Host: example.com\r\n"
          "Connection: close\r\n"
          "\r\n";

    size_t bytesSent = 0;
    const bool sendOk
        = sock.trySend(reinterpret_cast<const std::uint8_t*>(httpRequest), static_cast<std::uint32_t>(std::strlen(httpRequest)), bytesSent);
    ASSERT_TRUE(sendOk);
    ASSERT_GT(bytesSent, 0);

    std::vector<std::uint8_t> response;
    while (std::chrono::steady_clock::now() < deadline)
    {
        std::uint8_t buffer[4096];
        size_t bytesRead = 0;
        if (sock.tryReceive(buffer, sizeof(buffer), bytesRead) && bytesRead > 0)
        {
            response.insert(response.end(), buffer, buffer + bytesRead);
        }
        else
        {
            if (!response.empty())
            {
                break;
            }
        }

        std::this_thread::sleep_for(kPollSleepMs);
    }

    ASSERT_FALSE(response.empty()) << "No HTTP response from example.com over TCP";

    const std::string responseText(reinterpret_cast<const char*>(response.data()), response.size());
    EXPECT_NE(responseText.find("HTTP/1.1 200"), std::string::npos);
    EXPECT_NE(responseText.find("Example Domain"), std::string::npos);

    sock.close();
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "socket/platform/platform_socket.h"

#if REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS
#include "socket/platform/platform_secure_socket.h"
#endif

#include "reactormq/mqtt/connection_settings_builder.h"

#include <array>
#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>

using namespace reactormq::socket;
using namespace reactormq::mqtt;

#if REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS

namespace
{
    constexpr std::chrono::milliseconds kTestTimeoutMs{ 20000 };
    constexpr std::chrono::milliseconds kPollSleepMs{ 50 };
} // namespace

TEST(PlatformSecureSocket_Tls, TlsConnectionToPublicEndpoint)
{
    const auto settings
        = ConnectionSettingsBuilder().setHost("www.google.com").setPort(443).setShouldVerifyServerCertificate(false).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("www.google.com", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate TLS connection";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            connected = true;
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    if (connected)
    {
        EXPECT_TRUE(sock.isConnected()) << "TLS connection to public endpoint failed. "
                                           "This may indicate cert verification issues or network problems.";

        const auto httpRequest = "GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n";
        size_t bytesSent = 0;
        const bool sendSuccess = sock.trySend(
            reinterpret_cast<const std::uint8_t*>(httpRequest),
            static_cast<std::uint32_t>(std::strlen(httpRequest)),
            bytesSent);

        EXPECT_TRUE(sendSuccess) << "Failed to send HTTP request over TLS";
        EXPECT_GT(bytesSent, 0) << "No bytes sent over TLS connection";

        std::array<std::uint8_t, 1024> receiveBuffer;
        size_t bytesRead = 0;
        while (std::chrono::steady_clock::now() < deadline)
        {
            constexpr int receiveBufferSize = receiveBuffer.size();
            if (sock.tryReceive(receiveBuffer.data(), receiveBufferSize, bytesRead) && bytesRead > 0)
            {
                break;
            }
            std::this_thread::sleep_for(kPollSleepMs);
        }

        REACTORMQ_LOG(reactormq::logging::LogLevel::Info, "%d", bytesRead);
        EXPECT_GT(bytesRead, 0) << "No response received over TLS connection";

        sock.close();
    }
    else
    {
        GTEST_SKIP() << "TLS handshake did not complete within timeout. Network may be unavailable.";
    }
}

TEST(PlatformSecureSocket_Tls, TlsWithoutVerificationToPublicEndpoint)
{
    const auto settings
        = ConnectionSettingsBuilder().setHost("www.google.com").setPort(443).setShouldVerifyServerCertificate(false).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("www.google.com", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate TLS connection without verification";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            connected = true;
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    if (connected)
    {
        EXPECT_TRUE(sock.isConnected()) << "TLS connection without verification failed.";

        const auto httpRequest = "GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n";
        size_t bytesSent = 0;
        const bool sendSuccess = sock.trySend(
            reinterpret_cast<const std::uint8_t*>(httpRequest),
            static_cast<std::uint32_t>(std::strlen(httpRequest)),
            bytesSent);

        EXPECT_TRUE(sendSuccess) << "Failed to send HTTP request over TLS (no verification)";
        EXPECT_GT(bytesSent, 0) << "No bytes sent over TLS connection (no verification)";

        sock.close();
    }
    else
    {
        GTEST_SKIP() << "TLS handshake did not complete within timeout. Network may be unavailable.";
    }
}

TEST(PlatformSecureSocket_Tls, HttpBinUuidEndpoint)
{
    const auto settings = ConnectionSettingsBuilder().setHost("httpbin.org").setPort(443).setShouldVerifyServerCertificate(true).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("httpbin.org", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate TLS connection to httpbin.org";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            connected = true;
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    if (!connected)
    {
        GTEST_SKIP() << "TLS handshake did not complete within timeout. Network may be unavailable.";
    }

    EXPECT_TRUE(sock.isConnected());

    const auto httpRequest
        = "GET /uuid HTTP/1.1\r\n"
          "Host: httpbin.org\r\n"
          "Connection: close\r\n"
          "\r\n";

    size_t bytesSent = 0;
    const bool sendSuccess
        = sock.trySend(reinterpret_cast<const std::uint8_t*>(httpRequest), static_cast<std::uint32_t>(std::strlen(httpRequest)), bytesSent);

    EXPECT_TRUE(sendSuccess) << "Failed to send HTTP request to httpbin.org";
    EXPECT_GT(bytesSent, 0);

    std::vector<std::uint8_t> responseBuffer;
    while (std::chrono::steady_clock::now() < deadline)
    {
        std::array<std::uint8_t, 2048> buffer;
        size_t bytesRead = 0;
        constexpr int receiveBufferSize = buffer.size();
        if (sock.tryReceive(buffer.data(), receiveBufferSize, bytesRead) && bytesRead > 0)
        {
            responseBuffer.insert(responseBuffer.end(), buffer.data(), buffer.data() + bytesRead);
        }

        if (bytesRead == 0 && !responseBuffer.empty())
        {
            break;
        }

        std::this_thread::sleep_for(kPollSleepMs);
    }

    ASSERT_GT(responseBuffer.size(), 0) << "No response received from httpbin.org";

    const std::string response(reinterpret_cast<const char*>(responseBuffer.data()), responseBuffer.size());

    EXPECT_TRUE(response.find("HTTP/1.1 200") != std::string::npos) << "Expected HTTP 200 response";
    EXPECT_TRUE(response.find("\"uuid\":") != std::string::npos) << "Expected JSON with uuid field";

    sock.close();
}

TEST(PlatformSecureSocket_Tls, ExampleDotComKnownContent)
{
    const auto settings = ConnectionSettingsBuilder().setHost("example.com").setPort(443).setShouldVerifyServerCertificate(true).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("example.com", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate TLS connection to example.com";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    EXPECT_TRUE(sock.isConnected());

    const auto httpRequest
        = "GET / HTTP/1.1\r\n"
          "Host: example.com\r\n"
          "Connection: close\r\n"
          "\r\n";

    size_t bytesSent = 0;
    sock.trySend(reinterpret_cast<const std::uint8_t*>(httpRequest), static_cast<std::uint32_t>(std::strlen(httpRequest)), bytesSent);

    EXPECT_GT(bytesSent, 0);

    std::vector<std::uint8_t> responseBuffer;
    while (std::chrono::steady_clock::now() < deadline)
    {
        std::array<std::uint8_t, 2048> buffer;
        size_t bytesRead = 0;
        constexpr int receiveBufferSize = buffer.size();
        if (sock.tryReceive(buffer.data(), receiveBufferSize, bytesRead) && bytesRead > 0)
        {
            responseBuffer.insert(responseBuffer.end(), buffer.data(), buffer.data() + bytesRead);
        }

        std::this_thread::sleep_for(kPollSleepMs);

        if (bytesRead == 0 && !responseBuffer.empty())
        {
            break;
        }
    }

    ASSERT_GT(responseBuffer.size(), 0) << "No response received from example.com";

    const std::string response(reinterpret_cast<const char*>(responseBuffer.data()), responseBuffer.size());

    EXPECT_TRUE(response.find("Example Domain") != std::string::npos) << "Expected 'Example Domain' in response";

    sock.close();
}

TEST(PlatformSecureSocket_Tls, ExpiredCertificateRejected)
{
    const auto settings
        = ConnectionSettingsBuilder().setHost("expired.badssl.com").setPort(443).setShouldVerifyServerCertificate(true).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("expired.badssl.com", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate connection to expired.badssl.com";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            connected = true;
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    EXPECT_FALSE(connected) << "Should NOT connect with expired certificate when verification is enabled";

    sock.close();
}

TEST(PlatformSecureSocket_Tls, SelfSignedCertificateAcceptedWithoutVerification)
{
    const auto settings
        = ConnectionSettingsBuilder().setHost("self-signed.badssl.com").setPort(443).setShouldVerifyServerCertificate(false).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("self-signed.badssl.com", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate connection to self-signed.badssl.com";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            connected = true;
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    if (!connected)
    {
        GTEST_SKIP() << "Connection failed. Network may be unavailable.";
    }

    EXPECT_TRUE(connected) << "Should connect with self-signed certificate when verification is disabled";

    sock.close();
}

TEST(PlatformSecureSocket_Tls, HttpBinBytesEndpointLargeTransfer)
{
    const auto settings = ConnectionSettingsBuilder().setHost("httpbin.org").setPort(443).setShouldVerifyServerCertificate(true).build();
    PlatformSecureSocket sock(settings);

    const int connectResult = sock.connect("httpbin.org", 443);
    ASSERT_EQ(connectResult, 0) << "Failed to initiate TLS connection to httpbin.org";

    const auto deadline = std::chrono::steady_clock::now() + kTestTimeoutMs;
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sock.isConnected())
        {
            connected = true;
            break;
        }

        std::array<std::uint8_t, 64> dummyBuffer;
        size_t bytesRead = 0;
        constexpr int dummyBufferSize = dummyBuffer.size();
        sock.tryReceive(dummyBuffer.data(), dummyBufferSize, bytesRead);

        std::this_thread::sleep_for(kPollSleepMs);
    }

    if (!connected)
    {
        GTEST_SKIP() << "TLS handshake did not complete within timeout. Network may be unavailable.";
    }

    EXPECT_TRUE(sock.isConnected());

    const auto httpRequest
        = "GET /bytes/8192 HTTP/1.1\r\n"
          "Host: httpbin.org\r\n"
          "Connection: close\r\n"
          "\r\n";

    size_t bytesSent = 0;
    const bool sendSuccess
        = sock.trySend(reinterpret_cast<const std::uint8_t*>(httpRequest), static_cast<std::uint32_t>(std::strlen(httpRequest)), bytesSent);

    EXPECT_TRUE(sendSuccess);
    EXPECT_GT(bytesSent, 0);

    std::vector<std::uint8_t> responseBuffer;
    while (std::chrono::steady_clock::now() < deadline)
    {
        std::array<std::uint8_t, 4096> buffer;
        size_t bytesRead = 0;
        constexpr int receiveBufferSize = 4096;
        if (sock.tryReceive(buffer.data(), receiveBufferSize, bytesRead) && bytesRead > 0)
        {
            responseBuffer.insert(responseBuffer.end(), buffer.data(), buffer.data() + bytesRead);
        }

        if (bytesRead == 0 && !responseBuffer.empty())
        {
            break;
        }

        std::this_thread::sleep_for(kPollSleepMs);
    }

    ASSERT_GT(responseBuffer.size(), 0) << "No response received from httpbin.org";

    const std::string response(reinterpret_cast<const char*>(responseBuffer.data()), responseBuffer.size());

    EXPECT_TRUE(response.find("HTTP/1.1 200") != std::string::npos) << "Expected HTTP 200 response";
    EXPECT_GT(responseBuffer.size(), 8192) << "Expected at least 8192 bytes in response (headers + body)";

    sock.close();
}

#else

TEST(PlatformSecureSocket_Tls, OpenSslNotAvailable)
{
    GTEST_SKIP() << "PlatformSecureSocket not available; TLS tests skipped.";
}

#endif
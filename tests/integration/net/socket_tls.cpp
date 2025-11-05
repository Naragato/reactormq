#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/credentials.h"
#include "socket/native/native_socket.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

using namespace reactormq::socket;
using namespace reactormq::mqtt;

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

#ifdef REACTORMQ_WITH_OPENSSL

TEST(NativeSocket_Tls, TlsConnectionToPublicEndpoint)
{
    const auto settings = ConnectionSettingsBuilder("www.google.com")
        .setPort(443)
        .setProtocol(ConnectionProtocol::Tls)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .setShouldVerifyServerCertificate(true)
        .setSocketConnectionTimeoutSeconds(10)
        .build();

    NativeSocket sock(settings);

    std::atomic connectResult{ false };
    std::atomic connectCalled{ false };
    auto connectHandle = sock.getOnConnectCallback().add(
        [&](const bool success)
        {
            connectResult.store(success);
            connectCalled.store(true);
        });

    sock.connect();

    for (int i = 0; i < 200; ++i)
    {
        sock.tick();
        if (connectCalled.load())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (connectCalled.load())
    {
        EXPECT_TRUE(connectResult.load()) << "TLS connection to public endpoint failed. "
                                              "This may indicate cert verification issues or network problems.";
        if (sock.isConnected())
        {
            sock.disconnect();
            for (int i = 0; i < 20; ++i)
            {
                sock.tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    else
    {
        GTEST_SKIP() << "Connect callback not invoked within timeout. Network may be unavailable.";
    }
}

TEST(NativeSocket_Tls, TlsWithoutVerificationToPublicEndpoint)
{
    const auto settings = ConnectionSettingsBuilder("www.google.com")
        .setPort(443)
        .setProtocol(ConnectionProtocol::Tls)
        .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
        .setShouldVerifyServerCertificate(false)
        .setSocketConnectionTimeoutSeconds(10)
        .build();

    NativeSocket sock(settings);

    std::atomic connectResult{ false };
    std::atomic connectCalled{ false };
    auto connectHandle = sock.getOnConnectCallback().add(
        [&](const bool success)
        {
            connectResult.store(success);
            connectCalled.store(true);
        });

    sock.connect();

    for (int i = 0; i < 200; ++i)
    {
        sock.tick();
        if (connectCalled.load())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (connectCalled.load())
    {
        EXPECT_TRUE(connectResult.load()) << "TLS connection without verification failed.";
        if (sock.isConnected())
        {
            sock.disconnect();
            for (int i = 0; i < 20; ++i)
            {
                sock.tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    else
    {
        GTEST_SKIP() << "Connect callback not invoked within timeout. Network may be unavailable.";
    }
}

#else

TEST(NativeSocket_Tls, OpenSslNotAvailable)
{
    GTEST_SKIP() << "OpenSSL not available; TLS tests skipped.";
}

#endif // REACTORMQ_WITH_OPENSSL
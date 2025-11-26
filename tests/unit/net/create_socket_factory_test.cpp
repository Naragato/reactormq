//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/credentials.h"
#include "reactormq/mqtt/credentials_provider.h"
#include "socket/socket.h"

#include <gtest/gtest.h>

using namespace reactormq::mqtt;
using namespace reactormq::socket;

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

    ConnectionSettingsPtr makeSettings(const ConnectionProtocol protocol)
    {
        return ConnectionSettingsBuilder{}
            .setHost("127.0.0.1")
            .setPort(protocol == ConnectionProtocol::Ws || protocol == ConnectionProtocol::Wss ? 80 : 1883)
            .setProtocol(protocol)
            .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
            .build();
    }
} // namespace

namespace
{
    const char* expectedImplId(const ConnectionProtocol protocol)
    {
        switch (protocol)
        {
            using enum ConnectionProtocol;
        case Tcp:
        case Tls:
            {
#if REACTORMQ_WITH_O3DE
                return "O3deSocket";
#elif REACTORMQ_WITH_UE5
                return "Ue5Socket";
#else
                return "SecureSocket";
#endif // REACTORMQ_WITH_O3DE or REACTORMQ_WITH_UE5
            }
        case Ws:
        case Wss:
            {
#ifdef REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
                return "Ue5WebSocket";
#else
                return "SecureSocket";
#endif // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
            }
        default:
            return "Unknown";
        }
    }
} // namespace

TEST(SocketFactory_CreateSocket, Tcp_ReturnsExpectedType)
{
    const auto settings = makeSettings(ConnectionProtocol::Tcp);
    const auto sock = CreateSocket(settings);
    ASSERT_NE(sock, nullptr);
    EXPECT_STREQ(sock->getImplementationId(), expectedImplId(ConnectionProtocol::Tcp));
}

TEST(SocketFactory_CreateSocket, Tls_ReturnsExpectedType)
{
    const auto settings = makeSettings(ConnectionProtocol::Tls);
    const auto sock = CreateSocket(settings);
    ASSERT_NE(sock, nullptr);
    EXPECT_STREQ(sock->getImplementationId(), expectedImplId(ConnectionProtocol::Tls));
}

TEST(SocketFactory_CreateSocket, Ws_ReturnsExpectedType)
{
    const auto settings = makeSettings(ConnectionProtocol::Ws);
    const auto sock = CreateSocket(settings);
    ASSERT_NE(sock, nullptr);
    EXPECT_STREQ(sock->getImplementationId(), expectedImplId(ConnectionProtocol::Ws));
}

TEST(SocketFactory_CreateSocket, Wss_ReturnsExpectedType)
{
    const auto settings = makeSettings(ConnectionProtocol::Wss);
    const auto sock = CreateSocket(settings);
    ASSERT_NE(sock, nullptr);
    EXPECT_STREQ(sock->getImplementationId(), expectedImplId(ConnectionProtocol::Wss));
}
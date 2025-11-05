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
        return ConnectionSettingsBuilder{}.setHost("127.0.0.1").setPort(
                protocol == ConnectionProtocol::Ws || protocol == ConnectionProtocol::Wss ? 80 : 1883).setProtocol(protocol).
            setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>()).build();
    }
}

namespace
{
    const char* expectedImplId(const ConnectionProtocol protocol)
    {
        switch (protocol)
        {
        case ConnectionProtocol::Tcp:
        case ConnectionProtocol::Tls: {
#if REACTORMQ_WITH_O3DE_SOCKET
            return "O3deSocket";
#elif  REACTORMQ_WITH_FSOCKET_UE5
            return "Ue5Socket";
#else
            return "SecureSocket";
#endif // REACTORMQ_WITH_O3DE_SOCKET or REACTORMQ_WITH_FSOCKET_UE5
        }
        case ConnectionProtocol::Ws:
        case ConnectionProtocol::Wss: {
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
}

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
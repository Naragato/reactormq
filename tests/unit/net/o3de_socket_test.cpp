#if REACTORMQ_WITH_O3DE_SOCKET

#include <gtest/gtest.h>
#include "socket/o3de/o3de_socket.h"
#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/connection_settings_builder.h"

using namespace reactormq::socket;using namespace reactormq::mqtt;class O3deSocketTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};TEST_F(O3deSocketTest, ConstructorCreatesTcpSocketForMqttProtocol)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    ASSERT_NE(socket, nullptr);
    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, ConstructorCreatesTlsSocketForMqttsProtocol)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(8883).setProtocol(ConnectionProtocol::Tls).setClientId("test-client-tls");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    ASSERT_NE(socket, nullptr);
    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, InitialStateIsDisconnected)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, SendWhenDisconnectedDoesNotCrash)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    const uint8_t testData[] = { 0x10, 0x02, 0x00, 0x04 };

    socket->send(testData, sizeof(testData));

    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, TickWhenDisconnectedDoesNotCrash)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    socket->tick();

    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, CloseWhenDisconnectedDoesNotCrash)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    socket->close(1000, "test");

    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, DisconnectWhenDisconnectedDoesNotCrash)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    socket->disconnect();

    EXPECT_FALSE(socket->isConnected());
}TEST_F(O3deSocketTest, CallbacksCanBeSet)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-client");

    auto settings = builder.build();
    auto socket = std::make_unique<O3deSocket>(settings);

    bool connectCallbackCalled = false;
    bool disconnectCallbackCalled = false;
    bool dataReceivedCallbackCalled = false;

    auto connectHandle = socket->getOnConnectCallback().add(
        [&connectCallbackCalled](bool success)
        {
            connectCallbackCalled = true;
        });

    auto disconnectHandle = socket->getOnDisconnectCallback().add(
        [&disconnectCallbackCalled]()
        {
            disconnectCallbackCalled = true;
        });

    auto dataHandle = socket->getOnDataReceivedCallback().add(
        [&dataReceivedCallbackCalled](const uint8_t* data, uint32_t size)
        {
            dataReceivedCallbackCalled = true;
        });

    EXPECT_FALSE(connectCallbackCalled);
    EXPECT_FALSE(disconnectCallbackCalled);
    EXPECT_FALSE(dataReceivedCallbackCalled);
}TEST_F(O3deSocketTest, SocketSupportsAllConnectionProtocols)
{
    {
        ConnectionSettingsBuilder builder;
        builder.setHost("localhost").setPort(1883).setProtocol(ConnectionProtocol::Tcp).setClientId("test-mqtt");

        auto socket = std::make_unique<O3deSocket>(builder.build());
        EXPECT_NE(socket, nullptr);
    }

    {
        ConnectionSettingsBuilder builder;
        builder.setHost("localhost").setPort(8883).setProtocol(ConnectionProtocol::Tls).setClientId("test-mqtts");

        auto socket = std::make_unique<O3deSocket>(builder.build());
        EXPECT_NE(socket, nullptr);
    }
}

#endif // REACTORMQ_WITH_O3DE_SOCKET
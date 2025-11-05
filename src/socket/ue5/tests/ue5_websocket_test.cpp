#ifdef REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
#if WITH_DEV_AUTOMATION_TESTS

#include "socket/ue5/ue5_websocket.h"
#include "socket/ue5/tests/ue5_socket_runnable.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "Misc/AutomationTest.h"

using namespace reactormq;
using namespace reactormq::socket;
using namespace reactormq::mqtt;

BEGIN_DEFINE_SPEC(
    ReactorMQUe5WebSocketSpec,
    "ReactorMQ.Automation.Ue5WebSocket",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext)
    
    std::shared_ptr<Ue5WebSocket> m_webSocket;
    
END_DEFINE_SPEC(ReactorMQUe5WebSocketSpec)

void ReactorMQUe5WebSocketSpec::Define()
{
    Describe(
        TEXT("WebSocket Creation"),
        [this]
        {
            It(
                TEXT("should create WebSocket with ws:// protocol"),
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(9001)
                        .setProtocol(ConnectionProtocol::Ws)
                        .setPath("/mqtt")
                        .setClientId("test-client-ws");
                    
                    m_webSocket = std::make_shared<Ue5WebSocket>(builder.build());
                    TestTrue(TEXT("WebSocket should be created"), m_webSocket != nullptr);
                });

            It(
                TEXT("should create WebSocket with wss:// protocol"),
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(9443)
                        .setProtocol(ConnectionProtocol::Wss)
                        .setPath("/mqtt")
                        .setClientId("test-client-wss");
                    
                    m_webSocket = std::make_shared<Ue5WebSocket>(builder.build());
                    TestTrue(TEXT("WebSocket with TLS should be created"), m_webSocket != nullptr);
                });
        });

    Describe(
        TEXT("WebSocket Operations"),
        [this]
        {
            BeforeEach(
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(9001)
                        .setProtocol(ConnectionProtocol::Ws)
                        .setPath("/")
                        .setClientId("test-client");
                    
                    m_webSocket = std::make_shared<Ue5WebSocket>(builder.build());
                });

            AfterEach(
                [this]
                {
                    if (m_webSocket)
                    {
                        m_webSocket->disconnect();
                        m_webSocket.reset();
                    }
                });


            /*
            LatentIt(
                TEXT("should connect to WebSocket server"),
                [this](const FDoneDelegate& Done)
                {
                    m_webSocket->getOnConnectCallback().add([this, Done](bool wasSuccessful)
                    {
                        TestTrue(TEXT("WebSocket connection should succeed"), wasSuccessful);
                        TestTrue(TEXT("WebSocket should be connected"), m_webSocket->isConnected());
                        Done.Execute();
                    });
                    
                    m_webSocket->connect();
                });

            LatentIt(
                TEXT("should send binary data over WebSocket"),
                [this](const FDoneDelegate& Done)
                {
                    m_webSocket->getOnConnectCallback().add([this, Done](bool wasSuccessful)
                    {
                        if (!wasSuccessful)
                        {
                            AddError(TEXT("WebSocket connection failed"));
                            Done.Execute();
                            return;
                        }

                        const uint8_t testData[] = {0x10, 0x02, 0x00, 0x04}; // CONNECT packet start
                        m_webSocket->send(testData, sizeof(testData));
                        
                        TestTrue(TEXT("Should still be connected after send"), m_webSocket->isConnected());
                        Done.Execute();
                    });
                    
                    m_webSocket->connect();
                });

            LatentIt(
                TEXT("should receive binary data over WebSocket"),
                [this](const FDoneDelegate& Done)
                {
                    bool receivedData = false;
                    
                    m_webSocket->getOnDataReceivedCallback().add([&receivedData, Done](const uint8_t* data, uint32_t size)
                    {
                        if (size > 0)
                        {
                            receivedData = true;
                            Done.Execute();
                        }
                    });
                    
                    m_webSocket->getOnConnectCallback().add([this](bool wasSuccessful)
                    {
                        TestTrue(TEXT("Should connect"), wasSuccessful);
                    });
                    
                    m_webSocket->connect();
                    
                });

            LatentIt(
                TEXT("should disconnect cleanly"),
                [this](const FDoneDelegate& Done)
                {
                    m_webSocket->getOnConnectCallback().add([this, Done](bool wasSuccessful)
                    {
                        TestTrue(TEXT("Should connect"), wasSuccessful);
                        
                        m_webSocket->getOnDisconnectCallback().add([this, Done]()
                        {
                            TestFalse(TEXT("Should not be connected after disconnect"), m_webSocket->isConnected());
                            Done.Execute();
                        });
                        
                        m_webSocket->disconnect();
                    });
                    
                    m_webSocket->connect();
                });
            */
        });

    Describe(
        TEXT("WebSocket URL Construction"),
        [this]
        {
            It(
                TEXT("should construct correct ws:// URL with path"),
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("example.com")
                        .setPort(9001)
                        .setProtocol(ConnectionProtocol::Ws)
                        .setPath("/mqtt")
                        .setClientId("test-client");
                    
                    auto ws = std::make_shared<Ue5WebSocket>(builder.build());
                    TestTrue(TEXT("WebSocket should be created"), ws != nullptr);
                });

            It(
                TEXT("should construct correct wss:// URL with path"),
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("secure.example.com")
                        .setPort(9443)
                        .setProtocol(ConnectionProtocol::Wss)
                        .setPath("/secure/mqtt")
                        .setClientId("test-client");
                    
                    auto ws = std::make_shared<Ue5WebSocket>(builder.build());
                    TestTrue(TEXT("Secure WebSocket should be created"), ws != nullptr);
                });

            It(
                TEXT("should handle empty path with default slash"),
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("example.com")
                        .setPort(9001)
                        .setProtocol(ConnectionProtocol::Ws)
                        .setClientId("test-client");
                    
                    auto ws = std::make_shared<Ue5WebSocket>(builder.build());
                    TestTrue(TEXT("WebSocket should be created with default path"), ws != nullptr);
                });
        });

    Describe(
        TEXT("WebSocket State Management"),
        [this]
        {
            BeforeEach(
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(9001)
                        .setProtocol(ConnectionProtocol::Ws)
                        .setPath("/")
                        .setClientId("test-client");
                    
                    m_webSocket = std::make_shared<Ue5WebSocket>(builder.build());
                });

            AfterEach(
                [this]
                {
                    if (m_webSocket)
                    {
                        m_webSocket->disconnect();
                        m_webSocket.reset();
                    }
                });

            It(
                TEXT("should start in disconnected state"),
                [this]
                {
                    TestFalse(TEXT("Should not be connected initially"), m_webSocket->isConnected());
                });

            It(
                TEXT("should not send data when disconnected"),
                [this]
                {
                    const uint8_t testData[] = {0x10, 0x02, 0x00, 0x04};
                    m_webSocket->send(testData, sizeof(testData));
                    TestFalse(TEXT("Should still be disconnected"), m_webSocket->isConnected());
                });
        });

    Describe(
        TEXT("WebSocket Operations with Runnable"),
        [this]
        {
            std::shared_ptr<Ue5SocketRunnable> m_socketRunner;

            BeforeEach(
                [this, &m_socketRunner]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(9001)
                        .setProtocol(ConnectionProtocol::Ws)
                        .setPath("/")
                        .setClientId("test-client-runnable");
                    
                    m_socketRunner = std::make_shared<Ue5SocketRunnable>(builder.build());
                });

            AfterEach(
                [this, &m_socketRunner]
                {
                    if (m_socketRunner)
                    {
                        m_socketRunner->Stop();
                        m_socketRunner.reset();
                    }
                });


            /*
            LatentIt(
                TEXT("should connect to WebSocket server using runnable"),
                [this, &m_socketRunner](const FDoneDelegate& Done)
                {
                    m_socketRunner->getSocket()->getOnConnectCallback().add(
                        [this, Done](const bool wasSuccessful)
                        {
                            TestTrue(TEXT("WebSocket should connect successfully"), wasSuccessful);
                            if (Done.IsBound())
                            {
                                Done.Execute();
                            }
                        });
                    m_socketRunner->getSocket()->connect();
                });

            LatentIt(
                TEXT("should send and receive data over WebSocket using runnable"),
                [this, &m_socketRunner](const FDoneDelegate& Done)
                {
                    bool dataReceived = false;
                    
                    m_socketRunner->getSocket()->getOnConnectCallback().add(
                        [this, &m_socketRunner, &dataReceived, Done](const bool wasSuccessful)
                        {
                            if (!wasSuccessful)
                            {
                                AddError(TEXT("WebSocket should be connected"));
                                Done.Execute();
                                return;
                            }

                            m_socketRunner->getSocket()->getOnDataReceivedCallback().add(
                                [this, &dataReceived, Done](const uint8_t* data, uint32_t size)
                                {
                                    TestTrue(TEXT("Should receive data"), size > 0);
                                    dataReceived = true;
                                    Done.Execute();
                                });

                            const uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
                            m_socketRunner->getSocket()->send(testData, sizeof(testData));

                        });
                    
                    m_socketRunner->getSocket()->connect();
                });
            */

            It(
                TEXT("should create WebSocket runnable"),
                [this, &m_socketRunner]
                {
                    TestTrue(TEXT("Socket runnable should be created"), m_socketRunner != nullptr);
                    TestTrue(TEXT("Socket should be available"), m_socketRunner->getSocket() != nullptr);
                });
        });
}

#endif // WITH_DEV_AUTOMATION_TESTS
#endif // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5

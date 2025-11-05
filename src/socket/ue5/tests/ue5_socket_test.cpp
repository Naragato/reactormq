#ifdef REACTORMQ_WITH_FSOCKET_UE5
#if WITH_DEV_AUTOMATION_TESTS

#include "socket/ue5/ue5_socket.h"
#include "socket/ue5/tests/ue5_socket_runnable.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "Misc/AutomationTest.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

using namespace reactormq;
using namespace reactormq::socket;
using namespace reactormq::mqtt;

BEGIN_DEFINE_SPEC(
    ReactorMQUe5SocketSpec,
    "ReactorMQ.Automation.Ue5Socket",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext)
    
    std::shared_ptr<Ue5Socket> m_socket;
    FSocket* m_listeningSocket;
    FSocket* m_acceptedSocket;
    uint16_t m_testPort;
    
    static uint16_t findAvailablePort(uint16_t minPort, uint16_t maxPort)
    {
        ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        for (uint16_t port = minPort; port < maxPort; ++port)
        {
            const TSharedPtr<FInternetAddr> addr = socketSubsystem->CreateInternetAddr();
            addr->SetPort(port);
            FSocket* testSocket = socketSubsystem->CreateSocket(NAME_Stream, TEXT("PortTest"), false);
            if (testSocket && testSocket->Bind(*addr))
            {
                testSocket->Close();
                socketSubsystem->DestroySocket(testSocket);
                return port;
            }
            if (testSocket)
            {
                socketSubsystem->DestroySocket(testSocket);
            }
        }
        return 0;
    }

END_DEFINE_SPEC(ReactorMQUe5SocketSpec)

void ReactorMQUe5SocketSpec::Define()
{
    Describe(
        TEXT("TCP Socket"),
        [this]
        {
            BeforeEach(
                [this]
                {
                    m_testPort = findAvailablePort(10000, 32000);
                    TestTrue(TEXT("Should find available port"), m_testPort > 0);

                    ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
                    const TSharedPtr<FInternetAddr> addr = socketSubsystem->CreateInternetAddr();
                    addr->SetPort(m_testPort);
                    m_listeningSocket = socketSubsystem->CreateSocket(NAME_Stream, TEXT("TestListeningSocket"), false);
                    m_listeningSocket->SetNonBlocking(true);
                    
                    bool bound = false;
                    for (int i = 0; i < 100 && !bound; ++i)
                    {
                        bound = m_listeningSocket->Bind(*addr);
                        if (!bound)
                        {
                            FPlatformProcess::Sleep(0.01f);
                        }
                    }
                    
                    TestTrue(TEXT("Should bind listening socket"), bound);
                    TestTrue(TEXT("Should start listening"), m_listeningSocket->Listen(5));

                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(m_testPort)
                        .setProtocol(ConnectionProtocol::Mqtt)
                        .setClientId("test-client");
                    
                    m_socket = std::make_shared<Ue5Socket>(builder.build());
                    m_acceptedSocket = nullptr;
                });

            AfterEach(
                [this]
                {
                    if (m_socket)
                    {
                        m_socket->disconnect();
                        m_socket.reset();
                    }
                    
                    if (m_acceptedSocket)
                    {
                        m_acceptedSocket->Close();
                        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_acceptedSocket);
                        m_acceptedSocket = nullptr;
                    }
                    
                    if (m_listeningSocket)
                    {
                        m_listeningSocket->Close();
                        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_listeningSocket);
                        m_listeningSocket = nullptr;
                    }
                });

            LatentIt(
                TEXT("should connect to server successfully"),
                [this](const FDoneDelegate& Done)
                {
                    m_socket->getOnConnectCallback().add([this, Done](bool wasSuccessful)
                    {
                        TestTrue(TEXT("Connection should succeed"), wasSuccessful);
                        TestTrue(TEXT("Socket should be connected"), m_socket->isConnected());
                        Done.Execute();
                    });
                    
                    m_socket->connect();
                });

            LatentIt(
                TEXT("should send and receive data"),
                [this](const FDoneDelegate& Done)
                {
                    m_socket->getOnConnectCallback().add([this, Done](bool wasSuccessful)
                    {
                        if (!wasSuccessful)
                        {
                            AddError(TEXT("Connection failed"));
                            Done.Execute();
                            return;
                        }

                        bool hasPendingConnection = false;
                        for (int i = 0; i < 100 && !hasPendingConnection; ++i)
                        {
                            m_listeningSocket->WaitForPendingConnection(hasPendingConnection, 0.01f);
                        }
                        
                        if (!hasPendingConnection)
                        {
                            AddError(TEXT("No pending connection"));
                            Done.Execute();
                            return;
                        }

                        m_acceptedSocket = m_listeningSocket->Accept(TEXT("AcceptedSocket"));
                        TestTrue(TEXT("Should accept connection"), m_acceptedSocket != nullptr);

                        const uint8_t testData[] = {0x10, 0x02, 0x00, 0x04};
                        m_socket->send(testData, sizeof(testData));

                        uint8_t recvBuffer[256];
                        int32_t bytesRead = 0;
                        bool received = false;
                        
                        for (int i = 0; i < 100 && !received; ++i)
                        {
                            m_acceptedSocket->Recv(recvBuffer, sizeof(recvBuffer), bytesRead, ESocketReceiveFlags::None);
                            if (bytesRead > 0)
                            {
                                received = true;
                                break;
                            }
                            FPlatformProcess::Sleep(0.01f);
                        }

                        TestTrue(TEXT("Should receive data on server"), received);
                        TestEqual(TEXT("Should receive correct data size"), bytesRead, static_cast<int32_t>(sizeof(testData)));
                        
                        if (received && bytesRead == sizeof(testData))
                        {
                            for (int32_t i = 0; i < bytesRead; ++i)
                            {
                                TestEqual(FString::Printf(TEXT("Byte %d should match"), i), recvBuffer[i], testData[i]);
                            }
                        }

                        Done.Execute();
                    });
                    
                    m_socket->connect();
                });

            LatentIt(
                TEXT("should disconnect cleanly"),
                [this](const FDoneDelegate& Done)
                {
                    m_socket->getOnConnectCallback().add([this, Done](bool wasSuccessful)
                    {
                        TestTrue(TEXT("Should connect"), wasSuccessful);
                        
                        m_socket->getOnDisconnectCallback().add([this, Done]()
                        {
                            TestFalse(TEXT("Should not be connected after disconnect"), m_socket->isConnected());
                            Done.Execute();
                        });
                        
                        m_socket->disconnect();
                    });
                    
                    m_socket->connect();
                });
        });

    Describe(
        TEXT("TCP Socket with Runnable"),
        [this]
        {
            std::shared_ptr<Ue5SocketRunnable> m_socketRunner;
            FSocket* m_listeningSocket;
            FSocket* m_acceptedSocket;
            uint16_t m_testPort;

            BeforeEach(
                [this, &m_socketRunner, &m_listeningSocket, &m_acceptedSocket, &m_testPort]
                {
                    m_testPort = findAvailablePort(10000, 32000);
                    TestTrue(TEXT("Should find available port"), m_testPort > 0);

                    ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
                    const TSharedPtr<FInternetAddr> addr = socketSubsystem->CreateInternetAddr();
                    addr->SetPort(m_testPort);
                    m_listeningSocket = socketSubsystem->CreateSocket(NAME_Stream, TEXT("TestListeningSocket"), false);
                    m_listeningSocket->SetNonBlocking(true);
                    
                    bool bound = false;
                    for (int i = 0; i < 100 && !bound; ++i)
                    {
                        bound = m_listeningSocket->Bind(*addr);
                        if (!bound)
                        {
                            FPlatformProcess::Sleep(0.01f);
                        }
                    }
                    
                    TestTrue(TEXT("Should bind listening socket"), bound);
                    TestTrue(TEXT("Should start listening"), m_listeningSocket->Listen(5));

                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(m_testPort)
                        .setProtocol(ConnectionProtocol::Mqtt)
                        .setClientId("test-client-runnable");
                    
                    m_socketRunner = std::make_shared<Ue5SocketRunnable>(builder.build());
                    m_acceptedSocket = nullptr;
                });

            AfterEach(
                [this, &m_socketRunner, &m_listeningSocket, &m_acceptedSocket]
                {
                    if (m_socketRunner)
                    {
                        m_socketRunner->Stop();
                        m_socketRunner.reset();
                    }
                    
                    if (m_acceptedSocket)
                    {
                        m_acceptedSocket->Close();
                        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_acceptedSocket);
                        m_acceptedSocket = nullptr;
                    }
                    
                    if (m_listeningSocket)
                    {
                        m_listeningSocket->Close();
                        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_listeningSocket);
                        m_listeningSocket = nullptr;
                    }
                });

            LatentIt(
                TEXT("should connect to server successfully using runnable"),
                [this, &m_socketRunner](const FDoneDelegate& Done)
                {
                    m_socketRunner->getSocket()->getOnConnectCallback().add(
                        [this, Done](const bool wasSuccessful)
                        {
                            TestTrue(TEXT("Socket should connect successfully"), wasSuccessful);
                            if (Done.IsBound())
                            {
                                Done.Execute();
                            }
                        });
                    m_socketRunner->getSocket()->connect();
                });

            LatentIt(
                TEXT("should send data to server successfully using runnable"),
                [this, &m_socketRunner, &m_listeningSocket, &m_acceptedSocket](const FDoneDelegate& Done)
                {
                    m_socketRunner->getSocket()->getOnConnectCallback().add(
                        [this, &m_listeningSocket, &m_acceptedSocket, &m_socketRunner, Done](const bool wasSuccessful)
                        {
                            if (!wasSuccessful)
                            {
                                AddError(TEXT("Socket should be connected to the server"));
                                Done.Execute();
                                return;
                            }

                            bool hasPendingConnection = false;
                            m_listeningSocket->WaitForPendingConnection(hasPendingConnection, 5.0f);
                            TestTrue(
                                TEXT("Listening socket should have a pending connection"),
                                hasPendingConnection);

                            if (!hasPendingConnection)
                            {
                                AddError(TEXT("Listening socket should have a pending connection"));
                                Done.Execute();
                                return;
                            }

                            m_acceptedSocket = m_listeningSocket->Accept(TEXT("AcceptedSocket"));
                            m_acceptedSocket->SetNonBlocking(false);
                            TestTrue(TEXT("Client socket should not be null"), m_acceptedSocket != nullptr);

                            const uint8_t testData[] = {0x01, 0x02, 0x03};
                            const uint32_t dataSize = sizeof(testData);

                            m_socketRunner->getSocket()->send(testData, dataSize);

                            uint8_t receivedData[dataSize];
                            int32_t bytesRead = 0;
                            bool dataReceived = false;
                            const FTimespan waitTime = FTimespan::FromSeconds(5.0f);
                            const FDateTime endTime = FDateTime::Now() + waitTime;

                            while (FDateTime::Now() < endTime)
                            {
                                const bool dataAvailable = m_acceptedSocket->Wait(
                                    ESocketWaitConditions::WaitForRead,
                                    FTimespan::FromMilliseconds(100));
                                if (dataAvailable)
                                {
                                    const bool recvSuccessful = m_acceptedSocket->Recv(
                                        receivedData,
                                        dataSize,
                                        bytesRead,
                                        ESocketReceiveFlags::WaitAll);
                                    if (recvSuccessful && bytesRead > 0)
                                    {
                                        dataReceived = true;
                                        break;
                                    }
                                }
                                FPlatformProcess::Sleep(0.01f);
                            }

                            TestTrue(TEXT("Data should be received"), dataReceived);
                            if (dataReceived)
                            {
                                TestEqual(
                                    TEXT("Number of bytes received should match"),
                                    bytesRead,
                                    static_cast<int32_t>(dataSize));
                                for (uint32_t i = 0; i < dataSize && i < static_cast<uint32_t>(bytesRead); ++i)
                                {
                                    TestEqual(
                                        FString::Printf(TEXT("Received data byte %d should match"), i),
                                        receivedData[i],
                                        testData[i]);
                                }
                            }

                            Done.Execute();
                        });
                    m_socketRunner->getSocket()->connect();
                });
        });

    Describe(
        TEXT("TLS Socket"),
        [this]
        {
            It(
                TEXT("should create TLS socket"),
                [this]
                {
                    ConnectionSettingsBuilder builder;
                    builder.setHost("localhost")
                        .setPort(8883)
                        .setProtocol(ConnectionProtocol::Mqtts)
                        .setClientId("test-client-tls");
                    
                    auto tlsSocket = std::make_shared<Ue5Socket>(builder.build());
                    TestTrue(TEXT("TLS socket should be created"), tlsSocket != nullptr);
                });
        });
}

#endif // WITH_DEV_AUTOMATION_TESTS
#endif // REACTORMQ_WITH_FSOCKET_UE5

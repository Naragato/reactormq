//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Timespan.h"
#include "ReactorMQ.h"
#include "ReactorMQClientPool.h"
#include "IReactorMQClient.h"
#include "ReactorMQConnectionSettings.h"
#include "ReactorMQTopicFilter.h"
#include "ReactorMQMessage.h"
#include "LogReactorMQ.h"

#include "port_utils.h"
#include "docker_container.h"

BEGIN_DEFINE_SPEC(
    FReactorMQClientIntegrationTest,
    "ReactorMQ.Integration.ClientIntegration",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext)

    TSharedPtr<IReactorMQClient> Client;
    TUniquePtr<reactormq::test::DockerContainer> Container;
    uint16 BrokerPort = 0;
    FString TestTopic = TEXT("reactormq/test/topic");
    bool bReceivedMessage = false;
    TArray<uint8> ReceivedPayload;

    void TeardownBroker();

END_DEFINE_SPEC(FReactorMQClientIntegrationTest)

void FReactorMQClientIntegrationTest::Define()
{
    Describe(
        "MQTT Client Integration", [this]()
        {
            BeforeEach(
                [this]()
                {
                    bReceivedMessage = false;
                    ReceivedPayload.Empty();
                });

            LatentBeforeEach(
                FTimespan::FromSeconds(180), [this](const FDoneDelegate& BeforeDone)
                {
                    if (!reactormq::test::DockerContainer::isDockerAvailable())
                    {
                        AddWarning(TEXT("Docker is not available. Skipping integration tests."));
                        BeforeDone.Execute();
                        return;
                    }

                    auto OptPort = reactormq::test::findAvailablePort(5000, 10000);
                    if (!OptPort.has_value())
                    {
                        AddError(TEXT("Could not find available port for MQTT broker"));
                        BeforeDone.Execute();
                        return;
                    }

                    BrokerPort = OptPort.value();

                    std::string RunArgs =
                        "-e DOCKER_VERNEMQ_ACCEPT_EULA=yes -e DOCKER_VERNEMQ_ALLOW_ANONYMOUS=on -e DOCKER_VERNEMQ_LOG__CONSOLE__LEVEL=debug";
                    Container = MakeUnique<reactormq::test::DockerContainer>(
                        "reactormq_vernemq_test",
                        "vernemq/vernemq",
                        BrokerPort,
                        1883,
                        RunArgs
                        );

                    auto StartFuture = Container->startAsync(std::chrono::seconds(120));

                    bool bStarted = StartFuture.get();

                    if (bStarted)
                    {
                        UE_LOG(LogReactorMQ, Log, TEXT("MQTT broker started on port %d"), BrokerPort);
                    }
                    else
                    {
                        AddError(TEXT("Failed to start MQTT broker container"));
                    }

                    BeforeDone.Execute();
                });

            LatentAfterEach(
                FTimespan::FromSeconds(10), [this](const FDoneDelegate& AfterDone)
                {
                    if (Client.IsValid() && Client->IsConnected())
                    {
                        Client->DisconnectAsync().Next(
                            [this, AfterDone](const TReactorMQResult<void>& Result)
                            {
                                Client.Reset();
                                TeardownBroker();
                                AfterDone.Execute();
                            });
                    }
                    else
                    {
                        Client.Reset();
                        TeardownBroker();
                        AfterDone.Execute();
                    }
                });

            LatentIt(
                "Should connect to MQTT broker", FTimespan::FromSeconds(30), [this](const FDoneDelegate& TestDone)
                {
                    if (!Container.IsValid() || !Container->isRunning())
                    {
                        AddWarning(TEXT("Broker not running, skipping test"));
                        TestDone.Execute();
                        return;
                    }

                    FReactorMQConnectionSettings Settings;
                    Settings.ClientId = TEXT("ReactorMQTestClient");
                    Settings.Host = TEXT("localhost");
                    Settings.Port = BrokerPort;
                    Settings.Protocol = EReactorMQConnectionProtocol::Tcp;
                    Settings.SocketConnectionTimeoutSeconds = 10;
                    Settings.MqttConnectionTimeoutSeconds = 10;

                    Client = FReactorMQModule::Get().GetClientPool().GetOrCreateClient(Settings);

                    if (!Client.IsValid())
                    {
                        AddError(TEXT("Failed to create client"));
                        TestDone.Execute();
                        return;
                    }

                    Client->ConnectAsync(true).Next(
                        [this, TestDone](const TReactorMQResult<void>& Result)
                        {
                            if (!Result.HasSucceeded())
                            {
                                AddError(TEXT("Failed to connect"));
                            }
                            else
                            {
                                UE_LOG(LogReactorMQ, Log, TEXT("Successfully connected to MQTT broker"));
                            }
                            TestDone.Execute();
                        });
                });

            LatentIt(
                "Should publish and receive messages", FTimespan::FromSeconds(100), [this](const FDoneDelegate& TestDone)
                {
                    if (!Container.IsValid() || !Container->isRunning())
                    {
                        AddWarning(TEXT("Broker not running, skipping test"));
                        TestDone.Execute();
                        return;
                    }

                    FReactorMQConnectionSettings Settings;
                    Settings.ClientId = TEXT("ReactorMQTestClient_PubSub");
                    Settings.Host = TEXT("localhost");
                    Settings.Port = BrokerPort;
                    Settings.Protocol = EReactorMQConnectionProtocol::Tcp;
                    Settings.SocketConnectionTimeoutSeconds = 10;
                    Settings.MqttConnectionTimeoutSeconds = 10;

                    Client = FReactorMQModule::Get().GetClientPool().GetOrCreateClient(Settings);

                    if (!Client.IsValid())
                    {
                        AddError(TEXT("Failed to create client"));
                        TestDone.Execute();
                        return;
                    }

                    Client->OnMessage().AddLambda(
                        [this, TestDone](const FReactorMQMessage& Message)
                        {
                            if (Message.Topic != TestTopic)
                            {
                                return; // Ignore messages on other topics
                            }

                            if (bReceivedMessage)
                            {
                                return; // Already handled
                            }

                            bReceivedMessage = true;
                            ReceivedPayload = Message.Payload;
                            UE_LOG(LogReactorMQ, Log, TEXT("Received message on topic: %s"), *Message.Topic);

                            TestTrue(TEXT("Should have received message"), bReceivedMessage);
                            TestDone.Execute();
                        });

                    Client->ConnectAsync(true).Next(
                        [this, TestDone](const TReactorMQResult<void>& ConnectResult)
                        {
                            if (!ConnectResult.HasSucceeded())
                            {
                                AddError(TEXT("Failed to connect"));
                                TestDone.Execute();
                                return;
                            }

                            FReactorMQTopicFilter Filter;
                            Filter.Filter = TestTopic;
                            Filter.QualityOfService = EReactorMQQualityOfService::AtLeastOnce;
                            Filter.bNoLocal = false;

                            Client->SubscribeAsync(Filter).Next(
                                [this, TestDone](const TReactorMQResult<FReactorMQSubscribeResult>& SubscribeResult)
                                {
                                    if (!SubscribeResult.HasSucceeded())
                                    {
                                        AddError(TEXT("Failed to subscribe"));
                                        TestDone.Execute();
                                        return;
                                    }

                                    UE_LOG(LogReactorMQ, Log, TEXT("Successfully subscribed to topic: %s"), *TestTopic);

                                    TArray<uint8> Payload;
                                    FString TestMessage = TEXT("Hello from ReactorMQ!");
                                    Payload.Append(reinterpret_cast<const uint8*>(TCHAR_TO_UTF8(*TestMessage)), TestMessage.Len());

                                    Client->PublishAsync(TestTopic, Payload, EReactorMQQualityOfService::AtLeastOnce, false).Next(
                                        [this, TestDone](const TReactorMQResult<void>& PublishResult)
                                        {
                                            if (!PublishResult.HasSucceeded())
                                            {
                                                AddError(TEXT("Failed to publish"));
                                                TestDone.Execute();
                                                return;
                                            }

                                            UE_LOG(LogReactorMQ, Log, TEXT("Successfully published message to topic: %s"), *TestTopic);
                                        });
                                });
                        });
                });

            LatentIt(
                "Should unsubscribe from topic", FTimespan::FromSeconds(30), [this](const FDoneDelegate& TestDone)
                {
                    if (!Container.IsValid() || !Container->isRunning())
                    {
                        AddWarning(TEXT("Broker not running, skipping test"));
                        TestDone.Execute();
                        return;
                    }

                    FReactorMQConnectionSettings Settings;
                    Settings.ClientId = TEXT("ReactorMQTestClient_Unsub");
                    Settings.Host = TEXT("localhost");
                    Settings.Port = BrokerPort;
                    Settings.Protocol = EReactorMQConnectionProtocol::Tcp;
                    Settings.SocketConnectionTimeoutSeconds = 10;
                    Settings.MqttConnectionTimeoutSeconds = 10;

                    Client = FReactorMQModule::Get().GetClientPool().GetOrCreateClient(Settings);

                    if (!Client.IsValid())
                    {
                        AddError(TEXT("Failed to create client"));
                        TestDone.Execute();
                        return;
                    }

                    Client->ConnectAsync(true).Next(
                        [this, TestDone](const TReactorMQResult<void>& ConnectResult)
                        {
                            if (!ConnectResult.HasSucceeded())
                            {
                                AddError(TEXT("Failed to connect"));
                                TestDone.Execute();
                                return;
                            }

                            FReactorMQTopicFilter Filter;
                            Filter.Filter = TestTopic;
                            Filter.QualityOfService = EReactorMQQualityOfService::AtMostOnce;

                            Client->SubscribeAsync(Filter).Next(
                                [this, TestDone](const TReactorMQResult<FReactorMQSubscribeResult>& SubscribeResult)
                                {
                                    if (!SubscribeResult.HasSucceeded())
                                    {
                                        AddError(TEXT("Failed to subscribe"));
                                        TestDone.Execute();
                                        return;
                                    }

                                    Client->UnsubscribeAsync(TestTopic).Next(
                                        [this, TestDone](const TReactorMQResult<FReactorMQUnsubscribeResult>& UnsubscribeResult)
                                        {
                                            TestTrue(TEXT("Unsubscribe should succeed"), UnsubscribeResult.HasSucceeded());
                                            if (UnsubscribeResult.HasSucceeded())
                                            {
                                                UE_LOG(LogReactorMQ, Log, TEXT("Successfully unsubscribed from topic: %s"), *TestTopic);
                                            }
                                            TestDone.Execute();
                                        });
                                });
                        });
                });
        });
}

void FReactorMQClientIntegrationTest::TeardownBroker()
{
    if (Container.IsValid())
    {
        Container->stop();
        Container.Reset();
        UE_LOG(LogReactorMQ, Log, TEXT("MQTT broker stopped"));
    }
}

#endif // WITH_DEV_AUTOMATION_TESTS
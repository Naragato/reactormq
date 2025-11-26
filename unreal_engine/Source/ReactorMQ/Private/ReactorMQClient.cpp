//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "ReactorMQClient.h"
#include "ReactorMQTypeMappings.h"
#include "ReactorMQBuildConfig.h"

#include "Async/Async.h"
#include "reactormq/mqtt/client.h"
#include "reactormq/mqtt/client_factory.h"
#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/credentials_provider.h"
#include "reactormq/mqtt/message.h"

#include <functional>
#include <string>
#include <vector>

FReactorMQClient::FReactorMQClient(const FReactorMQConnectionSettings& InSettings)
    : Settings(InSettings)
{
    reactormq::mqtt::CallbackExecutor CallbackExec;

    if (GReactorMQThreadMode == EReactorMQThreadMode::BackgroundThreadMarshalled)
    {
        CallbackExec = [](std::function<void()> Callback)
        {
            AsyncTask(ENamedThreads::GameThread, MoveTemp(Callback));
        };
    }
    else
    {
        CallbackExec = nullptr;
    }

    auto CredentialsProvider = FReactorMQTypeMappings::ToNativeCredentialsProvider(Settings);

    auto NativeSettings = std::make_shared<reactormq::mqtt::ConnectionSettings>(
        TCHAR_TO_UTF8(*Settings.Host),
        Settings.Port,
        FReactorMQTypeMappings::ToNative(Settings.Protocol),
        CredentialsProvider,
        TCHAR_TO_UTF8(*Settings.Path),
        TCHAR_TO_UTF8(*Settings.ClientId),
        Settings.MaxPacketSize,
        Settings.MaxBufferSize,
        Settings.MaxOutboundQueueBytes,
        Settings.PacketRetryIntervalSeconds,
        Settings.PacketRetryBackoffMultiplier,
        Settings.MaxPacketRetryIntervalSeconds,
        Settings.SocketConnectionTimeoutSeconds,
        Settings.KeepAliveIntervalSeconds,
        Settings.MqttConnectionTimeoutSeconds,
        Settings.InitialRetryConnectionIntervalSeconds,
        Settings.MaxConnectionRetries,
        Settings.MaxPacketRetries,
        Settings.bShouldVerifyServerCertificate,
        Settings.SessionExpiryInterval,
        Settings.bAutoReconnectEnabled,
        Settings.AutoReconnectInitialDelayMs,
        Settings.AutoReconnectMaxDelayMs,
        Settings.AutoReconnectMultiplier,
        Settings.bStrictMode,
        Settings.bEnforceMaxPacketSize,
        Settings.MaxInboundPacketsPerTick,
        Settings.MaxPendingCommands,
        CallbackExec,
        nullptr
        );

    NativeClient = reactormq::mqtt::client::createClient(NativeSettings);

    NativeClient->onConnect().add(
        [this](bool bConnected)
        {
            const EReactorMQConnectReturnCode ReturnCode = bConnected
                ? EReactorMQConnectReturnCode::Accepted
                : EReactorMQConnectReturnCode::Cancelled;
            OnConnectDelegate.Broadcast(ReturnCode);
        });

    NativeClient->onDisconnect().add(
        [this](bool)
        {
            OnDisconnectDelegate.Broadcast();
        });

    NativeClient->onPublish().add(
        [this](bool bPublished)
        {
            OnPublishDelegate.Broadcast(bPublished);
        });

    NativeClient->onSubscribe().add(
        [this](const std::shared_ptr<std::vector<reactormq::mqtt::SubscribeResult>>& Results)
        {
            if (!Results || Results->empty())
            {
                return;
            }

            TArray<FReactorMQSubscribeResult> UEResults;
            UEResults.Reserve(Results->size());
            for (const auto& Result : *Results)
            {
                UEResults.Add(FReactorMQTypeMappings::FromNative(Result));
            }
            OnSubscribeDelegate.Broadcast(UEResults);
        });

    NativeClient->onUnsubscribe().add(
        [this](const std::shared_ptr<std::vector<reactormq::mqtt::UnsubscribeResult>>& Results)
        {
            if (!Results || Results->empty())
            {
                return;
            }

            TArray<FReactorMQUnsubscribeResult> UEResults;
            UEResults.Reserve(Results->size());
            for (const auto& Result : *Results)
            {
                UEResults.Add(FReactorMQTypeMappings::FromNative(Result));
            }
            OnUnsubscribeDelegate.Broadcast(UEResults);
        });

    NativeClient->onMessage().add(
        [this](const reactormq::mqtt::Message& Message)
        {
            FReactorMQMessage UEMessage = FReactorMQTypeMappings::FromNative(Message);
            OnMessageDelegate.Broadcast(UEMessage);
        });
}

FReactorMQClient::~FReactorMQClient()
{
    if (NativeClient && NativeClient->isConnected())
    {
        auto Future = NativeClient->disconnectAsync();
        Future.wait();
    }
}

TFuture<TReactorMQResult<void>> FReactorMQClient::ConnectAsync(bool bCleanSession)
{
    TPromise<TReactorMQResult<void>> Promise;
    TFuture<TReactorMQResult<void>> Future = Promise.GetFuture();

    auto NativeFuture = NativeClient->connectAsync(bCleanSession);

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            TReactorMQResult<void> UEResult(NativeResult.hasSucceeded());
            Promise.SetValue(MoveTemp(UEResult));
        });

    return Future;
}

TFuture<TReactorMQResult<void>> FReactorMQClient::DisconnectAsync()
{
    TPromise<TReactorMQResult<void>> Promise;
    TFuture<TReactorMQResult<void>> Future = Promise.GetFuture();

    auto NativeFuture = NativeClient->disconnectAsync();

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            TReactorMQResult<void> UEResult(NativeResult.hasSucceeded());
            Promise.SetValue(MoveTemp(UEResult));
        });

    return Future;
}

TFuture<TReactorMQResult<void>> FReactorMQClient::PublishAsync(
    const FString& Topic,
    const TArray<uint8>& Payload,
    EReactorMQQualityOfService QoS,
    bool bRetain)
{
    TPromise<TReactorMQResult<void>> Promise;
    TFuture<TReactorMQResult<void>> Future = Promise.GetFuture();

    auto NativeMessage = FReactorMQTypeMappings::ToNative(Topic, Payload, QoS, bRetain);
    auto NativeFuture = NativeClient->publishAsync(std::move(NativeMessage));

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            TReactorMQResult<void> UEResult(NativeResult.hasSucceeded());
            Promise.SetValue(MoveTemp(UEResult));
        });

    return Future;
}

TFuture<TReactorMQResult<TArray<FReactorMQSubscribeResult>>> FReactorMQClient::SubscribeAsync(
    const TArray<FReactorMQTopicFilter>& TopicFilters)
{
    TPromise<TReactorMQResult<TArray<FReactorMQSubscribeResult>>> Promise;
    TFuture<TReactorMQResult<TArray<FReactorMQSubscribeResult>>> Future = Promise.GetFuture();

    std::vector<reactormq::mqtt::TopicFilter> NativeFilters;
    NativeFilters.reserve(TopicFilters.Num());
    for (const auto& Filter : TopicFilters)
    {
        NativeFilters.push_back(FReactorMQTypeMappings::ToNative(Filter));
    }

    auto NativeFuture = NativeClient->subscribeAsync(std::move(NativeFilters));

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            if (NativeResult.hasSucceeded() && NativeResult.getResult())
            {
                TArray<FReactorMQSubscribeResult> UEResults;
                const auto& NativeResults = *NativeResult.getResult();
                UEResults.Reserve(NativeResults.size());
                for (const auto& Result : NativeResults)
                {
                    UEResults.Add(FReactorMQTypeMappings::FromNative(Result));
                }
                TReactorMQResult<TArray<FReactorMQSubscribeResult>> UEResult(true, MoveTemp(UEResults));
                Promise.SetValue(MoveTemp(UEResult));
            }
            else
            {
                TReactorMQResult<TArray<FReactorMQSubscribeResult>> UEResult(false);
                Promise.SetValue(MoveTemp(UEResult));
            }
        });

    return Future;
}

TFuture<TReactorMQResult<FReactorMQSubscribeResult>> FReactorMQClient::SubscribeAsync(const FReactorMQTopicFilter& TopicFilter)
{
    TPromise<TReactorMQResult<FReactorMQSubscribeResult>> Promise;
    TFuture<TReactorMQResult<FReactorMQSubscribeResult>> Future = Promise.GetFuture();

    auto NativeFilter = FReactorMQTypeMappings::ToNative(TopicFilter);
    auto NativeFuture = NativeClient->subscribeAsync(std::move(NativeFilter));

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            if (NativeResult.hasSucceeded() && NativeResult.getResult())
            {
                FReactorMQSubscribeResult UEResult = FReactorMQTypeMappings::FromNative(*NativeResult.getResult());
                TReactorMQResult<FReactorMQSubscribeResult> Result(true, MoveTemp(UEResult));
                Promise.SetValue(MoveTemp(Result));
            }
            else
            {
                TReactorMQResult<FReactorMQSubscribeResult> Result(false);
                Promise.SetValue(MoveTemp(Result));
            }
        });

    return Future;
}

TFuture<TReactorMQResult<TArray<FReactorMQUnsubscribeResult>>> FReactorMQClient::UnsubscribeAsync(const TArray<FString>& Topics)
{
    TPromise<TReactorMQResult<TArray<FReactorMQUnsubscribeResult>>> Promise;
    TFuture<TReactorMQResult<TArray<FReactorMQUnsubscribeResult>>> Future = Promise.GetFuture();

    std::vector<std::string> NativeTopics;
    NativeTopics.reserve(Topics.Num());
    for (const auto& Topic : Topics)
    {
        NativeTopics.push_back(TCHAR_TO_UTF8(*Topic));
    }

    auto NativeFuture = NativeClient->unsubscribeAsync(std::move(NativeTopics));

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            if (NativeResult.hasSucceeded() && NativeResult.getResult())
            {
                TArray<FReactorMQUnsubscribeResult> UEResults;
                const auto& NativeResults = *NativeResult.getResult();
                UEResults.Reserve(NativeResults.size());
                for (const auto& Result : NativeResults)
                {
                    UEResults.Add(FReactorMQTypeMappings::FromNative(Result));
                }
                TReactorMQResult<TArray<FReactorMQUnsubscribeResult>> UEResult(true, MoveTemp(UEResults));
                Promise.SetValue(MoveTemp(UEResult));
            }
            else
            {
                TReactorMQResult<TArray<FReactorMQUnsubscribeResult>> UEResult(false);
                Promise.SetValue(MoveTemp(UEResult));
            }
        });

    return Future;
}

TFuture<TReactorMQResult<FReactorMQUnsubscribeResult>> FReactorMQClient::UnsubscribeAsync(const FString& TopicFilter)
{
    TPromise<TReactorMQResult<FReactorMQUnsubscribeResult>> Promise;
    TFuture<TReactorMQResult<FReactorMQUnsubscribeResult>> Future = Promise.GetFuture();

    std::vector<std::string> NativeTopics;
    NativeTopics.emplace_back(TCHAR_TO_UTF8(*TopicFilter));

    auto NativeFuture = NativeClient->unsubscribeAsync(std::move(NativeTopics));

    AsyncTask(
        ENamedThreads::AnyBackgroundThreadNormalTask,
        [Promise = MoveTemp(Promise), NativeFuture = std::move(NativeFuture)]() mutable
        {
            auto NativeResult = NativeFuture.get();
            if (NativeResult.hasSucceeded() && NativeResult.getResult())
            {
                const auto& NativeResults = *NativeResult.getResult();
                if (!NativeResults.empty())
                {
                    FReactorMQUnsubscribeResult UEItem = FReactorMQTypeMappings::FromNative(NativeResults.front());
                    TReactorMQResult<FReactorMQUnsubscribeResult> UEResult(true, MoveTemp(UEItem));
                    Promise.SetValue(MoveTemp(UEResult));
                    return;
                }
            }

            TReactorMQResult<FReactorMQUnsubscribeResult> UEResult(false);
            Promise.SetValue(MoveTemp(UEResult));
        });

    return Future;
}

bool FReactorMQClient::IsConnected() const
{
    return NativeClient && NativeClient->isConnected();
}

void FReactorMQClient::CloseSocket(int32 Code, const FString& Reason)
{
    if (NativeClient)
    {
        NativeClient->closeSocket(Code, std::string(TCHAR_TO_UTF8(*Reason)));
    }
}

void FReactorMQClient::Tick() const
{
    if (NativeClient)
    {
        NativeClient->tick();
    }
}
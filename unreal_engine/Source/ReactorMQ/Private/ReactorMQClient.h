//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "IReactorMQClient.h"
#include "ReactorMQConnectionSettings.h"
#include <memory>

namespace reactormq::mqtt
{
    class IClient;
}

class FReactorMQClient : public IReactorMQClient
{
public:
    explicit FReactorMQClient(const FReactorMQConnectionSettings& InSettings);
    virtual ~FReactorMQClient() override;

    virtual TFuture<TReactorMQResult<void>> ConnectAsync(bool bCleanSession) override;
    virtual TFuture<TReactorMQResult<void>> DisconnectAsync() override;
    virtual TFuture<TReactorMQResult<void>> PublishAsync(
        const FString& Topic,
        const TArray<uint8>& Payload,
        EReactorMQQualityOfService QoS,
        bool bRetain) override;
    virtual TFuture<TReactorMQResult<TArray<FReactorMQSubscribeResult>>>
    SubscribeAsync(const TArray<FReactorMQTopicFilter>& TopicFilters) override;
    virtual TFuture<TReactorMQResult<FReactorMQSubscribeResult>> SubscribeAsync(const FReactorMQTopicFilter& TopicFilter) override;
    virtual TFuture<TReactorMQResult<TArray<FReactorMQUnsubscribeResult>>> UnsubscribeAsync(const TArray<FString>& Topics) override;
    virtual TFuture<TReactorMQResult<FReactorMQUnsubscribeResult>> UnsubscribeAsync(const FString& TopicFilter) override;

    virtual FOnReactorMQConnect& OnConnect() override
    {
        return OnConnectDelegate;
    }

    virtual FOnReactorMQDisconnect& OnDisconnect() override
    {
        return OnDisconnectDelegate;
    }

    virtual FOnReactorMQPublish& OnPublish() override
    {
        return OnPublishDelegate;
    }

    virtual FOnReactorMQSubscribe& OnSubscribe() override
    {
        return OnSubscribeDelegate;
    }

    virtual FOnReactorMQUnsubscribe& OnUnsubscribe() override
    {
        return OnUnsubscribeDelegate;
    }

    virtual FOnReactorMQMessage& OnMessage() override
    {
        return OnMessageDelegate;
    }

    virtual const FReactorMQConnectionSettings& GetConnectionSettings() const override
    {
        return Settings;
    }

    virtual bool IsConnected() const override;
    virtual void CloseSocket(int32 Code = 1000, const FString& Reason = TEXT("")) override;

    void Tick() const;

private:
    FReactorMQConnectionSettings Settings;
    std::shared_ptr<reactormq::mqtt::IClient> NativeClient;

    FOnReactorMQConnect OnConnectDelegate;
    FOnReactorMQDisconnect OnDisconnectDelegate;
    FOnReactorMQPublish OnPublishDelegate;
    FOnReactorMQSubscribe OnSubscribeDelegate;
    FOnReactorMQUnsubscribe OnUnsubscribeDelegate;
    FOnReactorMQMessage OnMessageDelegate;
};
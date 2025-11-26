//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "Async/Future.h"
#include "ReactorMQConnectionSettings.h"
#include "ReactorMQMessage.h"
#include "ReactorMQResult.h"
#include "ReactorMQSubscribeResult.h"
#include "ReactorMQTopicFilter.h"
#include "ReactorMQUnsubscribeResult.h"
#include "Delegates/OnConnect.h"
#include "Delegates/OnDisconnect.h"
#include "Delegates/OnMessage.h"
#include "Delegates/OnPublish.h"
#include "Delegates/OnSubscribe.h"
#include "Delegates/OnUnsubscribe.h"

class REACTORMQ_API IReactorMQClient
{
public:
    virtual ~IReactorMQClient() = default;

    virtual TFuture<TReactorMQResult<void>> ConnectAsync(bool bCleanSession) = 0;
    virtual TFuture<TReactorMQResult<void>> DisconnectAsync() = 0;
    virtual TFuture<TReactorMQResult<void>> PublishAsync(
        const FString& Topic, const TArray<uint8>& Payload, EReactorMQQualityOfService QoS, bool bRetain) = 0;
    virtual TFuture<TReactorMQResult<TArray<FReactorMQSubscribeResult>>> SubscribeAsync(const TArray<FReactorMQTopicFilter>& TopicFilters) =
    0;
    virtual TFuture<TReactorMQResult<FReactorMQSubscribeResult>> SubscribeAsync(const FReactorMQTopicFilter& TopicFilter) = 0;
    virtual TFuture<TReactorMQResult<TArray<FReactorMQUnsubscribeResult>>> UnsubscribeAsync(const TArray<FString>& Topics) = 0;
    virtual TFuture<TReactorMQResult<FReactorMQUnsubscribeResult>> UnsubscribeAsync(const FString& TopicFilter) = 0;

    virtual FOnReactorMQConnect& OnConnect() = 0;
    virtual FOnReactorMQDisconnect& OnDisconnect() = 0;
    virtual FOnReactorMQPublish& OnPublish() = 0;
    virtual FOnReactorMQSubscribe& OnSubscribe() = 0;
    virtual FOnReactorMQUnsubscribe& OnUnsubscribe() = 0;
    virtual FOnReactorMQMessage& OnMessage() = 0;

    virtual const FReactorMQConnectionSettings& GetConnectionSettings() const = 0;
    virtual bool IsConnected() const = 0;
    virtual void CloseSocket(int32 Code = 1000, const FString& Reason = TEXT("")) = 0;
};
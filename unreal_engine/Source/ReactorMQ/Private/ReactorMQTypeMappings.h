//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "ReactorMQConnectionSettings.h"
#include "ReactorMQMessage.h"
#include "ReactorMQSubscribeResult.h"
#include "ReactorMQTopicFilter.h"
#include "ReactorMQTypes.h"
#include "ReactorMQUnsubscribeResult.h"

#include <memory>

namespace reactormq::mqtt
{
    class ConnectionSettings;
    class ICredentialsProvider;
    enum class ConnectionProtocol : uint8_t;
    enum class QualityOfService : uint8_t;
    enum class ConnectReturnCode : uint8_t;
    enum class RetainHandlingOptions : uint8_t;
    struct Message;
    class TopicFilter;
    struct SubscribeResult;
    struct UnsubscribeResult;
}

class FReactorMQTypeMappings
{
public:
    static reactormq::mqtt::ConnectionProtocol ToNative(EReactorMQConnectionProtocol Protocol);
    static EReactorMQConnectionProtocol FromNative(reactormq::mqtt::ConnectionProtocol Protocol);

    static reactormq::mqtt::QualityOfService ToNative(EReactorMQQualityOfService QoS);
    static EReactorMQQualityOfService FromNative(reactormq::mqtt::QualityOfService QoS);

    static EReactorMQConnectReturnCode FromNative(reactormq::mqtt::ConnectReturnCode ReturnCode);

    static reactormq::mqtt::RetainHandlingOptions ToNative(EReactorMQRetainHandling RetainHandling);
    static EReactorMQRetainHandling FromNative(reactormq::mqtt::RetainHandlingOptions RetainHandling);

    static std::shared_ptr<reactormq::mqtt::ICredentialsProvider> ToNativeCredentialsProvider(
        const FReactorMQConnectionSettings& Settings);

    static std::shared_ptr<reactormq::mqtt::ConnectionSettings> ToNative(const FReactorMQConnectionSettings& Settings);

    static reactormq::mqtt::Message ToNative(
        const FString& Topic,
        const TArray<uint8>& Payload,
        EReactorMQQualityOfService QoS,
        bool bRetain);
    static FReactorMQMessage FromNative(const reactormq::mqtt::Message& Message);

    static reactormq::mqtt::TopicFilter ToNative(const FReactorMQTopicFilter& Filter);
    static FReactorMQTopicFilter FromNative(const reactormq::mqtt::TopicFilter& Filter);

    static FReactorMQSubscribeResult FromNative(const reactormq::mqtt::SubscribeResult& Result);
    static FReactorMQUnsubscribeResult FromNative(const reactormq::mqtt::UnsubscribeResult& Result);
};
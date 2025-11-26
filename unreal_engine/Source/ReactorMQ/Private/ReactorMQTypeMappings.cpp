//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "ReactorMQTypeMappings.h"
#include "reactormq/mqtt/connection_protocol.h"
#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/connect_return_code.h"
#include "reactormq/mqtt/credentials_provider.h"
#include "reactormq/mqtt/message.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/retain_handling_options.h"
#include "reactormq/mqtt/subscribe_result.h"
#include "reactormq/mqtt/topic_filter.h"
#include "reactormq/mqtt/unsubscribe_result.h"

reactormq::mqtt::ConnectionProtocol FReactorMQTypeMappings::ToNative(EReactorMQConnectionProtocol Protocol)
{
    switch (Protocol)
    {
    case EReactorMQConnectionProtocol::Tcp:
        return reactormq::mqtt::ConnectionProtocol::Tcp;
    case EReactorMQConnectionProtocol::Tls:
        return reactormq::mqtt::ConnectionProtocol::Tls;
    case EReactorMQConnectionProtocol::Ws:
        return reactormq::mqtt::ConnectionProtocol::Ws;
    case EReactorMQConnectionProtocol::Wss:
        return reactormq::mqtt::ConnectionProtocol::Wss;
    default:
        return reactormq::mqtt::ConnectionProtocol::Tcp;
    }
}

EReactorMQConnectionProtocol FReactorMQTypeMappings::FromNative(reactormq::mqtt::ConnectionProtocol Protocol)
{
    switch (Protocol)
    {
    case reactormq::mqtt::ConnectionProtocol::Tcp:
        return EReactorMQConnectionProtocol::Tcp;
    case reactormq::mqtt::ConnectionProtocol::Tls:
        return EReactorMQConnectionProtocol::Tls;
    case reactormq::mqtt::ConnectionProtocol::Ws:
        return EReactorMQConnectionProtocol::Ws;
    case reactormq::mqtt::ConnectionProtocol::Wss:
        return EReactorMQConnectionProtocol::Wss;
    default:
        return EReactorMQConnectionProtocol::Unknown;
    }
}

reactormq::mqtt::QualityOfService FReactorMQTypeMappings::ToNative(EReactorMQQualityOfService QoS)
{
    switch (QoS)
    {
    case EReactorMQQualityOfService::AtMostOnce:
        return reactormq::mqtt::QualityOfService::AtMostOnce;
    case EReactorMQQualityOfService::AtLeastOnce:
        return reactormq::mqtt::QualityOfService::AtLeastOnce;
    case EReactorMQQualityOfService::ExactlyOnce:
        return reactormq::mqtt::QualityOfService::ExactlyOnce;
    default:
        return reactormq::mqtt::QualityOfService::AtMostOnce;
    }
}

EReactorMQQualityOfService FReactorMQTypeMappings::FromNative(reactormq::mqtt::QualityOfService QoS)
{
    switch (QoS)
    {
    case reactormq::mqtt::QualityOfService::AtMostOnce:
        return EReactorMQQualityOfService::AtMostOnce;
    case reactormq::mqtt::QualityOfService::AtLeastOnce:
        return EReactorMQQualityOfService::AtLeastOnce;
    case reactormq::mqtt::QualityOfService::ExactlyOnce:
        return EReactorMQQualityOfService::ExactlyOnce;
    default:
        return EReactorMQQualityOfService::AtMostOnce;
    }
}

EReactorMQConnectReturnCode FReactorMQTypeMappings::FromNative(reactormq::mqtt::ConnectReturnCode ReturnCode)
{
    switch (ReturnCode)
    {
    case reactormq::mqtt::ConnectReturnCode::Accepted:
        return EReactorMQConnectReturnCode::Accepted;
    case reactormq::mqtt::ConnectReturnCode::RefusedProtocolVersion:
        return EReactorMQConnectReturnCode::RefusedProtocolVersion;
    case reactormq::mqtt::ConnectReturnCode::RefusedIdentifierRejected:
        return EReactorMQConnectReturnCode::RefusedIdentifierRejected;
    case reactormq::mqtt::ConnectReturnCode::RefusedServerUnavailable:
        return EReactorMQConnectReturnCode::RefusedServerUnavailable;
    case reactormq::mqtt::ConnectReturnCode::RefusedBadUserNameOrPassword:
        return EReactorMQConnectReturnCode::RefusedBadUserNameOrPassword;
    case reactormq::mqtt::ConnectReturnCode::RefusedNotAuthorized:
        return EReactorMQConnectReturnCode::RefusedNotAuthorized;
    case reactormq::mqtt::ConnectReturnCode::Cancelled:
        return EReactorMQConnectReturnCode::Cancelled;
    default:
        return EReactorMQConnectReturnCode::Cancelled;
    }
}

reactormq::mqtt::RetainHandlingOptions FReactorMQTypeMappings::ToNative(EReactorMQRetainHandling RetainHandling)
{
    switch (RetainHandling)
    {
    case EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTime:
        return reactormq::mqtt::RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime;
    case EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist:
        return reactormq::mqtt::RetainHandlingOptions::SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist;
    case EReactorMQRetainHandling::DoNotSendRetainedMessages:
        return reactormq::mqtt::RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime;
    default:
        return reactormq::mqtt::RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime;
    }
}

EReactorMQRetainHandling FReactorMQTypeMappings::FromNative(reactormq::mqtt::RetainHandlingOptions RetainHandling)
{
    switch (RetainHandling)
    {
    case reactormq::mqtt::RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime:
        return EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTime;
    case reactormq::mqtt::RetainHandlingOptions::SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist:
        return EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist;
    case reactormq::mqtt::RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime:
        return EReactorMQRetainHandling::DoNotSendRetainedMessages;
    default:
        return EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTime;
    }
}

std::shared_ptr<reactormq::mqtt::ICredentialsProvider> FReactorMQTypeMappings::ToNativeCredentialsProvider(
    const FReactorMQConnectionSettings& Settings)
{
    if (!Settings.CredentialsProvider.IsValid())
    {
        return nullptr;
    }

    class FCredentialsProviderWrapper : public reactormq::mqtt::ICredentialsProvider
    {
    public:
        explicit FCredentialsProviderWrapper(FReactorMQCredentialsProviderPtr InProvider)
            : Provider(MoveTemp(InProvider))
        {
        }

        virtual reactormq::mqtt::Credentials getCredentials() override
        {
            if (!Provider.IsValid())
            {
                return reactormq::mqtt::Credentials{};
            }

            const FReactorMQCredentials Creds = Provider->GetCredentials();
            return reactormq::mqtt::Credentials(
                std::string(TCHAR_TO_UTF8(*Creds.Username)),
                std::string(TCHAR_TO_UTF8(*Creds.Password)));
        }

        virtual std::string getAuthMethod() override
        {
            if (!Provider.IsValid())
            {
                return {};
            }
            const FString Method = Provider->GetAuthMethod();
            return std::string(TCHAR_TO_UTF8(*Method));
        }

        virtual std::vector<std::uint8_t> getInitialAuthData() override
        {
            if (!Provider.IsValid())
            {
                return {};
            }
            TArray<uint8> Data = Provider->GetInitialAuthData();
            std::vector<std::uint8_t> Out;
            Out.reserve(Data.Num());
            Out.insert(Out.end(), Data.GetData(), Data.GetData() + Data.Num());
            return Out;
        }

        virtual std::vector<std::uint8_t> onAuthChallenge(const std::vector<std::uint8_t>& ServerData) override
        {
            if (!Provider.IsValid())
            {
                return {};
            }

            TArray<uint8> In;
            In.SetNumUninitialized(ServerData.size());
            if (!ServerData.empty())
            {
                FMemory::Memcpy(In.GetData(), ServerData.data(), ServerData.size());
            }

            TArray<uint8> Response = Provider->OnAuthChallenge(In);
            std::vector<std::uint8_t> Out;
            Out.reserve(Response.Num());
            Out.insert(Out.end(), Response.GetData(), Response.GetData() + Response.Num());
            return Out;
        }

    private:
        FReactorMQCredentialsProviderPtr Provider;
    };

    return std::make_shared<FCredentialsProviderWrapper>(Settings.CredentialsProvider);
}

std::shared_ptr<reactormq::mqtt::ConnectionSettings> FReactorMQTypeMappings::ToNative(const FReactorMQConnectionSettings& Settings)
{
    auto CredentialsProvider = ToNativeCredentialsProvider(Settings);

    return std::make_shared<reactormq::mqtt::ConnectionSettings>(
        TCHAR_TO_UTF8(*Settings.Host),
        Settings.Port,
        ToNative(Settings.Protocol),
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
        nullptr,
        nullptr
        );
}

reactormq::mqtt::Message FReactorMQTypeMappings::ToNative(
    const FString& Topic,
    const TArray<uint8>& Payload,
    EReactorMQQualityOfService QoS,
    bool bRetain)
{
    std::vector<uint8_t> NativePayload(Payload.GetData(), Payload.GetData() + Payload.Num());
    return reactormq::mqtt::Message(
        TCHAR_TO_UTF8(*Topic),
        std::move(NativePayload),
        bRetain,
        ToNative(QoS)
        );
}

FReactorMQMessage FReactorMQTypeMappings::FromNative(const reactormq::mqtt::Message& Message)
{
    FReactorMQMessage Result;
    Result.Topic = UTF8_TO_TCHAR(Message.getTopic().c_str());
    const auto& NativePayload = Message.getPayload();
    Result.Payload.SetNum(NativePayload.size());
    FMemory::Memcpy(Result.Payload.GetData(), NativePayload.data(), NativePayload.size());
    Result.QualityOfService = FromNative(Message.getQualityOfService());
    Result.bShouldRetain = Message.shouldRetain();

    const auto TimePoint = Message.getTimestampUtc();
    const auto Duration = TimePoint.time_since_epoch();
    const auto Microseconds = std::chrono::duration_cast<std::chrono::microseconds>(Duration).count();
    Result.TimestampUtc = FDateTime(1970, 1, 1) + FTimespan::FromMicroseconds(Microseconds);

    return Result;
}

reactormq::mqtt::TopicFilter FReactorMQTypeMappings::ToNative(const FReactorMQTopicFilter& Filter)
{
    return reactormq::mqtt::TopicFilter(
        TCHAR_TO_UTF8(*Filter.Filter),
        ToNative(Filter.QualityOfService),
        Filter.bNoLocal,
        Filter.bRetainAsPublished,
        ToNative(Filter.RetainHandling)
        );
}

FReactorMQTopicFilter FReactorMQTypeMappings::FromNative(const reactormq::mqtt::TopicFilter& Filter)
{
    FReactorMQTopicFilter Result;
    Result.Filter = UTF8_TO_TCHAR(Filter.getFilter().c_str());
    Result.QualityOfService = FromNative(Filter.getQualityOfService());
    Result.bNoLocal = Filter.getIsNoLocal();
    Result.bRetainAsPublished = Filter.getIsRetainAsPublished();
    Result.RetainHandling = FromNative(Filter.getRetainHandlingOptions());
    return Result;
}

FReactorMQSubscribeResult FReactorMQTypeMappings::FromNative(const reactormq::mqtt::SubscribeResult& Result)
{
    return FReactorMQSubscribeResult(
        FromNative(Result.getFilter()),
        Result.wasSuccessful()
        );
}

FReactorMQUnsubscribeResult FReactorMQTypeMappings::FromNative(const reactormq::mqtt::UnsubscribeResult& Result)
{
    return FReactorMQUnsubscribeResult(
        FromNative(Result.getFilter()),
        Result.wasSuccessful()
        );
}
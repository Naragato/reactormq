#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /// @brief MQTT v5 Reason Codes (subset and common aliases)
    enum class ReasonCode : uint8_t
    {
        Success = 0x00,
        NormalDisconnection = 0x00,         // DISCONNECT
        GrantedQualityOfService0 = 0x00,    // SUBACK

        GrantedQualityOfService1 = 0x01,
        GrantedQualityOfService2 = 0x02,

        DisconnectWithWillMessage = 0x04,

        NoMatchingSubscribers = 0x10,       // PUBACK, PUBREC
        NoSubscriptionExisted = 0x11,       // UNSUBACK

        ContinueAuthentication = 0x18,      // AUTH
        ReAuthenticate = 0x19,              // AUTH

        UnspecifiedError = 0x80,
        MalformedPacket = 0x81,
        ProtocolError = 0x82,
        ImplementationSpecificError = 0x83,
        UnsupportedProtocolVersion = 0x84,  // CONNACK
        ClientIdentifierNotValid = 0x85,    // CONNACK
        BadUsernameOrPassword = 0x86,       // CONNACK
        NotAuthorized = 0x87,
        ServerUnavailable = 0x88,           // CONNACK
        ServerBusy = 0x89,                  // CONNACK, DISCONNECT
        Banned = 0x8A,                      // CONNACK
        ServerShuttingDown = 0x8B,          // DISCONNECT
        BadAuthenticationMethod = 0x8C,     // CONNACK, DISCONNECT
        KeepAliveTimeout = 0x8D,            // DISCONNECT
        SessionTakenOver = 0x8E,            // DISCONNECT
        TopicFilterInvalid = 0x8F,          // SUBACK, UNSUBACK, DISCONNECT
        TopicNameInvalid = 0x90,            // CONNACK, PUBACK/REC, DISCONNECT
        PacketIdentifierInUse = 0x91,
        PacketIdentifierNotFound = 0x92,
        ReceiveMaximumExceeded = 0x93,
        TopicAliasInvalid = 0x94,
        PacketTooLarge = 0x95,
        MessageRateTooHigh = 0x96,
        QuotaExceeded = 0x97,
        AdministrativeAction = 0x98,
        PayloadFormatInvalid = 0x99,
        RetainNotSupported = 0x9A,
        QoSNotSupported = 0x9B,
        UseAnotherServer = 0x9C,
        ServerMoved = 0x9D,
        SharedSubscriptionsNotSupported = 0x9E,
        ConnectionRateExceeded = 0x9F,
        MaximumConnectTime = 0xA0,
        SubscriptionIdentifiersNotSupported = 0xA1,
        WildcardSubscriptionsNotSupported = 0xA2,
    };
} // namespace reactormq::mqtt

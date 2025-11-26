//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 5 reason codes used across ACK/AUTH/DISCONNECT flows.
     *
     * This is a commonly used subset with a few aliases where the same value
     * appears in multiple contexts (e.g., Success for SUBACK and DISCONNECT).
     */
    enum class ReasonCode : uint8_t
    {
        Success = 0x00, ///< Generic success (context-dependent).
        NormalDisconnection = 0x00, ///< DISCONNECT: normal closure.
        GrantedQualityOfService0 = 0x00, ///< SUBACK: granted with QoS 0.

        GrantedQualityOfService1 = 0x01, ///< SUBACK: granted with QoS 1.
        GrantedQualityOfService2 = 0x02, ///< SUBACK: granted with QoS 2.

        DisconnectWithWillMessage = 0x04, ///< DISCONNECT: send Will message.

        NoMatchingSubscribers = 0x10, ///< PUBACK/PUBREC: no subscribers matched.
        NoSubscriptionExisted = 0x11, ///< UNSUBACK: topic filter not found.

        ContinueAuthentication = 0x18, ///< AUTH: continue step-up auth.
        ReAuthenticate = 0x19, ///< AUTH: request re-authentication.

        UnspecifiedError = 0x80, ///< Generic error without a specific reason.
        MalformedPacket = 0x81, ///< Packet failed basic format checks.
        ProtocolError = 0x82, ///< Protocol rule violated.
        ImplementationSpecificError = 0x83, ///< Vendor-specific failure.
        UnsupportedProtocolVersion = 0x84, ///< CONNACK: version not supported.
        ClientIdentifierNotValid = 0x85, ///< CONNACK: client id rejected.
        BadUsernameOrPassword = 0x86, ///< CONNACK: credentials invalid.
        NotAuthorized = 0x87, ///< Operation not authorized.
        ServerUnavailable = 0x88, ///< CONNACK: server cannot accept connections.
        ServerBusy = 0x89, ///< Server busy; try later.
        Banned = 0x8A, ///< CONNACK: client banned.
        ServerShuttingDown = 0x8B, ///< Server is shutting down.
        BadAuthenticationMethod = 0x8C, ///< Bad or unsupported auth method.
        KeepAliveTimeout = 0x8D, ///< No PINGRESP in time.
        SessionTakenOver = 0x8E, ///< Session used by another client.
        TopicFilterInvalid = 0x8F, ///< SUBACK/UNSUBACK/DISCONNECT: invalid filter.
        TopicNameInvalid = 0x90, ///< Topic name invalid for the operation.
        PacketIdentifierInUse = 0x91, ///< Packet id already in use.
        PacketIdentifierNotFound = 0x92, ///< Packet id not found.
        ReceiveMaximumExceeded = 0x93, ///< Exceeded Receive Maximum.
        TopicAliasInvalid = 0x94, ///< Topic alias invalid.
        PacketTooLarge = 0x95, ///< Packet exceeds allowed size.
        MessageRateTooHigh = 0x96, ///< Throttled due to rate limit.
        QuotaExceeded = 0x97, ///< Exceeded server quota.
        AdministrativeAction = 0x98, ///< Administrative disconnect or block.
        PayloadFormatInvalid = 0x99, ///< Payload format is invalid.
        RetainNotSupported = 0x9A, ///< Retained messages not supported.
        QoSNotSupported = 0x9B, ///< Requested QoS not supported.
        UseAnotherServer = 0x9C, ///< Try a different server.
        ServerMoved = 0x9D, ///< Server moved; use new address.
        SharedSubscriptionsNotSupported = 0x9E, ///< Shared subs not supported.
        ConnectionRateExceeded = 0x9F, ///< Connection attempts too frequent.
        MaximumConnectTime = 0xA0, ///< Connection exceeded allowed duration.
        SubscriptionIdentifiersNotSupported = 0xA1, ///< Subscription IDs not supported.
        WildcardSubscriptionsNotSupported = 0xA2, ///< Wildcards not supported.
    };
} // namespace reactormq::mqtt
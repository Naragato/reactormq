//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt::packets::properties
{
    /**
     * @brief Enum representing MQTT property identifiers.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901027
     */
    enum class PropertyIdentifier : uint8_t
    {
        /// @brief Represents an unknown or unspecified MQTT property.
        Unknown = 0,
        /// @brief Indicates the format of the MQTT payload.
        PayloadFormatIndicator = 1,
        /// @brief Specifies the time after which the message should expire.
        MessageExpiryInterval = 2,
        /// @brief Describes the type of content in the payload.
        ContentType = 3,
        /// @brief Topic where response messages should be sent.
        ResponseTopic = 8,
        /// @brief Data used to correlate response messages with request messages.
        CorrelationData = 9,
        /// @brief Identifier that specifies the subscription.
        SubscriptionIdentifier = 11,
        /// @brief Time after which the MQTT session should expire.
        SessionExpiryInterval = 17,
        /// @brief Client identifier assigned by the server.
        AssignedClientIdentifier = 18,
        /// @brief Keep-alive time specified by the server.
        ServerKeepAlive = 19,
        /// @brief Method used for authentication.
        AuthenticationMethod = 21,
        /// @brief Data used for authentication.
        AuthenticationData = 22,
        /// @brief Flag to request problem information from the server.
        RequestProblemInformation = 23,
        /// @brief Interval before the Will message is sent.
        WillDelayInterval = 24,
        /// @brief Flag to request response information from the server.
        RequestResponseInformation = 25,
        /// @brief Response information from the server.
        ResponseInformation = 26,
        /// @brief Reference to another server where the client can connect.
        ServerReference = 28,
        /// @brief Reason string to provide more information about the operation.
        ReasonString = 31,
        /// @brief Specifies the maximum number of messages that can be received.
        ReceiveMaximum = 33,
        /// @brief Maximum number of topic aliases that can be used.
        TopicAliasMaximum = 34,
        /// @brief Topic alias to be used instead of the topic name.
        TopicAlias = 35,
        /// @brief The maximum QoS level the server supports.
        MaximumQoS = 36,
        /// @brief Indicates if the server supports retained messages.
        RetainAvailable = 37,
        /// @brief General-purpose user property for the message.
        UserProperty = 38,
        /// @brief Specifies the maximum packet size.
        MaximumPacketSize = 39,
        /// @brief Indicates if wildcard subscriptions are available.
        WildcardSubscriptionAvailable = 40,
        /// @brief Indicates if the subscription identifier is available.
        SubscriptionIdentifierAvailable = 41,
        /// @brief Indicates if shared subscriptions are available.
        SharedSubscriptionAvailable = 42,
        /// @brief Max enum size, used for validation.
        Max = 43
    };

    /**
     * @brief Check if a property identifier value is valid.
     * @param value Property identifier value to check.
     * @return true if the value is a valid property identifier, false otherwise.
     */
    inline bool isValidPropertyIdentifier(const uint8_t value)
    {
        switch (value)
        {
            using enum PropertyIdentifier;
        case static_cast<uint8_t>(PayloadFormatIndicator):
        case static_cast<uint8_t>(MessageExpiryInterval):
        case static_cast<uint8_t>(ContentType):
        case static_cast<uint8_t>(ResponseTopic):
        case static_cast<uint8_t>(CorrelationData):
        case static_cast<uint8_t>(SubscriptionIdentifier):
        case static_cast<uint8_t>(SessionExpiryInterval):
        case static_cast<uint8_t>(AssignedClientIdentifier):
        case static_cast<uint8_t>(ServerKeepAlive):
        case static_cast<uint8_t>(AuthenticationMethod):
        case static_cast<uint8_t>(AuthenticationData):
        case static_cast<uint8_t>(RequestProblemInformation):
        case static_cast<uint8_t>(WillDelayInterval):
        case static_cast<uint8_t>(RequestResponseInformation):
        case static_cast<uint8_t>(ResponseInformation):
        case static_cast<uint8_t>(ServerReference):
        case static_cast<uint8_t>(ReasonString):
        case static_cast<uint8_t>(ReceiveMaximum):
        case static_cast<uint8_t>(TopicAliasMaximum):
        case static_cast<uint8_t>(TopicAlias):
        case static_cast<uint8_t>(MaximumQoS):
        case static_cast<uint8_t>(RetainAvailable):
        case static_cast<uint8_t>(UserProperty):
        case static_cast<uint8_t>(MaximumPacketSize):
        case static_cast<uint8_t>(WildcardSubscriptionAvailable):
        case static_cast<uint8_t>(SubscriptionIdentifierAvailable):
        case static_cast<uint8_t>(SharedSubscriptionAvailable):
            return true;
        case static_cast<uint8_t>(Unknown):
        case static_cast<uint8_t>(Max):
        default:
            return false;
        }
    }

    /**
     * @brief Convert PropertyIdentifier enum to string.
     * @param id Property identifier to convert.
     * @return String representation of the property identifier.
     */
    inline const char* propertyIdentifierToString(const PropertyIdentifier id)
    {
        switch (id)
        {
            using enum PropertyIdentifier;
        case Unknown:
            return "Unknown";
        case PayloadFormatIndicator:
            return "PayloadFormatIndicator";
        case MessageExpiryInterval:
            return "MessageExpiryInterval";
        case ContentType:
            return "ContentType";
        case ResponseTopic:
            return "ResponseTopic";
        case CorrelationData:
            return "CorrelationData";
        case SubscriptionIdentifier:
            return "SubscriptionIdentifier";
        case SessionExpiryInterval:
            return "SessionExpiryInterval";
        case AssignedClientIdentifier:
            return "AssignedClientIdentifier";
        case ServerKeepAlive:
            return "ServerKeepAlive";
        case AuthenticationMethod:
            return "AuthenticationMethod";
        case AuthenticationData:
            return "AuthenticationData";
        case RequestProblemInformation:
            return "RequestProblemInformation";
        case WillDelayInterval:
            return "WillDelayInterval";
        case RequestResponseInformation:
            return "RequestResponseInformation";
        case ResponseInformation:
            return "ResponseInformation";
        case ServerReference:
            return "ServerReference";
        case ReasonString:
            return "ReasonString";
        case ReceiveMaximum:
            return "ReceiveMaximum";
        case TopicAliasMaximum:
            return "TopicAliasMaximum";
        case TopicAlias:
            return "TopicAlias";
        case MaximumQoS:
            return "MaximumQoS";
        case RetainAvailable:
            return "RetainAvailable";
        case UserProperty:
            return "UserProperty";
        case MaximumPacketSize:
            return "MaximumPacketSize";
        case WildcardSubscriptionAvailable:
            return "WildcardSubscriptionAvailable";
        case SubscriptionIdentifierAvailable:
            return "SubscriptionIdAvailable";
        case SharedSubscriptionAvailable:
            return "SharedSubscriptionAvailable";
        case Max:
        default:
            return "Invalid";
        }
    }
} // namespace reactormq::mqtt::packets::properties
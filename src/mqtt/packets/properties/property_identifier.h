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
            case static_cast<uint8_t>(PropertyIdentifier::PayloadFormatIndicator):
            case static_cast<uint8_t>(PropertyIdentifier::MessageExpiryInterval):
            case static_cast<uint8_t>(PropertyIdentifier::ContentType):
            case static_cast<uint8_t>(PropertyIdentifier::ResponseTopic):
            case static_cast<uint8_t>(PropertyIdentifier::CorrelationData):
            case static_cast<uint8_t>(PropertyIdentifier::SubscriptionIdentifier):
            case static_cast<uint8_t>(PropertyIdentifier::SessionExpiryInterval):
            case static_cast<uint8_t>(PropertyIdentifier::AssignedClientIdentifier):
            case static_cast<uint8_t>(PropertyIdentifier::ServerKeepAlive):
            case static_cast<uint8_t>(PropertyIdentifier::AuthenticationMethod):
            case static_cast<uint8_t>(PropertyIdentifier::AuthenticationData):
            case static_cast<uint8_t>(PropertyIdentifier::RequestProblemInformation):
            case static_cast<uint8_t>(PropertyIdentifier::WillDelayInterval):
            case static_cast<uint8_t>(PropertyIdentifier::RequestResponseInformation):
            case static_cast<uint8_t>(PropertyIdentifier::ResponseInformation):
            case static_cast<uint8_t>(PropertyIdentifier::ServerReference):
            case static_cast<uint8_t>(PropertyIdentifier::ReasonString):
            case static_cast<uint8_t>(PropertyIdentifier::ReceiveMaximum):
            case static_cast<uint8_t>(PropertyIdentifier::TopicAliasMaximum):
            case static_cast<uint8_t>(PropertyIdentifier::TopicAlias):
            case static_cast<uint8_t>(PropertyIdentifier::MaximumQoS):
            case static_cast<uint8_t>(PropertyIdentifier::RetainAvailable):
            case static_cast<uint8_t>(PropertyIdentifier::UserProperty):
            case static_cast<uint8_t>(PropertyIdentifier::MaximumPacketSize):
            case static_cast<uint8_t>(PropertyIdentifier::WildcardSubscriptionAvailable):
            case static_cast<uint8_t>(PropertyIdentifier::SubscriptionIdentifierAvailable):
            case static_cast<uint8_t>(PropertyIdentifier::SharedSubscriptionAvailable):
                return true;
            case static_cast<uint8_t>(PropertyIdentifier::Unknown):
            case static_cast<uint8_t>(PropertyIdentifier::Max):
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
            case PropertyIdentifier::Unknown:
                return "Unknown";
            case PropertyIdentifier::PayloadFormatIndicator:
                return "PayloadFormatIndicator";
            case PropertyIdentifier::MessageExpiryInterval:
                return "MessageExpiryInterval";
            case PropertyIdentifier::ContentType:
                return "ContentType";
            case PropertyIdentifier::ResponseTopic:
                return "ResponseTopic";
            case PropertyIdentifier::CorrelationData:
                return "CorrelationData";
            case PropertyIdentifier::SubscriptionIdentifier:
                return "SubscriptionIdentifier";
            case PropertyIdentifier::SessionExpiryInterval:
                return "SessionExpiryInterval";
            case PropertyIdentifier::AssignedClientIdentifier:
                return "AssignedClientIdentifier";
            case PropertyIdentifier::ServerKeepAlive:
                return "ServerKeepAlive";
            case PropertyIdentifier::AuthenticationMethod:
                return "AuthenticationMethod";
            case PropertyIdentifier::AuthenticationData:
                return "AuthenticationData";
            case PropertyIdentifier::RequestProblemInformation:
                return "RequestProblemInformation";
            case PropertyIdentifier::WillDelayInterval:
                return "WillDelayInterval";
            case PropertyIdentifier::RequestResponseInformation:
                return "RequestResponseInformation";
            case PropertyIdentifier::ResponseInformation:
                return "ResponseInformation";
            case PropertyIdentifier::ServerReference:
                return "ServerReference";
            case PropertyIdentifier::ReasonString:
                return "ReasonString";
            case PropertyIdentifier::ReceiveMaximum:
                return "ReceiveMaximum";
            case PropertyIdentifier::TopicAliasMaximum:
                return "TopicAliasMaximum";
            case PropertyIdentifier::TopicAlias:
                return "TopicAlias";
            case PropertyIdentifier::MaximumQoS:
                return "MaximumQoS";
            case PropertyIdentifier::RetainAvailable:
                return "RetainAvailable";
            case PropertyIdentifier::UserProperty:
                return "UserProperty";
            case PropertyIdentifier::MaximumPacketSize:
                return "MaximumPacketSize";
            case PropertyIdentifier::WildcardSubscriptionAvailable:
                return "WildcardSubscriptionAvailable";
            case PropertyIdentifier::SubscriptionIdentifierAvailable:
                return "SubscriptionIdAvailable";
            case PropertyIdentifier::SharedSubscriptionAvailable:
                return "SharedSubscriptionAvailable";
            case PropertyIdentifier::Max:
            default:
                return "Invalid";
        }
    }
} // namespace reactormq::mqtt::packets::properties
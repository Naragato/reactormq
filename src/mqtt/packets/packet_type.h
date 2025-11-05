#pragma once

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Enum representing MQTT Control Packet types.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
     */
    enum class PacketType : uint8_t
    {
        /// @brief Non-spec entry.
        None = 0,

        /// @brief CLIENT requests a connection to a Server.
        Connect = 1,

        /// @brief CONNACK - Acknowledge connection request.
        ConnAck = 2,

        /// @brief PUBLISH - Publish message.
        Publish = 3,

        /// @brief PUBACK - Publish acknowledgement for QoS 1.
        PubAck = 4,

        /// @brief PUBREC - Publish received for QoS 2.
        PubRec = 5,

        /// @brief PUBREL - Publish release for QoS 2.
        PubRel = 6,

        /// @brief PUBCOMP - Publish complete for QoS 2.
        PubComp = 7,

        /// @brief SUBSCRIBE - Subscribe request.
        Subscribe = 8,

        /// @brief SUBACK - Subscribe acknowledgement.
        SubAck = 9,

        /// @brief UNSUBSCRIBE - Unsubscribe request.
        Unsubscribe = 10,

        /// @brief UNSUBACK - Unsubscribe acknowledgement.
        UnsubAck = 11,

        /// @brief PINGREQ - Ping request.
        PingReq = 12,

        /// @brief PINGRESP - Ping response.
        PingResp = 13,

        /// @brief DISCONNECT - Disconnect notification.
        Disconnect = 14,

        /// @brief AUTH - Authentication for MQTT 5.0.
        Auth = 15,

        /// @brief Max enum size, used for validation.
        Max
    };

    /**
     * @brief Convert PacketType enum to string.
     * @param type Packet type to convert.
     * @return String representation of the packet type.
     */
    inline const char* packetTypeToString(PacketType type)
    {
        switch (type)
        {
            case PacketType::Connect:
                return "Connect";
            case PacketType::ConnAck:
                return "ConnAck";
            case PacketType::Publish:
                return "Publish";
            case PacketType::PubAck:
                return "PubAck";
            case PacketType::PubRec:
                return "PubRec";
            case PacketType::PubRel:
                return "PubRel";
            case PacketType::PubComp:
                return "PubComp";
            case PacketType::Subscribe:
                return "Subscribe";
            case PacketType::SubAck:
                return "SubAck";
            case PacketType::Unsubscribe:
                return "Unsubscribe";
            case PacketType::UnsubAck:
                return "UnsubAck";
            case PacketType::PingReq:
                return "PingReq";
            case PacketType::PingResp:
                return "PingResp";
            case PacketType::Disconnect:
                return "Disconnect";
            case PacketType::Auth:
                return "Auth";
            case PacketType::None:
            case PacketType::Max:
            default:
                return "Unknown";
        }
    }
} // namespace reactormq::mqtt::packets

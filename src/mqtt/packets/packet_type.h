//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief MQTT Control Packet types.
     * Maps the high nibble of the fixed header to a readable enum.
     */
    enum class PacketType : uint8_t
    {
        None = 0, ///< Spec placeholder.
        Connect = 1, ///< CLIENT requests a connection to a Server.
        ConnAck = 2, ///< CONNACK - Acknowledge connection request.
        Publish = 3, ///< PUBLISH - Publish message.
        PubAck = 4, ///< PUBACK - Publish acknowledgement for QoS 1.
        PubRec = 5, ///< PUBREC - Publish received for QoS 2.
        PubRel = 6, ///< PUBREL - Publish release for QoS 2.
        PubComp = 7, ///< PUBCOMP - Publish complete for QoS 2.
        Subscribe = 8, ///< SUBSCRIBE - Subscribe request.
        SubAck = 9, ///< SUBACK - Subscribe acknowledgement.
        Unsubscribe = 10, ///< UNSUBSCRIBE - Unsubscribe request.
        UnsubAck = 11, ///< UNSUBACK - Unsubscribe acknowledgement.
        PingReq = 12, ///< PINGREQ - Ping request.
        PingResp = 13, ///< PINGRESP - Ping response.
        Disconnect = 14, ///< DISCONNECT - Disconnect notification.
        Auth = 15, ///< AUTH - Authentication for MQTT 5.0.
        Max ///< Sentinel value used for validation.
    };

    /**
     * @brief Convert a PacketType to a readable string.
     * @return Constant C-string.
     */
    inline const char* packetTypeToString(const PacketType type)
    {
        switch (type)
        {
            using enum PacketType;
        case Connect:
            return "Connect";
        case ConnAck:
            return "ConnAck";
        case Publish:
            return "Publish";
        case PubAck:
            return "PubAck";
        case PubRec:
            return "PubRec";
        case PubRel:
            return "PubRel";
        case PubComp:
            return "PubComp";
        case Subscribe:
            return "Subscribe";
        case SubAck:
            return "SubAck";
        case Unsubscribe:
            return "Unsubscribe";
        case UnsubAck:
            return "UnsubAck";
        case PingReq:
            return "PingReq";
        case PingResp:
            return "PingResp";
        case Disconnect:
            return "Disconnect";
        case Auth:
            return "Auth";
        case None:
        case Max:
        default:
            return "Unknown";
        }
    }
} // namespace reactormq::mqtt::packets
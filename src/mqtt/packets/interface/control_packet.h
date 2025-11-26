//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/packet_type.h"
#include "serialize/serializable.h"

namespace reactormq::mqtt::packets
{
    /**
     * @brief Common interface for all MQTT control packets.
     * Defines the packet type, length, optional packet id, and validity check.
     */
    class IControlPacket : public serialize::ISerializable
    {
    public:
        IControlPacket() = default;

        ~IControlPacket() override = default;

        /**
         * @brief Size of the serialized packet body (excludes fixed header).
         * @return Payload length in bytes.
         */
        [[nodiscard]] virtual uint32_t getLength() const = 0;

        /**
         * @brief MQTT control packet type.
         * @return Packet type value.
         */
        [[nodiscard]] virtual PacketType getPacketType() const = 0;

        /**
         * @brief Packet identifier when applicable.
         * @return Packet id, or 0 when the packet does not carry one.
         */
        [[nodiscard]] virtual uint16_t getPacketId() const
        {
            return 0;
        }

        /**
         * @brief Whether the packet passed basic validation.
         * @return True if valid; false otherwise.
         */
        [[nodiscard]] virtual bool isValid() const = 0;
    };
} // namespace reactormq::mqtt::packets
#pragma once

#include "mqtt/packets/packet_type.h"
#include "serialize/serializable.h"

namespace reactormq::mqtt::packets
{
    /**
     * @brief Interface for MQTT control packets.
     * 
     * Base interface for all MQTT control packet types.
     */
    class IControlPacket : public serialize::ISerializable
    {
    public:
        IControlPacket() = default;

        ~IControlPacket() override = default;

        /**
         * @brief Get the size of the serialized packet (excluding fixed header).
         * @return The size of the packet in bytes.
         */
        [[nodiscard]] virtual uint32_t getLength() const = 0;

        /**
         * @brief Get the type of the MQTT control packet.
         * @return The packet type.
         */
        [[nodiscard]] virtual PacketType getPacketType() const = 0;

        /**
         * @brief Get the packet ID of the control packet.
         * @return The packet ID. Returns 0 if the packet doesn't have a packet ID.
         */
        [[nodiscard]] virtual uint16_t getPacketId() const
        {
            return 0;
        }

        /**
         * @brief Check if the packet is valid.
         * @return True if valid, false otherwise.
         */
        [[nodiscard]] virtual bool isValid() const = 0;
    };
}
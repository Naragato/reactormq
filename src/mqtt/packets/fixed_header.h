#pragma once

#include "mqtt/packets/packet_type.h"
#include "mqtt/packets/interface/control_packet.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"
#include "util/logging/logging.h"

#include <bitset>
#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Represents the fixed header for an MQTT control packet.
     * 
     * Handles serialization and deserialization of the fixed header as defined
     * in MQTT 5.0 and 3.1.1 specifications.
     */
    class FixedHeader
    {
    public:
        /**
         * @brief Default constructor.
         */
        FixedHeader()
            : m_flags(0)
              , m_remainingLength(0)
        {
        }

        /**
         * @brief Get the remaining payload length.
         * @return The remaining payload length.
         */
        [[nodiscard]] uint32_t getRemainingLength() const
        {
            return m_remainingLength;
        }

        /**
         * @brief Get the packet type from flags.
         * @return The type of the MQTT packet.
         */
        [[nodiscard]] PacketType getPacketType() const
        {
            return static_cast<PacketType>((m_flags & 0xF0) >> 4);
        }

        /**
         * @brief Get the flags byte.
         * @return The flags of the MQTT packet's fixed header.
         */
        [[nodiscard]] uint8_t getFlags() const
        {
            return m_flags;
        }

        /**
         * @brief Create a fixed header by deserializing from a reader.
         * @param reader Reader for deserialization.
         * @return The deserialized fixed header.
         */
        static FixedHeader create(serialize::ByteReader& reader)
        {
            FixedHeader header;
            header.decode(reader);
            return header;
        }

        /**
         * @brief Create a fixed header from an MQTT control packet with default flags.
         * @param packet Pointer to the MQTT control packet.
         * @return The created fixed header.
         */
        static FixedHeader create(const IControlPacket* packet)
        {
            if (packet == nullptr)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Control packet is null");
                return {};
            }

            uint8_t flags = static_cast<uint8_t>(packet->getPacketType()) << 4;
            const uint32_t remainingLength = packet->getLength();

            if (packet->getPacketType() == PacketType::PubRel ||
                packet->getPacketType() == PacketType::Subscribe ||
                packet->getPacketType() == PacketType::Unsubscribe)
            {
                flags |= 0x02;
            }

            return {flags, remainingLength};
        }

        /**
         * @brief Create a fixed header from an MQTT control packet with custom flags.
         * @param packet Pointer to the MQTT control packet.
         * @param remainingLength Length of the remaining payload (packet size excluding fixed header).
         * @param shouldRetain Whether the packet should be retained.
         * @param qos Quality of service for the packet.
         * @param isDuplicated Whether the packet is a duplicate.
         * @return The created fixed header.
         */
        static FixedHeader create(
            const IControlPacket* packet,
            const uint32_t remainingLength,
            const bool shouldRetain,
            const reactormq::mqtt::QualityOfService qos,
            const bool isDuplicated)
        {
            if (packet == nullptr)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Control packet is null");
                return {};
            }

            uint8_t flags = static_cast<uint8_t>(packet->getPacketType()) << 4;

            if (shouldRetain)
            {
                flags |= 1 << 0;
            }

            flags |= static_cast<uint8_t>(qos) << 1;

            if (isDuplicated && qos != reactormq::mqtt::QualityOfService::AtMostOnce)
            {
                flags |= 1 << 3;
            }

            return {flags, remainingLength};
        }

        /**
         * @brief Encode the fixed header to a writer.
         * @param writer Writer for serialization.
         */
        void encode(serialize::ByteWriter& writer) const
        {
            writer.writeUint8(m_flags);
            serialize::encodeVariableByteInteger(m_remainingLength, writer);
        }

        /**
         * @brief Decode the fixed header from a reader.
         * @param reader Reader for deserialization.
         */
        void decode(serialize::ByteReader& reader)
        {
            uint8_t tempFlags = 0;
            if (!reader.tryReadUint8(tempFlags))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read flags byte");
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            const uint8_t rawPacketType = (tempFlags & 0xF0) >> 4;
            constexpr auto maxAllowedType = static_cast<uint8_t>(PacketType::Max);

            if (rawPacketType >= maxAllowedType)
            {
                const std::bitset<8> bits(tempFlags);
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "FixedHeader",
                    "Invalid packet type. Flags: 0b%s",
                    bits.to_string().c_str());
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            const auto packetType = static_cast<PacketType>(rawPacketType);

            if (packetType == PacketType::None)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Invalid packet type: None");
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            m_remainingLength = serialize::decodeVariableByteInteger(reader);

            const uint8_t remainingLengthFieldSize = serialize::variableByteIntegerSize(m_remainingLength);
            if (remainingLengthFieldSize == 0)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "FixedHeader",
                    "Invalid remaining length: %u",
                    m_remainingLength);
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            m_flags = tempFlags;
        }

        static constexpr const char* kInvalidPacketSize = "Invalid packet size";
        static constexpr const char* kInvalidRemainingLength = "Invalid Remaining Length.";

    private:
        /**
         * @brief Private constructor for creating fixed headers with specific values.
         * @param flags Packet-specific flags.
         * @param remainingLength Length of the remaining payload.
         */
        FixedHeader(const uint8_t flags, const uint32_t remainingLength)
            : m_flags(flags)
              , m_remainingLength(remainingLength)
        {
        }

        /// @brief Stores packet-specific flags (packet type + low nibble flags).
        uint8_t m_flags;

        /// @brief Stores the length of the remaining payload.
        uint32_t m_remainingLength;
    };
}

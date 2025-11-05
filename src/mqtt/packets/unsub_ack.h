#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT UNSUBACK packets.
     * 
     * UNSUBACK is the response to an UNSUBSCRIBE packet.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901187
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718077
     */
    class UnsubAckPacketBase : public TControlPacket<PacketType::UnsubAck>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit UnsubAckPacketBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
              , m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit UnsubAckPacketBase(const uint16_t packetIdentifier)
            : m_packetIdentifier(packetIdentifier)
        {
        }

        /**
         * @brief Get the packet identifier.
         * @return The packet identifier.
         */
        [[nodiscard]] uint16_t getPacketId() const override
        {
            return m_packetIdentifier;
        }

    protected:
        /// @brief The packet identifier for this UNSUBACK packet.
        uint16_t m_packetIdentifier;
    };

    template<ProtocolVersion TProtocolVersion>
    class UnsubAck;

    /**
     * @brief MQTT 3.1.1 UNSUBACK packet.
     * 
     * Contains only packet identifier (2 bytes). No payload in MQTT 3.1.1.
     */
    template<>
    class UnsubAck<ProtocolVersion::V311> final : public UnsubAckPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit UnsubAck(const uint16_t packetIdentifier)
            : UnsubAckPacketBase(packetIdentifier)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit UnsubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : UnsubAckPacketBase(fixedHeader)
        {
            decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return 2 (packet identifier only).
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return 2;
        }

        /**
         * @brief Encode the packet to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;
    };

    /**
     * @brief MQTT 5 UNSUBACK packet.
     * 
     * Contains packet identifier, reason codes array, and properties.
     */
    template<>
    class UnsubAck<ProtocolVersion::V5> final : public UnsubAckPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier, reason codes, and properties.
         * @param packetIdentifier The packet identifier.
         * @param reasonCodes Vector of reason codes for each topic filter.
         * @param properties The properties (default: empty).
         */
        explicit UnsubAck(
            const uint16_t packetIdentifier,
            const std::vector<ReasonCode>& reasonCodes,
            properties::Properties properties = properties::Properties{})
            : UnsubAckPacketBase(packetIdentifier)
              , m_reasonCodes(reasonCodes)
              , m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit UnsubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : UnsubAckPacketBase(fixedHeader)
        {
            decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return The calculated length.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            uint32_t length = 2;
            length += m_properties.getLength(true); // Include VBI length field
            length += static_cast<uint32_t>(m_reasonCodes.size());
            return length;
        }

        /**
         * @brief Encode the packet to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;

        /**
         * @brief Get the reason codes for each topic filter.
         * Ordered by the order of the topic filters in the unsubscribe packet.
         * @return The reason codes for each topic filter.
         */
        [[nodiscard]] const std::vector<ReasonCode>& getReasonCodes() const
        {
            return m_reasonCodes;
        }

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
        }

    private:
        /// @brief The reason codes for each unsubscribe topic filter.
        std::vector<ReasonCode> m_reasonCodes;

        /// @brief The properties for this UNSUBACK packet.
        properties::Properties m_properties;
    };

    using UnsubAck3 = UnsubAck<ProtocolVersion::V311>;
    using UnsubAck5 = UnsubAck<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

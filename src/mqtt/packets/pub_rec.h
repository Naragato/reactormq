#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"

#include <utility>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base for MQTT PUBREC packets.
     * Acknowledges receipt of a QoS 2 PUBLISH (step 1 of QoS 2 flow).
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901131
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718048
     */
    class PubRecPacketBase : public TControlPacket<PacketType::PubRec>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit PubRecPacketBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader), m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit PubRecPacketBase(const uint16_t packetIdentifier)
            : m_packetIdentifier(packetIdentifier)
        {
        }

        /**
         * @brief Packet identifier.
         * @return Packet id.
         */
        [[nodiscard]] uint16_t getPacketId() const override
        {
            return m_packetIdentifier;
        }

    protected:
        uint16_t m_packetIdentifier;
    };

    template<ProtocolVersion TProtocolVersion>
    class PubRec;

    /**
     * @brief MQTT 3.1.1 PubRec packet.
     * 
     * Contains only packet identifier (2 bytes).
     */
    template<>
    class PubRec<ProtocolVersion::V311> final : public PubRecPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit PubRec(const uint16_t packetIdentifier)
            : PubRecPacketBase(packetIdentifier)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubRec(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : PubRecPacketBase(fixedHeader)
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
     * @brief MQTT 5 PubRec packet.
     * 
     * Contains packet identifier, reason code, and properties.
     */
    template<>
    class PubRec<ProtocolVersion::V5> final : public PubRecPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier, reason code, and properties.
         * @param packetIdentifier The packet identifier.
         * @param reasonCode The reason code (default: Success).
         * @param properties The properties (default: empty).
         */
        explicit PubRec(
            const uint16_t packetIdentifier,
            const ReasonCode reasonCode = ReasonCode::Success,
            properties::Properties properties = properties::Properties{})
            : PubRecPacketBase(packetIdentifier), m_reasonCode(reasonCode), m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubRec(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : PubRecPacketBase(fixedHeader), m_reasonCode(ReasonCode::Success)
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

            if (m_reasonCode == ReasonCode::Success && m_properties.getLength(false) == 0)
            {
                return length;
            }

            length += 1;

            const uint32_t propsLength = m_properties.getLength(false);
            length += serialize::variableByteIntegerSize(propsLength) + propsLength;

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
         * @brief Get the reason code.
         * @return The reason code.
         */
        [[nodiscard]] ReasonCode getReasonCode() const
        {
            return m_reasonCode;
        }

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
        }

    protected:
        ReasonCode m_reasonCode;
        properties::Properties m_properties;
    };

    using PubRec3 = PubRec<ProtocolVersion::V311>;
    using PubRec5 = PubRec<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets
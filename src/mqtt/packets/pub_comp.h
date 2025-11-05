#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"
#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT PubComp packets.
     * 
     * PubComp is the response to a PUBLISH packet with QoS 2 (third part of QoS 2 handshake).
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901151
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718058
     */
    class PubCompPacketBase : public TControlPacket<PacketType::PubComp>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit PubCompPacketBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
              , m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit PubCompPacketBase(const uint16_t packetIdentifier)
            : m_packetIdentifier(packetIdentifier)
        {
        }

        /**
         * @brief Get the packet identifier.
         * @return The packet identifier.
         */
        uint16_t getPacketId() const override
        {
            return m_packetIdentifier;
        }

    protected:
        /// @brief The packet identifier for this PubComp packet.
        uint16_t m_packetIdentifier;
    };

    template<ProtocolVersion TProtocolVersion>
    class PubComp;

    /**
     * @brief MQTT 3.1.1 PubComp packet.
     * 
     * Contains only packet identifier (2 bytes).
     */
    template<>
    class PubComp<ProtocolVersion::V311> final : public PubCompPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit PubComp(const uint16_t packetIdentifier)
            : PubCompPacketBase(packetIdentifier)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubComp(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : PubCompPacketBase(fixedHeader)
        {
            decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return 2 (packet identifier only).
         */
        uint32_t getLength() const override
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
     * @brief MQTT 5 PubComp packet.
     * 
     * Contains packet identifier, reason code, and properties.
     */
    template<>
    class PubComp<ProtocolVersion::V5> final : public PubCompPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier, reason code, and properties.
         * @param packetIdentifier The packet identifier.
         * @param reasonCode The reason code (no default - must be specified).
         * @param properties The properties (default: empty).
         */
        explicit PubComp(
            const uint16_t packetIdentifier,
            const ReasonCode reasonCode,
            const properties::Properties& properties = properties::Properties{})
            : PubCompPacketBase(packetIdentifier)
              , m_reasonCode(reasonCode)
              , m_properties(properties)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubComp(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : PubCompPacketBase(fixedHeader)
              , m_reasonCode(ReasonCode::Success)
        {
            decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return The calculated length.
         */
        uint32_t getLength() const override
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
        ReasonCode getReasonCode() const
        {
            return m_reasonCode;
        }

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        const properties::Properties& getProperties() const
        {
            return m_properties;
        }

    protected:
        /// @brief The reason code for this PubComp packet.
        ReasonCode m_reasonCode;

        /// @brief The properties for this PubComp packet.
        properties::Properties m_properties;
    };

    using PubComp3 = PubComp<ProtocolVersion::V311>;
    using PubComp5 = PubComp<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

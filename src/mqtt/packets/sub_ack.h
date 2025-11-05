#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "reactormq/mqtt/subscribe_return_code.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT SUBACK packets.
     * 
     * SUBACK is sent by the server to acknowledge subscription requests.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901171
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718068
     */
    class SubAckPacketBase : public TControlPacket<PacketType::SubAck>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit SubAckPacketBase(const FixedHeader& fixedHeader)
            : TControlPacket<PacketType::SubAck>(fixedHeader)
              , m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit SubAckPacketBase(const uint16_t packetIdentifier)
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
        /// @brief The packet identifier for this SUBACK packet.
        uint16_t m_packetIdentifier;
    };

    template<ProtocolVersion TProtocolVersion>
    class SubAck;

    /**
     * @brief MQTT 3.1.1 SUBACK packet.
     * 
     * Contains packet identifier and array of subscription return codes.
     */
    template<>
    class SubAck<ProtocolVersion::V311> final : public SubAckPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier and return codes.
         * @param packetIdentifier The packet identifier.
         * @param returnCodes The return codes for each subscription.
         */
        explicit SubAck(
            const uint16_t packetIdentifier,
            const std::vector<SubscribeReturnCode>& returnCodes)
            : SubAckPacketBase(packetIdentifier)
              , m_returnCodes(returnCodes)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit SubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : SubAckPacketBase(fixedHeader)
        {
            decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return The calculated length.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return 2 + static_cast<uint32_t>(m_returnCodes.size());
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
         * @brief Get the return codes for each topic filter.
         * @return Return codes for each topic filter.
         */
        [[nodiscard]] std::vector<SubscribeReturnCode> getReturnCodes() const
        {
            return m_returnCodes;
        }

    private:
        /// @brief The return codes for each subscription.
        std::vector<SubscribeReturnCode> m_returnCodes;
    };

    /**
     * @brief MQTT 5 SUBACK packet.
     * 
     * Contains packet identifier, properties, and array of reason codes.
     */
    template<>
    class SubAck<ProtocolVersion::V5> final : public SubAckPacketBase
    {
    public:
        /**
         * @brief Constructor with packet identifier, reason codes, and properties.
         * @param packetIdentifier The packet identifier.
         * @param reasonCodes The reason codes for each subscription.
         * @param properties The properties (default: empty).
         */
        explicit SubAck(
            const uint16_t packetIdentifier,
            const std::vector<ReasonCode>& reasonCodes,
            properties::Properties properties = properties::Properties{})
            : SubAckPacketBase(packetIdentifier)
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
        explicit SubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : SubAckPacketBase(fixedHeader)
        {
            decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return The calculated length.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return 2 + m_properties.getLength() + static_cast<uint32_t>(m_reasonCodes.size());
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
         * Ordered by the order of the topic filters in the subscribe packet.
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
        /// @brief The reason codes for each subscription.
        std::vector<ReasonCode> m_reasonCodes;

        /// @brief The properties for this SUBACK packet.
        properties::Properties m_properties;
    };

    using SubAck3 = SubAck<ProtocolVersion::V311>;
    using SubAck5 = SubAck<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

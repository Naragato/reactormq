#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"

#include <string>
#include <utility>
#include <vector>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT UNSUBSCRIBE packets.
     * 
     * UNSUBSCRIBE is sent by the Client to unsubscribe from one or more topics.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901179
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718072
     */
    class UnsubscribeBase : public TControlPacket<PacketType::Unsubscribe>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit UnsubscribeBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
              , m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with topic filters and packet identifier.
         * @param topicFilters The topic filter strings to unsubscribe from.
         * @param packetIdentifier The packet identifier.
         */
        explicit UnsubscribeBase(
            std::vector<std::string> topicFilters,
            const uint16_t packetIdentifier)
            : m_packetIdentifier(packetIdentifier)
              , m_topicFilters(std::move(topicFilters))
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

        /**
         * @brief Get the topic filters.
         * @return The topic filter strings.
         */
        [[nodiscard]] const std::vector<std::string>& getTopicFilters() const
        {
            return m_topicFilters;
        }

    protected:
        /// @brief The packet identifier for this UNSUBSCRIBE packet.
        uint16_t m_packetIdentifier;

        /// @brief The topic filter strings to unsubscribe from.
        std::vector<std::string> m_topicFilters;
    };

    template<ProtocolVersion TProtocolVersion>
    class Unsubscribe;

    /**
     * @brief MQTT 3.1.1 UNSUBSCRIBE packet.
     * 
     * Contains packet identifier and array of topic filter strings.
     */
    template<>
    class Unsubscribe<ProtocolVersion::V311> final : public UnsubscribeBase
    {
    public:
        /**
         * @brief Constructor with topic filters and packet identifier.
         * @param topicFilters The topic filter strings to unsubscribe from.
         * @param packetIdentifier The packet identifier.
         */
        explicit Unsubscribe(
            std::vector<std::string> topicFilters,
            const uint16_t packetIdentifier)
            : UnsubscribeBase(std::move(topicFilters), packetIdentifier)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Unsubscribe(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : UnsubscribeBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            uint32_t length = 2;

            for (const auto& topicFilter: m_topicFilters)
            {
                length += kStringLengthFieldSize;
                length += static_cast<uint32_t>(topicFilter.length());
            }

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
    };

    /**
     * @brief MQTT 5 UNSUBSCRIBE packet.
     * 
     * Contains packet identifier, properties, and array of topic filter strings.
     */
    template<>
    class Unsubscribe<ProtocolVersion::V5> final : public UnsubscribeBase
    {
    public:
        /**
         * @brief Constructor with topic filters, packet identifier, and properties.
         * @param topicFilters The topic filter strings to unsubscribe from.
         * @param packetIdentifier The packet identifier.
         * @param properties The UNSUBSCRIBE properties (default: empty).
         */
        explicit Unsubscribe(
            std::vector<std::string> topicFilters,
            const uint16_t packetIdentifier,
            properties::Properties properties = properties::Properties{})
            : UnsubscribeBase(std::move(topicFilters), packetIdentifier)
              , m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Unsubscribe(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : UnsubscribeBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            uint32_t length = 2;

            length += m_properties.getLength();

            for (const auto& topicFilter: m_topicFilters)
            {
                length += kStringLengthFieldSize;
                length += static_cast<uint32_t>(topicFilter.length());
            }

            return length;
        }

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
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

    protected:
        /// @brief The properties for this UNSUBSCRIBE packet.
        properties::Properties m_properties;
    };

    using Unsubscribe3 = Unsubscribe<ProtocolVersion::V311>;
    using Unsubscribe5 = Unsubscribe<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/topic_filter.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base for MQTT SUBSCRIBE packets.
     *
     * Sent by a client to request one or more topic filters.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901161
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718063
     */
    class SubscribePacketBase : public TControlPacket<PacketType::Subscribe>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit SubscribePacketBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader), m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with topic filters and packet identifier.
         * @param topicFilters The topic filters to subscribe to.
         * @param packetIdentifier The packet identifier.
         */
        explicit SubscribePacketBase(
            std::vector<TopicFilter> topicFilters,
            const uint16_t packetIdentifier)
            : m_packetIdentifier(packetIdentifier), m_topicFilters(std::move(topicFilters))
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

        /**
         * @brief Topic filters requested by this subscribe.
         */
        [[nodiscard]] const std::vector<TopicFilter>& getTopicFilters() const
        {
            return m_topicFilters;
        }

    protected:
        uint16_t m_packetIdentifier;
        std::vector<TopicFilter> m_topicFilters;
    };

    template<ProtocolVersion TProtocolVersion>
    class Subscribe;

    /**
     * @brief MQTT 3.1.1 SUBSCRIBE packet.
     * Contains packet identifier and array of topic filters with QoS.
     */
    template<>
    class Subscribe<ProtocolVersion::V311> final : public SubscribePacketBase
    {
    public:
        /**
         * @brief Constructor with topic filters and packet identifier.
         * @param topicFilters The topic filters to subscribe to.
         * @param packetIdentifier The packet identifier.
         */
        explicit Subscribe(
            std::vector<TopicFilter> topicFilters,
            const uint16_t packetIdentifier)
            : SubscribePacketBase(std::move(topicFilters), packetIdentifier)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Subscribe(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : SubscribePacketBase(fixedHeader)
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

            for (const auto& topicFilter: m_topicFilters)
            {
                length += kStringLengthFieldSize;
                length += static_cast<uint32_t>(topicFilter.getFilter().length());
                length += sizeof(QualityOfService);
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
     * @brief MQTT 5 SUBSCRIBE packet.
     * 
     * Contains packet identifier, properties, and array of topic filters with subscription options.
     */
    template<>
    class Subscribe<ProtocolVersion::V5> final : public SubscribePacketBase
    {
    public:
        /**
         * @brief Constructor with topic filters, packet identifier, and properties.
         * @param topicFilters The topic filters to subscribe to.
         * @param packetIdentifier The packet identifier.
         * @param properties The properties (default: empty).
         */
        explicit Subscribe(
            std::vector<TopicFilter> topicFilters,
            const uint16_t packetIdentifier,
            properties::Properties properties = properties::Properties{})
            : SubscribePacketBase(std::move(topicFilters), packetIdentifier), m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Subscribe(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : SubscribePacketBase(fixedHeader)
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

            length += m_properties.getLength(true);

            for (const auto& topicFilter: m_topicFilters)
            {
                length += kStringLengthFieldSize;
                length += static_cast<uint32_t>(topicFilter.getFilter().length());
                length += 1; // Subscription options byte
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

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
        }

    private:
        static constexpr std::byte kQosMask{std::byte{0x3}};
        static constexpr std::byte kNoLocalBit{std::byte{0x1} << 2};
        static constexpr std::byte kRetainAsPublishedBit{std::byte{0x1} << 3};
        static constexpr std::byte kRetainHandlingMask{std::byte{0x3}};
        static constexpr int kRetainHandlingShift{4};
        static constexpr std::byte kReservedMask{std::byte{0xC0}};
        properties::Properties m_properties;
    };

    using Subscribe3 = Subscribe<ProtocolVersion::V311>;
    using Subscribe5 = Subscribe<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

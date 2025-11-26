//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/topic_filter.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT SUBSCRIBE packets.
         */
        template<ProtocolVersion V>
        struct SubscribeTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 SUBSCRIBE packets.
         */
        template<>
        struct SubscribeTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 SUBSCRIBE packets.
         */
        template<>
        struct SubscribeTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT SUBSCRIBE packets.
     */
    class ISubscribePacket : public TControlPacket<PacketType::Subscribe>
    {
    public:
        ISubscribePacket() = default;

        explicit ISubscribePacket(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
        {
        }

        /**
         * @brief Get the topic filters.
         * @return The topic filters.
         */
        [[nodiscard]] virtual const std::vector<TopicFilter>& getTopicFilters() const = 0;
    };

    /**
     * @brief MQTT SUBSCRIBE packet for a specific protocol version.
     *
     * Sent by a client to request one or more topic filters.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901161
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718063
     */
    template<ProtocolVersion TProtocolVersion>
    class Subscribe final : public ISubscribePacket
    {
        using Traits = detail::SubscribeTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with topic filters and packet identifier.
         * @param topicFilters The topic filters to subscribe to.
         * @param packetId The packet identifier.
         */
        explicit Subscribe(std::vector<TopicFilter> topicFilters, uint16_t packetId);

        /**
         * @brief Constructor with topic filters, packet identifier, and properties (V5).
         * @param topicFilters The topic filters to subscribe to.
         * @param packetId The packet identifier.
         * @param properties The properties.
         */
        explicit Subscribe(std::vector<TopicFilter> topicFilters, uint16_t packetId, PropertiesStorage&& properties)
            requires(detail::SubscribeTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Subscribe(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        [[nodiscard]] const std::vector<TopicFilter>& getTopicFilters() const override;

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::SubscribeTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        std::vector<TopicFilter> m_topicFilters;
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;

        // V5 specific constants for decoding options
        static constexpr std::byte kQosMask{ std::byte{ 0x3 } };
        static constexpr std::byte kNoLocalBit{ std::byte{ 0x1 } << 2 };
        static constexpr std::byte kRetainAsPublishedBit{ std::byte{ 0x1 } << 3 };
        static constexpr std::byte kRetainHandlingMask{ std::byte{ 0x3 } };
        static constexpr int kRetainHandlingShift{ 4 };
        static constexpr std::byte kReservedMask{ std::byte{ 0xC0 } };
    };

    using Subscribe3 = Subscribe<ProtocolVersion::V311>;
    using Subscribe5 = Subscribe<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeSubscribeToWriter(
        serialize::ByteWriter& writer,
        const std::vector<TopicFilter>& topicFilters,
        uint16_t packetId,
        typename detail::SubscribeTraits<V>::PropertiesType&& properties = {});

    extern template void encodeSubscribeToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&,
        const std::vector<TopicFilter>&,
        uint16_t,
        detail::SubscribeTraits<ProtocolVersion::V311>::PropertiesType&&);
    extern template void encodeSubscribeToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&, const std::vector<TopicFilter>&, uint16_t, detail::SubscribeTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
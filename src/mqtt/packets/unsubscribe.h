//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "serialize/bytes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT UNSUBSCRIBE packets.
         */
        template<ProtocolVersion V>
        struct UnsubscribeTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 UNSUBSCRIBE packets.
         */
        template<>
        struct UnsubscribeTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 UNSUBSCRIBE packets.
         */
        template<>
        struct UnsubscribeTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT UNSUBSCRIBE packets.
     */
    class IUnsubscribePacket : public TControlPacket<PacketType::Unsubscribe>
    {
    public:
        IUnsubscribePacket() = default;

        using TControlPacket::TControlPacket;

        /**
         * @brief Get the topic filters.
         * @return The topic filters.
         */
        [[nodiscard]] virtual const std::vector<std::string>& getTopicFilters() const = 0;
    };

    /**
     * @brief MQTT UNSUBSCRIBE packet for a specific protocol version.
     *
     * Sent by a client to remove one or more topic filters.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901179
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718072
     */
    template<ProtocolVersion TProtocolVersion>
    class Unsubscribe final : public IUnsubscribePacket
    {
        using Traits = detail::UnsubscribeTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with topic filters and packet identifier.
         * @param topicFilters The topic filters to unsubscribe from.
         * @param packetId The packet identifier.
         */
        explicit Unsubscribe(std::vector<std::string> topicFilters, uint16_t packetId);

        /**
         * @brief Constructor with topic filters, packet identifier, and properties (V5).
         * @param topicFilters The topic filters to unsubscribe from.
         * @param packetId The packet identifier.
         * @param properties The properties.
         */
        explicit Unsubscribe(std::vector<std::string> topicFilters, uint16_t packetId, PropertiesStorage&& properties)
            requires(detail::UnsubscribeTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Unsubscribe(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        [[nodiscard]] const std::vector<std::string>& getTopicFilters() const override;

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::UnsubscribeTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        std::vector<std::string> m_topicFilters;
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;
    };

    using Unsubscribe3 = Unsubscribe<ProtocolVersion::V311>;
    using Unsubscribe5 = Unsubscribe<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeUnsubscribeToWriter(
        serialize::ByteWriter& writer,
        const std::vector<std::string>& topicFilters,
        uint16_t packetId,
        typename detail::UnsubscribeTraits<V>::PropertiesType&& properties = {});

    extern template void encodeUnsubscribeToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&,
        const std::vector<std::string>&,
        uint16_t,
        detail::UnsubscribeTraits<ProtocolVersion::V311>::PropertiesType&&);

    extern template void encodeUnsubscribeToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&,
        const std::vector<std::string>&,
        uint16_t,
        detail::UnsubscribeTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
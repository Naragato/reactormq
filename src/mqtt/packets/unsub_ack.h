//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"

#include <cstdint>
#include <vector>

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT UNSUBACK packets.
         */
        template<ProtocolVersion V>
        struct UnsubAckTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 UNSUBACK packets.
         */
        template<>
        struct UnsubAckTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            static constexpr bool HasReasonCodes = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 UNSUBACK packets.
         */
        template<>
        struct UnsubAckTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            static constexpr bool HasReasonCodes = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT UNSUBACK packets.
     */
    class IUnsubAckPacket : public TControlPacket<PacketType::UnsubAck>
    {
    public:
        IUnsubAckPacket() = default;

        using TControlPacket::TControlPacket;
    };

    /**
     * @brief MQTT UNSUBACK packet for a specific protocol version.
     *
     * UNSUBACK is the response to an UNSUBSCRIBE packet.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901187
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718077
     */
    template<ProtocolVersion TProtocolVersion>
    class UnsubAck final : public IUnsubAckPacket
    {
        using Traits = detail::UnsubAckTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with packet identifier (V3.1.1).
         * @param packetId The packet identifier.
         */
        explicit UnsubAck(uint16_t packetId);

        /**
         * @brief Constructor with packet identifier, reason codes, and properties (V5).
         * @param packetId The packet identifier.
         * @param reasonCodes The reason codes.
         * @param properties The properties.
         */
        explicit UnsubAck(uint16_t packetId, std::vector<ReasonCode> reasonCodes, PropertiesStorage&& properties = {})
            requires(detail::UnsubAckTraits<TProtocolVersion>::HasReasonCodes);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit UnsubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        /**
         * @brief Get the reason codes for each topic filter (MQTT 5 only).
         * @return The reason codes.
         */
        [[nodiscard]] const std::vector<ReasonCode>& getReasonCodes() const
            requires(detail::UnsubAckTraits<TProtocolVersion>::HasReasonCodes);

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::UnsubAckTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        std::vector<ReasonCode> m_reasonCodes;
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;
    };

    using UnsubAck3 = UnsubAck<ProtocolVersion::V311>;
    using UnsubAck5 = UnsubAck<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeUnsubAckToWriter(
        serialize::ByteWriter& writer,
        uint16_t packetId,
        const std::vector<ReasonCode>& reasonCodes = {},
        typename detail::UnsubAckTraits<V>::PropertiesType&& properties = {});

    extern template void encodeUnsubAckToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&, uint16_t, const std::vector<ReasonCode>&, detail::UnsubAckTraits<ProtocolVersion::V311>::PropertiesType&&);
    extern template void encodeUnsubAckToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&, uint16_t, const std::vector<ReasonCode>&, detail::UnsubAckTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
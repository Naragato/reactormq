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

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT PUBREC packets.
         */
        template<ProtocolVersion V>
        struct PubRecTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 PUBREC packets.
         */
        template<>
        struct PubRecTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            static constexpr bool HasReasonCode = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 PUBREC packets.
         */
        template<>
        struct PubRecTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            static constexpr bool HasReasonCode = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT PUBREC packets.
     */
    class IPubRecPacket : public TControlPacket<PacketType::PubRec>
    {
    public:
        IPubRecPacket() = default;

        using TControlPacket::TControlPacket;
    };

    /**
     * @brief MQTT PUBREC packet for a specific protocol version.
     *
     * PUBREC is the first packet in the QoS 2 protocol exchange.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901131
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718048
     */
    template<ProtocolVersion TProtocolVersion>
    class PubRec final : public IPubRecPacket
    {
        using Traits = detail::PubRecTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with packet identifier (V3.1.1/V5).
         * @param packetId The packet identifier.
         */
        explicit PubRec(uint16_t packetId);

        /**
         * @brief Constructor with packet identifier, reason code, and properties (V5 only).
         * @param packetId The packet identifier.
         * @param reasonCode The reason code.
         * @param properties The properties.
         */
        explicit PubRec(uint16_t packetId, ReasonCode reasonCode, PropertiesStorage&& properties = {})
            requires(detail::PubRecTraits<TProtocolVersion>::HasReasonCode);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubRec(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        /**
         * @brief Get the reason code (MQTT 5 only).
         * @return The reason code.
         */
        [[nodiscard]] ReasonCode getReasonCode() const
            requires(detail::PubRecTraits<TProtocolVersion>::HasReasonCode);

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::PubRecTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        ReasonCode m_reasonCode{ ReasonCode::Success };
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;
    };

    using PubRec3 = PubRec<ProtocolVersion::V311>;
    using PubRec5 = PubRec<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodePubRecToWriter(
        serialize::ByteWriter& writer,
        uint16_t packetId,
        ReasonCode reasonCode = ReasonCode::Success,
        typename detail::PubRecTraits<V>::PropertiesType&& properties = {});

    extern template void encodePubRecToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubRecTraits<ProtocolVersion::V311>::PropertiesType&&);

    extern template void encodePubRecToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubRecTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
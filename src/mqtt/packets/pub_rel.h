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

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT PUBREL packets.
         */
        template<ProtocolVersion V>
        struct PubRelTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 PUBREL packets.
         */
        template<>
        struct PubRelTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            static constexpr bool HasReasonCode = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 PUBREL packets.
         */
        template<>
        struct PubRelTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            static constexpr bool HasReasonCode = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT PUBREL packets.
     */
    class IPubRelPacket : public TControlPacket<PacketType::PubRel>
    {
    public:
        IPubRelPacket() = default;

        using TControlPacket::TControlPacket;
    };

    /**
     * @brief MQTT PUBREL packet for a specific protocol version.
     *
     * PUBREL is the second packet in the QoS 2 protocol exchange.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901141
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718053
     */
    template<ProtocolVersion TProtocolVersion>
    class PubRel final : public IPubRelPacket
    {
        using Traits = detail::PubRelTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with packet identifier (V3.1.1/V5).
         * @param packetId The packet identifier.
         */
        explicit PubRel(uint16_t packetId);

        /**
         * @brief Constructor with packet identifier, reason code, and properties (V5 only).
         * @param packetId The packet identifier.
         * @param reasonCode The reason code.
         * @param properties The properties.
         */
        explicit PubRel(uint16_t packetId, ReasonCode reasonCode, PropertiesStorage&& properties = {})
            requires(detail::PubRelTraits<TProtocolVersion>::HasReasonCode);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubRel(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        /**
         * @brief Get the reason code (MQTT 5 only).
         * @return The reason code.
         */
        [[nodiscard]] ReasonCode getReasonCode() const
            requires(detail::PubRelTraits<TProtocolVersion>::HasReasonCode);

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::PubRelTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        ReasonCode m_reasonCode{ ReasonCode::Success };
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;
    };

    using PubRel3 = PubRel<ProtocolVersion::V311>;
    using PubRel5 = PubRel<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodePubRelToWriter(
        serialize::ByteWriter& writer,
        uint16_t packetId,
        ReasonCode reasonCode = ReasonCode::Success,
        typename detail::PubRelTraits<V>::PropertiesType&& properties = {});

    extern template void encodePubRelToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubRelTraits<ProtocolVersion::V311>::PropertiesType&&);
    extern template void encodePubRelToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubRelTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
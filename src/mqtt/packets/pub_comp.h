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
         * @brief Traits for MQTT PUBCOMP packets.
         */
        template<ProtocolVersion V>
        struct PubCompTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 PUBCOMP packets.
         */
        template<>
        struct PubCompTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            static constexpr bool HasReasonCode = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 PUBCOMP packets.
         */
        template<>
        struct PubCompTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            static constexpr bool HasReasonCode = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT PUBCOMP packets.
     */
    class IPubCompPacket : public TControlPacket<PacketType::PubComp>
    {
    public:
        IPubCompPacket() = default;

        using TControlPacket::TControlPacket;
    };

    /**
     * @brief MQTT PUBCOMP packet for a specific protocol version.
     *
     * PubComp is the response to a PUBLISH packet with QoS 2 (third part of QoS 2 handshake).
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901151
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718058
     */
    template<ProtocolVersion TProtocolVersion>
    class PubComp final : public IPubCompPacket
    {
        using Traits = detail::PubCompTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with packet identifier (V3.1.1/V5).
         * @param packetId The packet identifier.
         */
        explicit PubComp(uint16_t packetId);

        /**
         * @brief Constructor with packet identifier, reason code, and properties (V5 only).
         * @param packetId The packet identifier.
         * @param reasonCode The reason code.
         * @param properties The properties.
         */
        explicit PubComp(uint16_t packetId, ReasonCode reasonCode, PropertiesStorage&& properties = {})
            requires(detail::PubCompTraits<TProtocolVersion>::HasReasonCode);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubComp(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        /**
         * @brief Get the reason code (MQTT 5 only).
         * @return The reason code.
         */
        [[nodiscard]] ReasonCode getReasonCode() const
            requires(detail::PubCompTraits<TProtocolVersion>::HasReasonCode);

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::PubCompTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        ReasonCode m_reasonCode{ ReasonCode::Success };
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;
    };

    using PubComp3 = PubComp<ProtocolVersion::V311>;
    using PubComp5 = PubComp<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodePubCompToWriter(
        serialize::ByteWriter& writer,
        uint16_t packetId,
        ReasonCode reasonCode = ReasonCode::Success,
        typename detail::PubCompTraits<V>::PropertiesType&& properties = {});

    extern template void encodePubCompToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubCompTraits<ProtocolVersion::V311>::PropertiesType&&);
    extern template void encodePubCompToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubCompTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
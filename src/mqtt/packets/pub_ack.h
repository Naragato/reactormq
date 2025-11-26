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
    template<ProtocolVersion TProtocolVersion>
    class PubAck;

    namespace detail
    {
        /**
         * @brief Traits for MQTT PUBACK packets.
         */
        template<ProtocolVersion V>
        struct PubAckTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 PUBACK packets.
         */
        template<>
        struct PubAckTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 PUBACK packets.
         */
        template<>
        struct PubAckTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief MQTT PUBACK packet for a specific protocol version.
     *
     * Acknowledges a QoS 1 PUBLISH.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901121
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718043
     */
    template<ProtocolVersion TProtocolVersion>
    class PubAck final : public TControlPacket<PacketType::PubAck>
    {
        using Traits = detail::PubAckTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor with packet identifier.
         * @param packetIdentifier The packet identifier.
         */
        explicit PubAck(std::uint16_t packetIdentifier);

        /**
         * @brief Constructor with packet identifier, reason code, and properties (MQTT 5 only).
         * @param packetIdentifier The packet identifier.
         * @param reasonCode The reason code.
         * @param properties The properties.
         */
        explicit PubAck(std::uint16_t packetIdentifier, ReasonCode reasonCode, typename Traits::PropertiesType&& properties = {})
            requires(detail::PubAckTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit PubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        /**
         * @brief Packet identifier.
         * @return Packet identifier.
         */
        [[nodiscard]] std::uint16_t getPacketId() const override;

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] std::uint32_t getLength() const override;

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
         * @brief Get the reason code (MQTT 5 only).
         * @return The reason code.
         */
        [[nodiscard]] ReasonCode getReasonCode() const
            requires(detail::PubAckTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const typename Traits::PropertiesType& getProperties() const
            requires(detail::PubAckTraits<TProtocolVersion>::HasProperties);

    private:
        std::uint16_t m_packetIdentifier{};
        ReasonCode m_reasonCode{ ReasonCode::Success };
        PropertiesStorage m_properties{};
    };

    /**
     * @brief Alias for MQTT 3.1.1 PUBACK packet.
     */
    using PubAck3 = PubAck<ProtocolVersion::V311>;

    /**
     * @brief Alias for MQTT 5 PUBACK packet.
     */
    using PubAck5 = PubAck<ProtocolVersion::V5>;

    extern template class PubAck<ProtocolVersion::V311>;
    extern template class PubAck<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodePubAckToWriter(
        serialize::ByteWriter& writer,
        uint16_t packetId,
        ReasonCode reasonCode = ReasonCode::Success,
        typename detail::PubAckTraits<V>::PropertiesType&& properties = {});

    extern template void encodePubAckToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubAckTraits<ProtocolVersion::V311>::PropertiesType&&);
    extern template void encodePubAckToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&, uint16_t, ReasonCode, detail::PubAckTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
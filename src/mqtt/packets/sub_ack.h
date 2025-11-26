//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "reactormq/mqtt/subscribe_return_code.h"
#include "serialize/bytes.h"

#include <cstdint>
#include <vector>

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT SUBACK packets.
         */
        template<ProtocolVersion V>
        struct SubAckTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 SUBACK packets.
         */
        template<>
        struct SubAckTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            using PropertiesType = properties::EmptyProperties;
            using ReasonCodeType = SubscribeReturnCode;
        };

        /**
         * @brief Traits specialization for MQTT 5 SUBACK packets.
         */
        template<>
        struct SubAckTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            using PropertiesType = properties::Properties;
            using ReasonCodeType = ReasonCode;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT SUBACK packets.
     */
    class ISubAckPacket : public TControlPacket<PacketType::SubAck>
    {
    public:
        ISubAckPacket() = default;

        explicit ISubAckPacket(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
        {
        }
    };

    /**
     * @brief MQTT SUBACK packet for a specific protocol version.
     *
     * Acknowledges a SUBSCRIBE request with per-filter results.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901171
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718068
     */
    template<ProtocolVersion TProtocolVersion>
    class SubAck final : public ISubAckPacket
    {
        using Traits = detail::SubAckTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;
        using ReasonCodeType = typename Traits::ReasonCodeType;

    public:
        /**
         * @brief Constructor with packet identifier and reason codes.
         * @param packetId The packet identifier.
         * @param reasonCodes The reason codes (or return codes).
         */
        explicit SubAck(uint16_t packetId, std::vector<ReasonCodeType> reasonCodes);

        /**
         * @brief Constructor with packet identifier, reason codes, and properties (V5 only).
         * @param packetId The packet identifier.
         * @param reasonCodes The reason codes.
         * @param properties The properties.
         */
        explicit SubAck(uint16_t packetId, std::vector<ReasonCodeType> reasonCodes, PropertiesStorage&& properties)
            requires(detail::SubAckTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit SubAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        [[nodiscard]] uint32_t getLength() const override;

        void encode(serialize::ByteWriter& writer) const override;

        bool decode(serialize::ByteReader& reader) override;

        [[nodiscard]] uint16_t getPacketId() const override;

        /**
         * @brief Get the reason codes (or return codes).
         * @return The reason codes.
         */
        [[nodiscard]] const std::vector<ReasonCodeType>& getReasonCodes() const;

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::SubAckTraits<TProtocolVersion>::HasProperties);

    private:
        uint16_t m_packetIdentifier{ 0 };
        std::vector<ReasonCodeType> m_reasonCodes;
        PropertiesStorage m_properties{};

        [[nodiscard]] uint32_t getPropertiesLength() const;
    };

    using SubAck3 = SubAck<ProtocolVersion::V311>;
    using SubAck5 = SubAck<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeSubAckToWriter(
        serialize::ByteWriter& writer,
        uint16_t packetId,
        const std::vector<typename detail::SubAckTraits<V>::ReasonCodeType>& reasonCodes,
        typename detail::SubAckTraits<V>::PropertiesType&& properties = {});

    extern template void encodeSubAckToWriter<ProtocolVersion::V311>(
        serialize::ByteWriter&,
        uint16_t,
        const std::vector<detail::SubAckTraits<ProtocolVersion::V311>::ReasonCodeType>&,
        detail::SubAckTraits<ProtocolVersion::V311>::PropertiesType&&);
    extern template void encodeSubAckToWriter<ProtocolVersion::V5>(
        serialize::ByteWriter&,
        uint16_t,
        const std::vector<detail::SubAckTraits<ProtocolVersion::V5>::ReasonCodeType>&,
        detail::SubAckTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
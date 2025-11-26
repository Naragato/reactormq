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

#include <cstddef>
#include <type_traits>

namespace reactormq::mqtt::packets
{
    /**
     * @brief MQTT 3.1.1 Connect Return Code.
     *
     * These are the return codes used in MQTT 3.1.1 CONNACK packets.
     */
    enum class ConnectReturnCode : std::uint8_t
    {
        Accepted = 0x00,
        RefusedProtocolVersion = 0x01,
        RefusedIdentifierRejected = 0x02,
        RefusedServerUnavailable = 0x03,
        RefusedBadUserNameOrPassword = 0x04,
        RefusedNotAuthorized = 0x05
    };

    namespace detail
    {
        /**
         * @brief Empty properties type for MQTT 3.1.1 CONNACK packets.
         */
        struct EmptyConnAckProperties
        {
        };

        /**
         * @brief Traits for MQTT CONNACK packets.
         * @tparam V Protocol version.
         */
        template<ProtocolVersion V>
        struct ConnAckTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 CONNACK packets.
         */
        template<>
        struct ConnAckTraits<ProtocolVersion::V311>
        {
            using ReasonCodeType = ConnectReturnCode;
            using PropertiesType = EmptyConnAckProperties;
            static constexpr bool HasProperties = false;
        };

        /**
         * @brief Traits specialization for MQTT 5 CONNACK packets.
         */
        template<>
        struct ConnAckTraits<ProtocolVersion::V5>
        {
            using ReasonCodeType = ReasonCode;
            using PropertiesType = properties::Properties;
            static constexpr bool HasProperties = true;

            /**
             * @brief Check if the reason code is valid for CONNACK.
             * @param reasonCode The reason code to validate.
             * @return true if the reason code is valid for CONNACK.
             */
            static bool isReasonCodeValid(ReasonCode reasonCode);
        };
    } // namespace detail

    /**
     * @brief MQTT CONNACK packet for a specific protocol version.
     *
     * CONNACK is the packet sent by the server in response to a CONNECT packet from a client.
     *
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901074
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718033
     *
     * @tparam TProtocolVersion Protocol version.
     */
    template<ProtocolVersion TProtocolVersion>
    class ConnAck final : public TControlPacket<PacketType::ConnAck>
    {
    public:
        using Traits = detail::ConnAckTraits<TProtocolVersion>;
        using ReasonCodeType = Traits::ReasonCodeType;
        using PropertiesType = typename Traits::PropertiesType;

        /**
         * @brief Constructor with session present flag and reason code.
         * @param bSessionPresent Whether a session is present.
         * @param reasonCode The connect or reason code.
         */
        explicit ConnAck(bool bSessionPresent, ReasonCodeType reasonCode);

        /**
         * @brief Constructor with session present flag, reason code, and properties.
         * @param bSessionPresent Whether a session is present.
         * @param reasonCode The reason code.
         * @param properties The CONNACK properties.
         */
        explicit ConnAck(bool bSessionPresent, ReasonCodeType reasonCode, PropertiesType&& properties)
            requires(detail::ConnAckTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit ConnAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        /**
         * @brief Get the length of the packet payload.
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
         * @brief Get the session present flag.
         * @return The session present flag.
         */
        [[nodiscard]] bool getSessionPresent() const;

        /**
         * @brief Get the connect or reason code.
         * @return The connect or reason code.
         */
        [[nodiscard]] ReasonCodeType getReasonCode() const;

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesType& getProperties() const
            requires(detail::ConnAckTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Check if the reason code is valid for CONNACK (MQTT 5 only).
         * @param reasonCode The reason code to validate.
         * @return true if the reason code is valid for CONNACK.
         */
        static bool isReasonCodeValid(ReasonCodeType reasonCode)
            requires(std::is_same_v<ReasonCodeType, ReasonCode>);

    private:
        [[nodiscard]] static constexpr ReasonCodeType defaultReasonCode();

        void setSessionPresent(bool isSessionPresent);

        bool m_isSessionPresent{ false };
        ReasonCodeType m_reasonCode{ defaultReasonCode() };
        PropertiesType m_properties{};

        static constexpr auto kSessionPresentBit{ std::byte{ 0x1 } };
    };

    /**
     * @brief Alias for MQTT 3.1.1 CONNACK packet.
     */
    using ConnAck3 = ConnAck<ProtocolVersion::V311>;

    /**
     * @brief Alias for MQTT 5 CONNACK packet.
     */
    using ConnAck5 = ConnAck<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets
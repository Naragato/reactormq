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
    class Disconnect;

    namespace detail
    {
        /**
         * @brief Traits for MQTT DISCONNECT packets.
         */
        template<ProtocolVersion V>
        struct DisconnectTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 DISCONNECT packets.
         */
        template<>
        struct DisconnectTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            static constexpr bool HasReasonCode = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 DISCONNECT packets.
         */
        template<>
        struct DisconnectTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            static constexpr bool HasReasonCode = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief MQTT DISCONNECT packet for a specific protocol version.
     *
     * DISCONNECT is sent by either the client or server to indicate that it is about to close the connection.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901205
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718090
     */
    template<ProtocolVersion TProtocolVersion>
    class Disconnect final : public TControlPacket<PacketType::Disconnect>
    {
        using Traits = detail::DisconnectTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Default constructor.
         *
         * For MQTT 3.1.1, creates an empty DISCONNECT with no payload.
         * For MQTT 5, creates a DISCONNECT with NormalDisconnection and no properties.
         */
        explicit Disconnect();

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Disconnect(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        /**
         * @brief Constructor with reason code and properties (MQTT 5 only).
         * @param reasonCode The reason code.
         * @param properties The DISCONNECT properties.
         */
        explicit Disconnect(ReasonCode reasonCode, PropertiesStorage&& properties = PropertiesStorage{})
            requires(detail::DisconnectTraits<TProtocolVersion>::HasProperties == true);

        /**
         * @brief Get the length of the packet payload.
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override;

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
            requires(detail::DisconnectTraits<TProtocolVersion>::HasReasonCode == true);

        /**
         * @brief Get the properties (MQTT 5 only).
         * @return The properties.
         */
        [[nodiscard]] const PropertiesStorage& getProperties() const
            requires(detail::DisconnectTraits<TProtocolVersion>::HasProperties == true);

        /**
         * @brief Check if the reason code is valid for DISCONNECT (MQTT 5 only).
         * @param reasonCode The reason code to validate.
         * @return true if the reason code is valid for DISCONNECT.
         */
        static bool isReasonCodeValid(ReasonCode reasonCode);

    private:
        ReasonCode m_reasonCode{ ReasonCode::Success };
        PropertiesStorage m_properties{};
    };

    /**
     * @brief Encode a DISCONNECT packet to the writer.
     * @tparam V Protocol version.
     * @param writer Writer to encode to.
     */
    template<ProtocolVersion V>
    void encodeDisconnectToWriter(serialize::ByteWriter& writer);

    /**
     * @brief Alias for MQTT 3.1.1 DISCONNECT packet.
     */
    using Disconnect3 = Disconnect<ProtocolVersion::V311>;

    /**
     * @brief Alias for MQTT 5 DISCONNECT packet.
     */
    using Disconnect5 = Disconnect<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets
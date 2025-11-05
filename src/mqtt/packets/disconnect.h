#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"
#include <cstdint>
#include <utility>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT DISCONNECT packets.
     * 
     * DISCONNECT is sent by either the client or server to indicate that it is about to close the connection.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901205
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718090
     */
    class DisconnectBase : public TControlPacket<PacketType::Disconnect>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit DisconnectBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
        {
        }

        /**
         * @brief Default constructor.
         */
        explicit DisconnectBase()
        {
        }
    };

    template<reactormq::mqtt::packets::ProtocolVersion TProtocolVersion>
    class Disconnect;

    /**
     * @brief MQTT 3.1.1 DISCONNECT packet.
     * 
     * In MQTT 3.1.1, DISCONNECT has no payload - it's just a fixed header.
     */
    template<>
    class Disconnect<ProtocolVersion::V311> final : public DisconnectBase
    {
    public:
        /**
         * @brief Default constructor.
         */
        explicit Disconnect()
            : DisconnectBase()
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Disconnect(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : DisconnectBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return 0 (no payload for MQTT 3.1.1 DISCONNECT).
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return 0;
        }

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
    };

    /**
     * @brief MQTT 5 DISCONNECT packet.
     * 
     * In MQTT 5, DISCONNECT can optionally include a reason code and properties.
     * Special optimization: if reason code is Success (0x00) with no properties, the packet has length 0.
     */
    template<>
    class Disconnect<ProtocolVersion::V5> final : public DisconnectBase
    {
    public:
        /**
         * @brief Constructor with reason code and properties.
         * @param reasonCode The reason code (default: Success/NormalDisconnection).
         * @param properties The DISCONNECT properties (default: empty).
         */
        explicit Disconnect(
            const reactormq::mqtt::ReasonCode reasonCode = reactormq::mqtt::ReasonCode::NormalDisconnection,
            properties::Properties properties = properties::Properties{})
            : DisconnectBase()
              , m_reasonCode(reasonCode)
              , m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
            if (!isReasonCodeValid(reasonCode))
            {
                m_isValid = false;
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "Disconnect",
                    "[Disconnect] Invalid reason code: %u",
                    static_cast<uint8_t>(reasonCode));
            }
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Disconnect(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : DisconnectBase(fixedHeader)
              , m_reasonCode(reactormq::mqtt::ReasonCode::Success)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * Special optimization: if reason code is Success (0x00), returns 0.
         * Otherwise returns reason code (1 byte) + properties length.
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override;

        /**
         * @brief Encode the packet to a ByteWriter.
         * Special optimization: if reason code is Success (0x00), encodes nothing (just fixed header).
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * Special case: if remaining length is 0, reason code is Success.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;

        /**
         * @brief Get the reason code.
         * @return The reason code.
         */
        [[nodiscard]] ReasonCode getReasonCode() const
        {
            return m_reasonCode;
        }

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
        }

        /**
         * @brief Check if the reason code is valid for DISCONNECT.
         * @param reasonCode The reason code to validate.
         * @return true if the reason code is valid for DISCONNECT.
         */
        static bool isReasonCodeValid(reactormq::mqtt::ReasonCode reasonCode);

    protected:
        /// @brief The reason code for this DISCONNECT packet.
        reactormq::mqtt::ReasonCode m_reasonCode;

        /// @brief The properties for this DISCONNECT packet.
        properties::Properties m_properties;
    };

    using Disconnect3 = Disconnect<ProtocolVersion::V311>;
    using Disconnect5 = Disconnect<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

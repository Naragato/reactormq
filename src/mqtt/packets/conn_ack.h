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
     * @brief MQTT 3.1.1 Connect Return Code.
     * 
     * These are the return codes used in MQTT 3.1.1 CONNACK packets.
     */
    enum class ConnectReturnCode : uint8_t
    {
        Accepted = 0x00,
        RefusedProtocolVersion = 0x01,
        RefusedIdentifierRejected = 0x02,
        RefusedServerUnavailable = 0x03,
        RefusedBadUserNameOrPassword = 0x04,
        RefusedNotAuthorized = 0x05
    };

    /**
     * @brief Base class for MQTT CONNACK packets.
     * 
     * CONNACK is the packet sent by the server in response to a CONNECT packet from a client.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901074
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718033
     */
    class ConnAckBase : public TControlPacket<PacketType::ConnAck>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit ConnAckBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
              , m_bSessionPresent(false)
        {
        }

        /**
         * @brief Constructor with session present flag.
         * @param bSessionPresent Whether a session is present.
         */
        explicit ConnAckBase(const bool bSessionPresent)
            : m_bSessionPresent(bSessionPresent)
        {
        }

        /**
         * @brief Get the session present flag.
         * @return The session present flag.
         */
        [[nodiscard]] bool getSessionPresent() const
        {
            return m_bSessionPresent;
        }

    protected:
        /// @brief Whether a session is present on the server.
        bool m_bSessionPresent;
    };

    template<ProtocolVersion TProtocolVersion>
    class ConnAck;

    /**
     * @brief MQTT 3.1.1 CONNACK packet.
     * 
     * Contains session present flag (1 byte) and connect return code (1 byte).
     */
    template<>
    class ConnAck<ProtocolVersion::V311> final : public ConnAckBase
    {
    public:
        /**
         * @brief Constructor with session present flag and return code.
         * @param bSessionPresent Whether a session is present.
         * @param returnCode The connect return code.
         */
        explicit ConnAck(const bool bSessionPresent, const ConnectReturnCode returnCode)
            : ConnAckBase(bSessionPresent)
              , m_returnCode(returnCode)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit ConnAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : ConnAckBase(fixedHeader)
              , m_returnCode(ConnectReturnCode::Accepted)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return 2 (1 byte flags + 1 byte return code).
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return 2;
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

        /**
         * @brief Get the connect return code.
         * @return The connect return code.
         */
        [[nodiscard]] ConnectReturnCode getReasonCode() const
        {
            return m_returnCode;
        }

    protected:
        /// @brief The connect return code for this CONNACK packet.
        ConnectReturnCode m_returnCode;
    };

    /**
     * @brief MQTT 5 CONNACK packet.
     * 
     * Contains session present flag (1 byte), reason code (1 byte), and properties.
     */
    template<>
    class ConnAck<ProtocolVersion::V5> final : public ConnAckBase
    {
    public:
        /**
         * @brief Constructor with session present flag, reason code, and properties.
         * @param bSessionPresent Whether a session is present.
         * @param reasonCode The reason code.
         * @param properties The CONNACK properties (default: empty).
         */
        explicit ConnAck(
            const bool bSessionPresent,
            const ReasonCode reasonCode,
            properties::Properties properties = properties::Properties{})
            : ConnAckBase(bSessionPresent)
              , m_reasonCode(reasonCode)
              , m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
            if (!isReasonCodeValid(reasonCode))
            {
                m_isValid = false;
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "ConnAck",
                    "[ConnAck] Invalid reason code: %u",
                    static_cast<uint8_t>(reasonCode));
                m_reasonCode = ReasonCode::UnspecifiedError;
            }
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit ConnAck(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : ConnAckBase(fixedHeader)
              , m_reasonCode(ReasonCode::Success)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload.
         * @return 2 (flags + reason code) + properties length.
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
         * @brief Check if the reason code is valid for CONNACK.
         * @param reasonCode The reason code to validate.
         * @return true if the reason code is valid for CONNACK.
         */
        static bool isReasonCodeValid(ReasonCode reasonCode);

    protected:
        /// @brief The reason code for this CONNACK packet.
        ReasonCode m_reasonCode;

        /// @brief The properties for this CONNACK packet.
        properties::Properties m_properties;
    };

    using ConnAck3 = ConnAck<ProtocolVersion::V311>;
    using ConnAck5 = ConnAck<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

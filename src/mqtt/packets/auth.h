//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief MQTT 5 AUTH packet.
     * AUTH is used for enhanced authentication in MQTT 5.0.
     * It can be sent by either client or server during or after the connection flow.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901217
     * Note: AUTH is MQTT 5 only - there is no MQTT 3.1.1 equivalent.
     */
    class Auth final : public TControlPacket<PacketType::Auth>
    {
    public:
        /**
         * @brief Constructor with reason code and properties.
         * @param reasonCode The reason code (required, no default).
         * @param properties The AUTH properties (default: empty).
         */
        explicit Auth(const ReasonCode reasonCode, const properties::Properties& properties = properties::Properties{})
            : m_reasonCode(reasonCode)
            , m_properties(properties)
        {
            if (!isReasonCodeValid(reasonCode))
            {
                setIsValid(false);
                REACTORMQ_LOG(logging::LogLevel::Error, "[Auth] Invalid reason code: %u", reasonCode);
            }

            if (!arePropertiesValid(properties))
            {
                setIsValid(false);
                REACTORMQ_LOG(logging::LogLevel::Error, "[Auth] Invalid properties");
            }

            setFixedHeader(FixedHeader::create(this));
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Auth(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
            , m_reasonCode(ReasonCode::Success)
        {
            setIsValid(decode(reader));
        }

        /**
         * @brief Get the length of the packet payload.
         * Special optimization: if reason code is Success (0x00), returns 0.
         * Otherwise returns reason code (1 byte) + properties length.
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            if (m_reasonCode != ReasonCode::Success)
            {
                return sizeof(uint8_t) + m_properties.getLength();
            }

            return 0;
        }

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
         * @brief Check if the reason code is valid for AUTH.
         * Only Success, ContinueAuthentication, and ReAuthenticate are valid.
         * @param reasonCode The reason code to validate.
         * @return true if the reason code is valid for AUTH.
         */
        static bool isReasonCodeValid(ReasonCode reasonCode);

        /**
         * @brief Check if the properties are valid for AUTH.
         * Only AuthenticationMethod, AuthenticationData, ReasonString, and UserProperty are allowed.
         * @param properties The properties to validate.
         * @return true if all properties are valid for AUTH.
         */
        static bool arePropertiesValid(const properties::Properties& properties);

    private:
        ReasonCode m_reasonCode;
        properties::Properties m_properties;
    };
} // namespace reactormq::mqtt::packets
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "auth.h"

#include "mqtt/packets/properties/property_identifier.h"

namespace reactormq::mqtt::packets
{
    void Auth::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Auth] MQTT 5");
        getFixedHeader().encode(writer);

        if (m_reasonCode != ReasonCode::Success)
        {
            writer.writeUint8(static_cast<uint8_t>(m_reasonCode));
            m_properties.encode(writer);
        }
    }

    bool Auth::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Auth] MQTT 5");
        if (getFixedHeader().getPacketType() != PacketType::Auth)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Auth] Invalid packet type: expected Auth, got %d", getFixedHeader().getPacketType());
            m_reasonCode = ReasonCode::UnspecifiedError;
            return false;
        }

        if (getFixedHeader().getRemainingLength() == 0)
        {
            m_reasonCode = ReasonCode::Success;
            return true;
        }

        uint8_t reasonCodeByte = reader.readUint8();
        m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

        if (!isReasonCodeValid(m_reasonCode))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Auth] Invalid reason code: %u", m_reasonCode);
            return false;
        }

        m_properties.decode(reader);

        if (!arePropertiesValid(m_properties))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Auth] Invalid properties");
            return false;
        }

        return true;
    }

    bool Auth::isReasonCodeValid(const ReasonCode reasonCode)
    {
        switch (reasonCode)
        {
            using enum ReasonCode;
        case Success:
        case ContinueAuthentication:
        case ReAuthenticate:
            return true;
        default:
            return false;
        }
    }

    bool Auth::arePropertiesValid(const properties::Properties& properties)
    {
        for (const auto props = properties.getProperties(); const auto& prop : props)
        {
            switch (const auto id = prop.getIdentifier())
            {
                using enum properties::PropertyIdentifier;
            case AuthenticationMethod:
            case AuthenticationData:
            case ReasonString:
            case UserProperty:
                break;
            default:
                REACTORMQ_LOG(logging::LogLevel::Error, "[Auth] Invalid property identifier: %u", id);
                return false;
            }
        }

        return true;
    }
} // namespace reactormq::mqtt::packets
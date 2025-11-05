#include "conn_ack.h"

namespace reactormq::mqtt::packets
{
    void ConnAck<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][ConnAck] MQTT 3.1.1");
        m_fixedHeader.encode(writer);

        std::byte flags{};
        if (m_bSessionPresent)
        {
            flags |= kSessionPresentBit;
        }
        writer.writeUint8(std::to_integer<uint8_t>(flags));

        writer.writeUint8(static_cast<uint8_t>(m_returnCode));
    }

    bool ConnAck<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        using enum ConnectReturnCode;
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][ConnAck] MQTT 3.1.1");
        if (m_fixedHeader.getPacketType() != PacketType::ConnAck)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "ConnAck", "[ConnAck] Invalid packet type: expected ConnAck, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            m_returnCode = RefusedProtocolVersion;
            return false;
        }

        const auto flags = static_cast<std::byte>(reader.readUint8());
        m_bSessionPresent = (flags & kSessionPresentBit) != std::byte{};

        uint8_t returnCodeByte = reader.readUint8();
        m_returnCode = static_cast<ConnectReturnCode>(returnCodeByte);

        switch (m_returnCode)
        {
        case Accepted:
        case RefusedBadUserNameOrPassword:
        case RefusedIdentifierRejected:
        case RefusedServerUnavailable:
        case RefusedProtocolVersion:
        case RefusedNotAuthorized:
            break;
        default: REACTORMQ_LOG(logging::LogLevel::Error, "ConnAck", "[ConnAck] Invalid return code: %u", static_cast<uint8_t>(m_returnCode))            ;
            return false;
        }

        return true;
    }

    uint32_t ConnAck<ProtocolVersion::V5>::getLength() const
    {
        uint32_t payloadLength = 2;
        payloadLength += m_properties.getLength();
        return payloadLength;
    }

    void ConnAck<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][ConnAck] MQTT 5");
        m_fixedHeader.encode(writer);

        std::byte flags{};
        if (m_bSessionPresent)
        {
            flags |= kSessionPresentBit;
        }
        writer.writeUint8(std::to_integer<uint8_t>(flags));
        writer.writeUint8(static_cast<uint8_t>(m_reasonCode));
        m_properties.encode(writer);
    }

    bool ConnAck<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][ConnAck] MQTT 5");
        if (m_fixedHeader.getPacketType() != PacketType::ConnAck)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "ConnAck", "[ConnAck] Invalid packet type: expected ConnAck, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            m_reasonCode = ReasonCode::UnspecifiedError;
            return false;
        }

        const auto flags = static_cast<std::byte>(reader.readUint8());
        m_bSessionPresent = (flags & kSessionPresentBit) != std::byte{};

        uint8_t reasonCodeByte = reader.readUint8();
        m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

        if (!isReasonCodeValid(m_reasonCode))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "ConnAck", "[ConnAck] Invalid reason code: %u", static_cast<uint8_t>(m_reasonCode));
            m_reasonCode = ReasonCode::UnspecifiedError;
            return false;
        }

        m_properties.decode(reader);

        return true;
    }

    bool ConnAck<ProtocolVersion::V5>::isReasonCodeValid(const ReasonCode reasonCode)
    {
        switch (reasonCode)
        {
            using enum ReasonCode;
        case Success:
        case UnspecifiedError:
        case MalformedPacket:
        case ProtocolError:
        case ImplementationSpecificError:
        case UnsupportedProtocolVersion:
        case ClientIdentifierNotValid:
        case BadUsernameOrPassword:
        case NotAuthorized:
        case ServerUnavailable:
        case ServerBusy:
        case Banned:
        case BadAuthenticationMethod:
        case TopicNameInvalid:
        case PacketTooLarge:
        case QuotaExceeded:
        case PayloadFormatInvalid:
        case RetainNotSupported:
        case QoSNotSupported:
        case UseAnotherServer:
        case ServerMoved:
        case ConnectionRateExceeded:
            return true;
        default:
            return false;
        }
    }
} // namespace reactormq::mqtt::packets
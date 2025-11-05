#include "conn_ack.h"

namespace reactormq::mqtt::packets
{
    void ConnAck<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][ConnAck] MQTT 3.1.1");
        m_fixedHeader.encode(writer);

        const uint8_t flags = m_bSessionPresent ? 0x01 : 0x00;
        writer.writeUint8(flags);

        writer.writeUint8(static_cast<uint8_t>(m_returnCode));
    }

    bool ConnAck<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][ConnAck] MQTT 3.1.1");
        if (m_fixedHeader.getPacketType() != PacketType::ConnAck)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "ConnAck",
                "[ConnAck] Invalid packet type: expected ConnAck, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            m_returnCode = ConnectReturnCode::RefusedProtocolVersion;
            return false;
        }

        const uint8_t flags = reader.readUint8();
        m_bSessionPresent = (flags & 0x01) == 0x01;

        uint8_t returnCodeByte = reader.readUint8();
        m_returnCode = static_cast<ConnectReturnCode>(returnCodeByte);

        switch (m_returnCode)
        {
        case ConnectReturnCode::Accepted:
        case ConnectReturnCode::RefusedBadUserNameOrPassword:
        case ConnectReturnCode::RefusedIdentifierRejected:
        case ConnectReturnCode::RefusedServerUnavailable:
        case ConnectReturnCode::RefusedProtocolVersion:
        case ConnectReturnCode::RefusedNotAuthorized:
            break;
        default:
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "ConnAck",
                "[ConnAck] Invalid return code: %u",
                static_cast<uint8_t>(m_returnCode));
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

        const uint8_t flags = m_bSessionPresent ? 0x01 : 0x00;
        writer.writeUint8(flags);
        writer.writeUint8(static_cast<uint8_t>(m_reasonCode));
        m_properties.encode(writer);
    }

    bool ConnAck<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][ConnAck] MQTT 5");
        if (m_fixedHeader.getPacketType() != PacketType::ConnAck)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "ConnAck",
                "[ConnAck] Invalid packet type: expected ConnAck, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            m_reasonCode = ReasonCode::UnspecifiedError;
            return false;
        }

        const uint8_t flags = reader.readUint8();
        m_bSessionPresent = (flags & 0x01) == 0x01;

        uint8_t reasonCodeByte = reader.readUint8();
        m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

        if (!isReasonCodeValid(m_reasonCode))
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "ConnAck",
                "[ConnAck] Invalid reason code: %u",
                static_cast<uint8_t>(m_reasonCode));
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
        case ReasonCode::Success:
        case ReasonCode::UnspecifiedError:
        case ReasonCode::MalformedPacket:
        case ReasonCode::ProtocolError:
        case ReasonCode::ImplementationSpecificError:
        case ReasonCode::UnsupportedProtocolVersion:
        case ReasonCode::ClientIdentifierNotValid:
        case ReasonCode::BadUsernameOrPassword:
        case ReasonCode::NotAuthorized:
        case ReasonCode::ServerUnavailable:
        case ReasonCode::ServerBusy:
        case ReasonCode::Banned:
        case ReasonCode::BadAuthenticationMethod:
        case ReasonCode::TopicNameInvalid:
        case ReasonCode::PacketTooLarge:
        case ReasonCode::QuotaExceeded:
        case ReasonCode::PayloadFormatInvalid:
        case ReasonCode::RetainNotSupported:
        case ReasonCode::QoSNotSupported:
        case ReasonCode::UseAnotherServer:
        case ReasonCode::ServerMoved:
        case ReasonCode::ConnectionRateExceeded:
            return true;
        default:
            return false;
        }
    }
} // namespace reactormq::mqtt::packets
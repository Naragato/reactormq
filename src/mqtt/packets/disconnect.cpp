#include "disconnect.h"

namespace reactormq::mqtt::packets
{
    void Disconnect<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Disconnect] MQTT 3.1.1");
        m_fixedHeader.encode(writer);
    }

    bool Disconnect<ProtocolVersion::V311>::decode(serialize::ByteReader& /*reader*/)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Disconnect] MQTT 3.1.1");
        if (m_fixedHeader.getPacketType() != PacketType::Disconnect)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "Disconnect",
                "[Disconnect] Invalid packet type: expected Disconnect, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        return true;
    }

    uint32_t Disconnect<ProtocolVersion::V5>::getLength() const
    {
        if (m_reasonCode != ReasonCode::Success)
        {
            return sizeof(uint8_t) + m_properties.getLength();
        }

        return 0;
    }

    void Disconnect<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Disconnect] MQTT 5");
        m_fixedHeader.encode(writer);

        if (m_reasonCode != ReasonCode::Success)
        {
            writer.writeUint8(static_cast<uint8_t>(m_reasonCode));
            m_properties.encode(writer);
        }
    }

    bool Disconnect<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Disconnect] MQTT 5");
        if (m_fixedHeader.getPacketType() != PacketType::Disconnect)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Disconnect] Invalid packet type: expected Disconnect, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            m_reasonCode = ReasonCode::UnspecifiedError;
            return false;
        }

        if (m_fixedHeader.getRemainingLength() == 0)
        {
            m_reasonCode = ReasonCode::Success;
            return true;
        }

        uint8_t reasonCodeByte = reader.readUint8();
        m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

        if (!isReasonCodeValid(m_reasonCode))
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Disconnect] Invalid reason code: %u",
                static_cast<uint8_t>(m_reasonCode));
            return false;
        }

        if (m_fixedHeader.getRemainingLength() >= 2)
        {
            m_properties.decode(reader);
        }

        return true;
    }

    bool Disconnect<ProtocolVersion::V5>::isReasonCodeValid(const ReasonCode reasonCode)
    {
        switch (reasonCode)
        {
        case ReasonCode::NormalDisconnection:
        case ReasonCode::DisconnectWithWillMessage:
        case ReasonCode::UnspecifiedError:
        case ReasonCode::MalformedPacket:
        case ReasonCode::ProtocolError:
        case ReasonCode::ImplementationSpecificError:
        case ReasonCode::NotAuthorized:
        case ReasonCode::ServerBusy:
        case ReasonCode::ServerShuttingDown:
        case ReasonCode::KeepAliveTimeout:
        case ReasonCode::SessionTakenOver:
        case ReasonCode::TopicFilterInvalid:
        case ReasonCode::TopicNameInvalid:
        case ReasonCode::ReceiveMaximumExceeded:
        case ReasonCode::TopicAliasInvalid:
        case ReasonCode::PacketTooLarge:
        case ReasonCode::MessageRateTooHigh:
        case ReasonCode::QuotaExceeded:
        case ReasonCode::AdministrativeAction:
        case ReasonCode::PayloadFormatInvalid:
        case ReasonCode::RetainNotSupported:
        case ReasonCode::QoSNotSupported:
        case ReasonCode::UseAnotherServer:
        case ReasonCode::ServerMoved:
        case ReasonCode::SharedSubscriptionsNotSupported:
        case ReasonCode::ConnectionRateExceeded:
        case ReasonCode::MaximumConnectTime:
        case ReasonCode::SubscriptionIdentifiersNotSupported:
        case ReasonCode::WildcardSubscriptionsNotSupported:
            return true;
        default:
            return false;
        }
    }
} // namespace reactormq::mqtt::packets
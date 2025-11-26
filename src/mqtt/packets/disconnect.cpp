//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "disconnect.h"

namespace reactormq::mqtt::packets
{
    using serialize::ByteReader;
    using serialize::ByteWriter;

    template<ProtocolVersion TProtocolVersion>
    Disconnect<TProtocolVersion>::Disconnect()
        : TControlPacket()
    {
        if constexpr (detail::DisconnectTraits<TProtocolVersion>::HasReasonCode)
        {
            m_reasonCode = ReasonCode::NormalDisconnection;
        }

        this->setFixedHeader(FixedHeader::create(this));

        if constexpr (detail::DisconnectTraits<TProtocolVersion>::HasReasonCode)
        {
            if (!isReasonCodeValid(m_reasonCode))
            {
                this->setIsValid(false);
                REACTORMQ_LOG(logging::LogLevel::Error, "[Disconnect] Invalid reason code: %u", m_reasonCode);
            }
        }
    }

    template<ProtocolVersion TProtocolVersion>
    Disconnect<TProtocolVersion>::Disconnect(ByteReader& reader, const FixedHeader& fixedHeader)
        : TControlPacket(fixedHeader)
    {
        if constexpr (detail::DisconnectTraits<TProtocolVersion>::HasReasonCode)
        {
            m_reasonCode = ReasonCode::Success;
        }

        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    Disconnect<TProtocolVersion>::Disconnect(
        const ReasonCode reasonCode, typename detail::DisconnectTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::DisconnectTraits<TProtocolVersion>::HasProperties == true)

        : TControlPacket()
        , m_reasonCode(reasonCode)
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));

        if (!isReasonCodeValid(m_reasonCode))
        {
            this->setIsValid(false);
            REACTORMQ_LOG(logging::LogLevel::Error, "[Disconnect] Invalid reason code: %u", m_reasonCode);
        }
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Disconnect<TProtocolVersion>::getLength() const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            if (m_reasonCode != ReasonCode::Success)
            {
                return sizeof(uint8_t) + m_properties.getLength();
            }

            return 0;
        }
        else
        {
            return 0;
        }
    }

    template<ProtocolVersion TProtocolVersion>
    void Disconnect<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Disconnect] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Disconnect] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);

        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            if (m_reasonCode != ReasonCode::Success)
            {
                writer.writeUint8(static_cast<uint8_t>(m_reasonCode));
                m_properties.encode(writer);
            }
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool Disconnect<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Disconnect] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Disconnect] MQTT 3.1.1");
        }

        if (this->getFixedHeader().getPacketType() != PacketType::Disconnect)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Disconnect] Invalid packet type: expected Disconnect, got %d",
                static_cast<int>(this->getFixedHeader().getPacketType()));

            if constexpr (detail::DisconnectTraits<TProtocolVersion>::HasReasonCode)
            {
                m_reasonCode = ReasonCode::UnspecifiedError;
            }

            return false;
        }

        if constexpr (TProtocolVersion == ProtocolVersion::V311)
        {
            return true;
        }
        else
        {
            if (this->getFixedHeader().getRemainingLength() == 0)
            {
                m_reasonCode = ReasonCode::Success;
                return true;
            }

            const uint8_t reasonCodeByte = reader.readUint8();
            m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

            if (!isReasonCodeValid(m_reasonCode))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Disconnect] Invalid reason code: %u", m_reasonCode);
                return false;
            }

            if (this->getFixedHeader().getRemainingLength() >= 2)
            {
                m_properties.decode(reader);
            }

            return true;
        }
    }

    template<ProtocolVersion TProtocolVersion>
    ReasonCode Disconnect<TProtocolVersion>::getReasonCode() const
        requires(detail::DisconnectTraits<TProtocolVersion>::HasReasonCode == true)
    {
        return m_reasonCode;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::DisconnectTraits<TProtocolVersion>::PropertiesType& Disconnect<TProtocolVersion>::getProperties() const
        requires(detail::DisconnectTraits<TProtocolVersion>::HasProperties == true)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    bool Disconnect<TProtocolVersion>::isReasonCodeValid(const ReasonCode reasonCode)
    {
        switch (reasonCode)
        {
            using enum ReasonCode;
        case NormalDisconnection:
        case DisconnectWithWillMessage:
        case UnspecifiedError:
        case MalformedPacket:
        case ProtocolError:
        case ImplementationSpecificError:
        case NotAuthorized:
        case ServerBusy:
        case ServerShuttingDown:
        case KeepAliveTimeout:
        case SessionTakenOver:
        case TopicFilterInvalid:
        case TopicNameInvalid:
        case ReceiveMaximumExceeded:
        case TopicAliasInvalid:
        case PacketTooLarge:
        case MessageRateTooHigh:
        case QuotaExceeded:
        case AdministrativeAction:
        case PayloadFormatInvalid:
        case RetainNotSupported:
        case QoSNotSupported:
        case UseAnotherServer:
        case ServerMoved:
        case SharedSubscriptionsNotSupported:
        case ConnectionRateExceeded:
        case MaximumConnectTime:
        case SubscriptionIdentifiersNotSupported:
        case WildcardSubscriptionsNotSupported:
            return true;
        default:
            return false;
        }
    }

    template<ProtocolVersion V>
    void encodeDisconnectToWriter(ByteWriter& writer)
    {
        if constexpr (V == ProtocolVersion::V5)
        {
            Disconnect<V> disconnectPacket(ReasonCode::NormalDisconnection, typename detail::DisconnectTraits<V>::PropertiesType{});
            disconnectPacket.encode(writer);
        }
        else
        {
            Disconnect<V> disconnectPacket;
            disconnectPacket.encode(writer);
        }
    }

    template class Disconnect<ProtocolVersion::V311>;
    template class Disconnect<ProtocolVersion::V5>;

    template void encodeDisconnectToWriter<ProtocolVersion::V311>(ByteWriter&);

    template void encodeDisconnectToWriter<ProtocolVersion::V5>(ByteWriter&);
} // namespace reactormq::mqtt::packets
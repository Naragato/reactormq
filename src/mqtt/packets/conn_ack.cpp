//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "conn_ack.h"

#include "util/logging/registry.h"

#include <utility>

namespace reactormq::mqtt::packets
{
    using serialize::ByteReader;
    using serialize::ByteWriter;

    bool detail::ConnAckTraits<ProtocolVersion::V5>::isReasonCodeValid(const ReasonCode reasonCode)
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

    template<ProtocolVersion TProtocolVersion>
    constexpr ConnAck<TProtocolVersion>::ReasonCodeType ConnAck<TProtocolVersion>::defaultReasonCode()
    {
        if constexpr (std::is_same_v<ReasonCodeType, ConnectReturnCode>)
        {
            return ConnectReturnCode::Accepted;
        }
        else
        {
            return ReasonCode::Success;
        }
    }

    template<ProtocolVersion TProtocolVersion>
    ConnAck<TProtocolVersion>::ConnAck(
        const bool bSessionPresent, typename detail::ConnAckTraits<TProtocolVersion>::ReasonCodeType reasonCode)
        : m_isSessionPresent(bSessionPresent)
        , m_reasonCode(reasonCode)
    {
        this->setFixedHeader(FixedHeader::create(this));

        if constexpr (std::is_same_v<ReasonCodeType, ReasonCode>)
        {
            if (!isReasonCodeValid(m_reasonCode))
            {
                this->setIsValid(false);
                REACTORMQ_LOG(logging::LogLevel::Error, "[ConnAck] Invalid reason code: %u", static_cast<std::uint8_t>(m_reasonCode));
                m_reasonCode = ReasonCode::UnspecifiedError;
            }
        }
    }

    template<ProtocolVersion TProtocolVersion>
    ConnAck<TProtocolVersion>::ConnAck(
        const bool bSessionPresent,
        typename detail::ConnAckTraits<TProtocolVersion>::ReasonCodeType reasonCode,
        typename detail::ConnAckTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::ConnAckTraits<TProtocolVersion>::HasProperties)
        : m_isSessionPresent(bSessionPresent)
        , m_reasonCode(reasonCode)
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));

        if (!isReasonCodeValid(m_reasonCode))
        {
            this->setIsValid(false);
            REACTORMQ_LOG(logging::LogLevel::Error, "[ConnAck] Invalid reason code: %u", static_cast<std::uint8_t>(m_reasonCode));
            m_reasonCode = ReasonCode::UnspecifiedError;
        }
    }

    template<ProtocolVersion TProtocolVersion>
    ConnAck<TProtocolVersion>::ConnAck(ByteReader& reader, const FixedHeader& fixedHeader)
        : TControlPacket(fixedHeader)
        , m_reasonCode(defaultReasonCode())
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    std::uint32_t ConnAck<TProtocolVersion>::getLength() const
    {
        std::uint32_t payloadLength = 2;

        if constexpr (Traits::HasProperties)
        {
            payloadLength += m_properties.getLength();
        }

        return payloadLength;
    }

    template<ProtocolVersion TProtocolVersion>
    void ConnAck<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (std::is_same_v<ReasonCodeType, ConnectReturnCode>)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][ConnAck] MQTT 3.1.1");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][ConnAck] MQTT 5");
        }

        this->getFixedHeader().encode(writer);

        std::byte flags{};
        if (m_isSessionPresent)
        {
            flags |= kSessionPresentBit;
        }

        writer.writeUint8(std::to_integer<std::uint8_t>(flags));
        writer.writeUint8(static_cast<std::uint8_t>(m_reasonCode));

        if constexpr (Traits::HasProperties)
        {
            m_properties.encode(writer);
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool ConnAck<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (std::is_same_v<ReasonCodeType, ConnectReturnCode>)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][ConnAck] MQTT 3.1.1");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][ConnAck] MQTT 5");
        }

        if (this->getFixedHeader().getPacketType() != PacketType::ConnAck)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[ConnAck] Invalid packet type: expected ConnAck, got %d",
                static_cast<int>(this->getFixedHeader().getPacketType()));

            if constexpr (std::is_same_v<ReasonCodeType, ConnectReturnCode>)
            {
                m_reasonCode = ConnectReturnCode::RefusedProtocolVersion;
            }
            else
            {
                m_reasonCode = ReasonCode::UnspecifiedError;
            }

            return false;
        }

        const auto flags = static_cast<std::byte>(reader.readUint8());
        setSessionPresent((flags & kSessionPresentBit) != std::byte{});

        const std::uint8_t reasonCodeByte = reader.readUint8();
        m_reasonCode = static_cast<ReasonCodeType>(reasonCodeByte);

        if constexpr (std::is_same_v<ReasonCodeType, ConnectReturnCode>)
        {
            using enum ConnectReturnCode;

            switch (m_reasonCode)
            {
            case Accepted:
            case RefusedBadUserNameOrPassword:
            case RefusedIdentifierRejected:
            case RefusedServerUnavailable:
            case RefusedProtocolVersion:
            case RefusedNotAuthorized:
                break;
            default:
                REACTORMQ_LOG(logging::LogLevel::Error, "[ConnAck] Invalid return code: %u", static_cast<std::uint8_t>(m_reasonCode));
                return false;
            }
        }
        else
        {
            if (!isReasonCodeValid(m_reasonCode))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[ConnAck] Invalid reason code: %u", static_cast<std::uint8_t>(m_reasonCode));
                m_reasonCode = ReasonCode::UnspecifiedError;
                return false;
            }
        }

        if constexpr (Traits::HasProperties)
        {
            m_properties.decode(reader);
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    bool ConnAck<TProtocolVersion>::isReasonCodeValid(typename detail::ConnAckTraits<TProtocolVersion>::ReasonCodeType reasonCode)
        requires(std::is_same_v<ReasonCodeType, ReasonCode>)
    {
        return Traits::isReasonCodeValid(reasonCode);
    }

    template<ProtocolVersion TProtocolVersion>
    bool ConnAck<TProtocolVersion>::getSessionPresent() const
    {
        return m_isSessionPresent;
    }

    template<ProtocolVersion TProtocolVersion>
    ConnAck<TProtocolVersion>::ReasonCodeType ConnAck<TProtocolVersion>::getReasonCode() const
    {
        return m_reasonCode;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::ConnAckTraits<TProtocolVersion>::PropertiesType& ConnAck<TProtocolVersion>::getProperties() const
        requires(detail::ConnAckTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    void ConnAck<TProtocolVersion>::setSessionPresent(const bool isSessionPresent)
    {
        m_isSessionPresent = isSessionPresent;
    }

    template class ConnAck<ProtocolVersion::V311>;
    template class ConnAck<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets
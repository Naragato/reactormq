//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "pub_ack.h"

#include "util/logging/logging.h"

namespace reactormq::mqtt::packets
{
    using serialize::ByteReader;
    using serialize::ByteWriter;

    template<ProtocolVersion TProtocolVersion>
    PubAck<TProtocolVersion>::PubAck(const std::uint16_t packetIdentifier)
        : TControlPacket(FixedHeader{})
        , m_packetIdentifier(packetIdentifier)
        , m_properties()
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    PubAck<TProtocolVersion>::PubAck(
        const std::uint16_t packetIdentifier,
        const ReasonCode reasonCode,
        typename detail::PubAckTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::PubAckTraits<TProtocolVersion>::HasProperties)
        : TControlPacket(FixedHeader{})
        , m_packetIdentifier(packetIdentifier)
        , m_reasonCode(reasonCode)
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    PubAck<TProtocolVersion>::PubAck(ByteReader& reader, const FixedHeader& fixedHeader)
        : TControlPacket(fixedHeader)
        , m_properties()
    {
        decode(reader);
    }

    template<ProtocolVersion TProtocolVersion>
    std::uint16_t PubAck<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    std::uint32_t PubAck<TProtocolVersion>::getLength() const
    {
        std::uint32_t length = 2;

        if constexpr (detail::PubAckTraits<TProtocolVersion>::HasProperties)
        {
            const std::uint32_t propsLength = m_properties.getLength(false);

            if (m_reasonCode == ReasonCode::Success && propsLength == 0)
            {
                return length;
            }

            length += 1;
            length += serialize::variableByteIntegerSize(propsLength) + propsLength;
        }

        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void PubAck<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PubAck] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PubAck] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if constexpr (detail::PubAckTraits<TProtocolVersion>::HasProperties)
        {
            if (const std::uint32_t propsLength = m_properties.getLength(false); m_reasonCode == ReasonCode::Success && propsLength == 0)
            {
                return;
            }

            writer.writeUint8(static_cast<std::uint8_t>(m_reasonCode));
            m_properties.encode(writer);
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool PubAck<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PubAck] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PubAck] MQTT 3.1.1");
        }

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            this->setIsValid(false);
            return false;
        }

        if constexpr (detail::PubAckTraits<TProtocolVersion>::HasProperties)
        {
            if (reader.getRemaining() == 0)
            {
                m_reasonCode = ReasonCode::Success;
                return true;
            }

            std::uint8_t reasonCodeByte = 0;
            if (!reader.tryReadUint8(reasonCodeByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read reason code");
                this->setIsValid(false);
                return false;
            }

            m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

            if (reader.getRemaining() > 0)
            {
                m_properties.decode(reader);
            }
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    ReasonCode PubAck<TProtocolVersion>::getReasonCode() const
        requires(detail::PubAckTraits<TProtocolVersion>::HasProperties)
    {
        return m_reasonCode;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::PubAckTraits<TProtocolVersion>::PropertiesType& PubAck<TProtocolVersion>::getProperties() const
        requires(detail::PubAckTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template class PubAck<ProtocolVersion::V311>;
    template class PubAck<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodePubAckToWriter(
        ByteWriter& writer, uint16_t packetId, ReasonCode reasonCode, typename detail::PubAckTraits<V>::PropertiesType&& properties)
    {
        using Traits = detail::PubAckTraits<V>;
        if constexpr (Traits::HasProperties)
        {
            PubAck<V> packet(packetId, reasonCode, std::move(properties));
            packet.encode(writer);
        }
        else
        {
            PubAck<V> packet(packetId);
            packet.encode(writer);
        }
    }

    template void encodePubAckToWriter<ProtocolVersion::V311>(
        ByteWriter&, uint16_t, ReasonCode, typename detail::PubAckTraits<ProtocolVersion::V311>::PropertiesType&&);
    template void encodePubAckToWriter<ProtocolVersion::V5>(
        ByteWriter&, uint16_t, ReasonCode, typename detail::PubAckTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets
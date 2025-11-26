//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "unsub_ack.h"

#include "util/logging/logging.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
// Some clang versions also emit this variant:
#pragma clang diagnostic ignored "-Wunreachable-code-break"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunreachable-code"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4702)
#endif // defined(__clang__)

namespace reactormq::mqtt::packets
{
    using serialize::ByteReader;
    using serialize::ByteWriter;

    template<ProtocolVersion TProtocolVersion>
    UnsubAck<TProtocolVersion>::UnsubAck(const uint16_t packetId)
        : m_packetIdentifier(packetId)
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    UnsubAck<TProtocolVersion>::UnsubAck(
        const uint16_t packetId,
        std::vector<ReasonCode> reasonCodes,
        typename detail::UnsubAckTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::UnsubAckTraits<TProtocolVersion>::HasReasonCodes)
        : m_packetIdentifier(packetId)
        , m_reasonCodes(std::move(reasonCodes))
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    UnsubAck<TProtocolVersion>::UnsubAck(ByteReader& reader, const FixedHeader& fixedHeader)
        : IUnsubAckPacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t UnsubAck<TProtocolVersion>::getLength() const
    {
        uint32_t length = 2; // Packet ID

        if constexpr (Traits::HasProperties)
        {
            const uint32_t propsLen = getPropertiesLength();
            length += serialize::variableByteIntegerSize(propsLen) + propsLen;
        }

        if constexpr (Traits::HasReasonCodes)
        {
            length += static_cast<uint32_t>(m_reasonCodes.size());
        }
        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void UnsubAck<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][UnsubAck] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][UnsubAck] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if constexpr (Traits::HasProperties)
        {
            m_properties.encode(writer);
        }

        if constexpr (Traits::HasReasonCodes)
        {
            for (const auto& code : m_reasonCodes)
            {
                writer.writeUint8(static_cast<uint8_t>(code));
            }
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool UnsubAck<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][UnsubAck] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][UnsubAck] MQTT 3.1.1");
        }

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            return false;
        }

        uint32_t remainingLength = this->getFixedHeader().getRemainingLength() - 2;

        if constexpr (Traits::HasProperties)
        {
            m_properties.decode(reader);
            const uint32_t propsFullLen = m_properties.getLength(true);
            if (remainingLength < propsFullLen)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Invalid remaining length");
                return false;
            }
            remainingLength -= propsFullLen;
        }

        if constexpr (Traits::HasReasonCodes)
        {
            while (remainingLength > 0)
            {
                uint8_t codeByte = 0;
                if (!reader.tryReadUint8(codeByte))
                {
                    REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read reason code");
                    return false;
                }
                m_reasonCodes.push_back(static_cast<ReasonCode>(codeByte));
                --remainingLength;
            }
        }
        else
        {
            if (remainingLength > 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "Extra data in V3.1.1 UnsubAck");
            }
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t UnsubAck<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::vector<ReasonCode>& UnsubAck<TProtocolVersion>::getReasonCodes() const
        requires(detail::UnsubAckTraits<TProtocolVersion>::HasReasonCodes)
    {
        return m_reasonCodes;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::UnsubAckTraits<TProtocolVersion>::PropertiesType& UnsubAck<TProtocolVersion>::getProperties() const
        requires(detail::UnsubAckTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t UnsubAck<TProtocolVersion>::getPropertiesLength() const
    {
        if constexpr (Traits::HasProperties)
        {
            return m_properties.getLength(false);
        }
        return 0;
    }

    template class UnsubAck<ProtocolVersion::V311>;
    template class UnsubAck<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeUnsubAckToWriter(
        ByteWriter& writer,
        uint16_t packetId,
        const std::vector<ReasonCode>& reasonCodes,
        typename detail::UnsubAckTraits<V>::PropertiesType&& properties)
    {
        using Traits = detail::UnsubAckTraits<V>;
        if constexpr (Traits::HasReasonCodes)
        {
            UnsubAck<V> packet(packetId, reasonCodes, std::move(properties));
            packet.encode(writer);
        }
        else
        {
            UnsubAck<V> packet(packetId);
            packet.encode(writer);
        }
    }

    template void encodeUnsubAckToWriter<ProtocolVersion::V311>(
        ByteWriter&, uint16_t, const std::vector<ReasonCode>&, typename detail::UnsubAckTraits<ProtocolVersion::V311>::PropertiesType&&);
    template void encodeUnsubAckToWriter<ProtocolVersion::V5>(
        ByteWriter&, uint16_t, const std::vector<ReasonCode>&, typename detail::UnsubAckTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "pub_rel.h"

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
    PubRel<TProtocolVersion>::PubRel(const uint16_t packetId)
        : m_packetIdentifier(packetId)
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    PubRel<TProtocolVersion>::PubRel(
        const uint16_t packetId, const ReasonCode reasonCode, typename detail::PubRelTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::PubRelTraits<TProtocolVersion>::HasReasonCode)
        : m_packetIdentifier(packetId)
        , m_reasonCode(reasonCode)
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    PubRel<TProtocolVersion>::PubRel(ByteReader& reader, const FixedHeader& fixedHeader)
        : IPubRelPacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t PubRel<TProtocolVersion>::getLength() const
    {
        uint32_t length = 2; // Packet ID

        if constexpr (Traits::HasReasonCode)
        {
            if (m_reasonCode == ReasonCode::Success && getPropertiesLength() == 0)
            {
                return length;
            }
            length += 1; // Reason Code
        }

        if constexpr (Traits::HasProperties)
        {
            if (m_reasonCode != ReasonCode::Success || getPropertiesLength() > 0)
            {
                const uint32_t propsLen = getPropertiesLength();
                length += serialize::variableByteIntegerSize(propsLen) + propsLen;
            }
        }
        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void PubRel<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PubRel] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PubRel] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if constexpr (Traits::HasReasonCode)
        {
            if (m_reasonCode == ReasonCode::Success && getPropertiesLength() == 0)
            {
                return;
            }
            writer.writeUint8(static_cast<uint8_t>(m_reasonCode));
        }

        if constexpr (Traits::HasProperties)
        {
            m_properties.encode(writer);
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool PubRel<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PubRel] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PubRel] MQTT 3.1.1");
        }

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            return false;
        }

        if constexpr (Traits::HasReasonCode)
        {
            if (reader.getRemaining() == 0)
            {
                m_reasonCode = ReasonCode::Success;
                return true;
            }

            uint8_t reasonCodeByte = 0;
            if (!reader.tryReadUint8(reasonCodeByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read reason code");
                return false;
            }
            m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);
        }

        if constexpr (Traits::HasProperties)
        {
            if (reader.getRemaining() > 0)
            {
                m_properties.decode(reader);
            }
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t PubRel<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    ReasonCode PubRel<TProtocolVersion>::getReasonCode() const
        requires(detail::PubRelTraits<TProtocolVersion>::HasReasonCode)
    {
        return m_reasonCode;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::PubRelTraits<TProtocolVersion>::PropertiesType& PubRel<TProtocolVersion>::getProperties() const
        requires(detail::PubRelTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t PubRel<TProtocolVersion>::getPropertiesLength() const
    {
        if constexpr (Traits::HasProperties)
        {
            return m_properties.getLength(false);
        }
        return 0;
    }

    template class PubRel<ProtocolVersion::V311>;
    template class PubRel<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodePubRelToWriter(
        ByteWriter& writer, uint16_t packetId, ReasonCode reasonCode, typename detail::PubRelTraits<V>::PropertiesType&& properties)
    {
        using Traits = detail::PubRelTraits<V>;
        if constexpr (Traits::HasReasonCode)
        {
            PubRel<V> packet(packetId, reasonCode, std::move(properties));
            packet.encode(writer);
        }
        else
        {
            PubRel<V> packet(packetId);
            packet.encode(writer);
        }
    }

    template void encodePubRelToWriter<ProtocolVersion::V311>(
        ByteWriter&, uint16_t, ReasonCode, typename detail::PubRelTraits<ProtocolVersion::V311>::PropertiesType&&);
    template void encodePubRelToWriter<ProtocolVersion::V5>(
        ByteWriter&, uint16_t, ReasonCode, typename detail::PubRelTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
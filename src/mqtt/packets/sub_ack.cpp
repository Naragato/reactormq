//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "sub_ack.h"

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
    SubAck<TProtocolVersion>::SubAck(
        const uint16_t packetId, std::vector<typename detail::SubAckTraits<TProtocolVersion>::ReasonCodeType> reasonCodes)
        : m_packetIdentifier(packetId)
        , m_reasonCodes(std::move(reasonCodes))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    SubAck<TProtocolVersion>::SubAck(
        const uint16_t packetId,
        std::vector<typename detail::SubAckTraits<TProtocolVersion>::ReasonCodeType> reasonCodes,
        typename detail::SubAckTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::SubAckTraits<TProtocolVersion>::HasProperties)
        : m_packetIdentifier(packetId)
        , m_reasonCodes(std::move(reasonCodes))
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    SubAck<TProtocolVersion>::SubAck(ByteReader& reader, const FixedHeader& fixedHeader)
        : ISubAckPacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t SubAck<TProtocolVersion>::getLength() const
    {
        uint32_t length = 2; // Packet ID

        if constexpr (Traits::HasProperties)
        {
            const uint32_t propsLen = getPropertiesLength();
            length += serialize::variableByteIntegerSize(propsLen) + propsLen;
        }

        length += static_cast<uint32_t>(m_reasonCodes.size());
        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void SubAck<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][SubAck] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][SubAck] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if constexpr (Traits::HasProperties)
        {
            m_properties.encode(writer);
        }

        for (const auto& code : m_reasonCodes)
        {
            writer.writeUint8(static_cast<uint8_t>(code));
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool SubAck<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][SubAck] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][SubAck] MQTT 3.1.1");
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
            // Calculate properties encoded size to deduce remaining length for codes
            const uint32_t propsLen = m_properties.getLength(false);
            const uint32_t varIntSize = serialize::variableByteIntegerSize(propsLen);

            if (remainingLength < varIntSize + propsLen)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Invalid remaining length after properties");
                return false;
            }
            remainingLength -= (varIntSize + propsLen);
        }

        while (remainingLength > 0)
        {
            uint8_t codeByte = 0;
            if (!reader.tryReadUint8(codeByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read return code");
                return false;
            }
            m_reasonCodes.push_back(static_cast<ReasonCodeType>(codeByte));
            --remainingLength;
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t SubAck<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::vector<typename SubAck<TProtocolVersion>::ReasonCodeType>& SubAck<TProtocolVersion>::getReasonCodes() const
    {
        return m_reasonCodes;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::SubAckTraits<TProtocolVersion>::PropertiesType& SubAck<TProtocolVersion>::getProperties() const
        requires(detail::SubAckTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t SubAck<TProtocolVersion>::getPropertiesLength() const
    {
        if constexpr (Traits::HasProperties)
        {
            return m_properties.getLength(false);
        }
        return 0;
    }

    template class SubAck<ProtocolVersion::V311>;
    template class SubAck<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeSubAckToWriter(
        ByteWriter& writer,
        uint16_t packetId,
        const std::vector<typename detail::SubAckTraits<V>::ReasonCodeType>& reasonCodes,
        typename detail::SubAckTraits<V>::PropertiesType&& properties)
    {
        using Traits = detail::SubAckTraits<V>;
        if constexpr (Traits::HasProperties)
        {
            SubAck<V> packet(packetId, reasonCodes, std::move(properties));
            packet.encode(writer);
        }
        else
        {
            SubAck<V> packet(packetId, reasonCodes);
            packet.encode(writer);
        }
    }

    template void encodeSubAckToWriter<ProtocolVersion::V311>(
        ByteWriter&,
        uint16_t,
        const std::vector<detail::SubAckTraits<ProtocolVersion::V311>::ReasonCodeType>&,
        typename detail::SubAckTraits<ProtocolVersion::V311>::PropertiesType&&);
    template void encodeSubAckToWriter<ProtocolVersion::V5>(
        ByteWriter&,
        uint16_t,
        const std::vector<detail::SubAckTraits<ProtocolVersion::V5>::ReasonCodeType>&,
        typename detail::SubAckTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "unsubscribe.h"

#include <utility>

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
    Unsubscribe<TProtocolVersion>::Unsubscribe(std::vector<std::string> topicFilters, const uint16_t packetId)
        : m_packetIdentifier(packetId)
        , m_topicFilters(std::move(topicFilters))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    Unsubscribe<TProtocolVersion>::Unsubscribe(
        std::vector<std::string> topicFilters,
        const uint16_t packetId,
        typename detail::UnsubscribeTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::UnsubscribeTraits<TProtocolVersion>::HasProperties)
        : m_packetIdentifier(packetId)
        , m_topicFilters(std::move(topicFilters))
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    Unsubscribe<TProtocolVersion>::Unsubscribe(ByteReader& reader, const FixedHeader& fixedHeader)
        : IUnsubscribePacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Unsubscribe<TProtocolVersion>::getLength() const
    {
        uint32_t length = 2; // Packet ID

        if constexpr (Traits::HasProperties)
        {
            length += m_properties.getLength(true);
        }

        for (const auto& topicFilter : m_topicFilters)
        {
            length += kStringLengthFieldSize;
            length += static_cast<uint32_t>(topicFilter.length());
        }
        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void Unsubscribe<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Unsubscribe] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Unsubscribe] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if constexpr (Traits::HasProperties)
        {
            m_properties.encode(writer);
        }

        for (const auto& topicFilter : m_topicFilters)
        {
            serialize::encodeString(topicFilter, writer);
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool Unsubscribe<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Unsubscribe] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Unsubscribe] MQTT 3.1.1");
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
            const uint32_t propsLen = m_properties.getLength(true);
            if (remainingLength < propsLen)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Invalid remaining length after properties");
                return false;
            }
            remainingLength -= propsLen;
        }

        while (remainingLength > 0)
        {
            if (reader.getRemaining() == 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Unexpected end of stream");
                return false;
            }

            std::string topicFilter;
            if (!serialize::decodeString(reader, topicFilter))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read topic filter");
                return false;
            }

            const uint32_t strLen = kStringLengthFieldSize + static_cast<uint32_t>(topicFilter.length());
            if (remainingLength < strLen)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Topic filter exceeds packet length");
                return false;
            }
            remainingLength -= strLen;

            m_topicFilters.emplace_back(std::move(topicFilter));
        }

        if (m_topicFilters.empty())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "No topic filters provided");
            return false;
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t Unsubscribe<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::vector<std::string>& Unsubscribe<TProtocolVersion>::getTopicFilters() const
    {
        return m_topicFilters;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::UnsubscribeTraits<TProtocolVersion>::PropertiesType& Unsubscribe<TProtocolVersion>::getProperties() const
        requires(detail::UnsubscribeTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Unsubscribe<TProtocolVersion>::getPropertiesLength() const
    {
        if constexpr (Traits::HasProperties)
        {
            return m_properties.getLength(false);
        }
        return 0;
    }

    template class Unsubscribe<ProtocolVersion::V311>;
    template class Unsubscribe<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeUnsubscribeToWriter(
        ByteWriter& writer,
        const std::vector<std::string>& topicFilters,
        uint16_t packetId,
        typename detail::UnsubscribeTraits<V>::PropertiesType&& properties)
    {
        using Traits = detail::UnsubscribeTraits<V>;
        if constexpr (Traits::HasProperties)
        {
            Unsubscribe<V> packet(topicFilters, packetId, std::move(properties));
            packet.encode(writer);
        }
        else
        {
            Unsubscribe<V> packet(topicFilters, packetId);
            packet.encode(writer);
        }
    }

    template void encodeUnsubscribeToWriter<ProtocolVersion::V311>(
        ByteWriter&,
        const std::vector<std::string>&,
        uint16_t,
        typename detail::UnsubscribeTraits<ProtocolVersion::V311>::PropertiesType&&);

    template void encodeUnsubscribeToWriter<ProtocolVersion::V5>(
        ByteWriter&, const std::vector<std::string>&, uint16_t, typename detail::UnsubscribeTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
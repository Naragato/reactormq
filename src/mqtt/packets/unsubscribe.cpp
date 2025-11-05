#include "unsubscribe.h"
#include <utility>

namespace reactormq::mqtt::packets
{
    void Unsubscribe<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Unsubscribe] MQTT 3.1.1");
        m_fixedHeader.encode(writer);

        writer.writeUint16(m_packetIdentifier);

        for (const auto& topicFilter : m_topicFilters)
        {
            serialize::encodeString(topicFilter, writer);
        }
    }

    bool Unsubscribe<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Unsubscribe] MQTT 3.1.1");

        if (m_fixedHeader.getPacketType() != PacketType::Unsubscribe)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Unsubscribe", "[Unsubscribe] Invalid packet type: expected Unsubscribe, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            return false;
        }

        int32_t payloadSize = static_cast<int32_t>(m_fixedHeader.getRemainingLength()) - 2;

        size_t readerPosBefore = reader.getRemaining();
        while (payloadSize > 0)
        {
            std::string topicFilter;
            if (!serialize::decodeString(reader, topicFilter))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read topic filter");
                return false;
            }

            const size_t readerPosAfter = reader.getRemaining();
            if (readerPosBefore == readerPosAfter)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Unsubscribe", "Archive too small - no progress reading topic filter");
                return false;
            }

            readerPosBefore = readerPosAfter;
            payloadSize -= static_cast<int32_t>(kStringLengthFieldSize + topicFilter.length());
            m_topicFilters.emplace_back(std::move(topicFilter));
        }

        if (m_topicFilters.empty())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "No topic filters provided");
            return false;
        }

        return true;
    }

    void Unsubscribe<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Unsubscribe] MQTT 5");

        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);

        m_properties.encode(writer);

        for (const auto& topicFilter : m_topicFilters)
        {
            serialize::encodeString(topicFilter, writer);
        }
    }

    bool Unsubscribe<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Unsubscribe] MQTT 5");

        if (m_fixedHeader.getPacketType() != PacketType::Unsubscribe)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Unsubscribe", "[Unsubscribe] Invalid packet type: expected Unsubscribe, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            return false;
        }

        m_properties.decode(reader);

        auto payloadSize = static_cast<int32_t>(m_fixedHeader.getRemainingLength());
        payloadSize -= 2;
        payloadSize -= static_cast<int32_t>(m_properties.getLength());

        size_t readerPosBefore = reader.getRemaining();
        while (payloadSize > 0)
        {
            std::string topicFilter;
            if (!serialize::decodeString(reader, topicFilter))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read topic filter");
                return false;
            }

            const size_t readerPosAfter = reader.getRemaining();
            if (readerPosBefore == readerPosAfter)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Unsubscribe", "Archive too small - no progress reading topic filter");
                return false;
            }

            readerPosBefore = readerPosAfter;
            payloadSize -= static_cast<int32_t>(kStringLengthFieldSize + topicFilter.length());
            m_topicFilters.emplace_back(std::move(topicFilter));
        }

        if (m_topicFilters.empty())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "No topic filters provided");
            return false;
        }

        return true;
    }
} // namespace reactormq::mqtt::packets
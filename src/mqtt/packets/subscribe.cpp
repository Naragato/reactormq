#include "subscribe.h"

namespace reactormq::mqtt::packets
{
    // MQTT 3.1.1
    void Subscribe<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Subscribe] MQTT 3.1.1");
        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);

        for (const auto& topicFilter : m_topicFilters)
        {
            serialize::encodeString(topicFilter.getFilter(), writer);
            writer.writeUint8(static_cast<uint8_t>(topicFilter.getQualityOfService()));
        }
    }

    bool Subscribe<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Subscribe] MQTT 3.1.1");

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            m_isValid = false;
            return false;
        }

        while (reader.getRemaining() > 0)
        {
            std::string topicFilterStr;
            if (!serialize::decodeString(reader, topicFilterStr))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read topic filter");
                m_isValid = false;
                return false;
            }

            uint8_t qosByte = 0;
            if (!reader.tryReadUint8(qosByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read QoS");
                m_isValid = false;
                return false;
            }

            auto qos = static_cast<QualityOfService>(qosByte);
            m_topicFilters.emplace_back(std::move(topicFilterStr), qos);
        }

        return true;
    }

    // MQTT 5
    void Subscribe<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Subscribe] MQTT 5");
        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);
        m_properties.encode(writer);

        for (const auto& topicFilter : m_topicFilters)
        {
            serialize::encodeString(topicFilter.getFilter(), writer);

            auto subscribeOptions = static_cast<uint8_t>(topicFilter.getQualityOfService());
            subscribeOptions |= topicFilter.getIsNoLocal() ? 0x04 : 0;
            subscribeOptions |= topicFilter.getIsRetainAsPublished() ? 0x08 : 0;
            const auto retainHandling = static_cast<uint8_t>(topicFilter.getRetainHandlingOptions());
            subscribeOptions |= retainHandling << 4;

            writer.writeUint8(subscribeOptions);
        }
    }

    bool Subscribe<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Subscribe] MQTT 5");

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            m_isValid = false;
            return false;
        }

        m_properties.decode(reader);

        while (reader.getRemaining() > 0)
        {
            std::string topicFilterStr;
            if (!serialize::decodeString(reader, topicFilterStr))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read topic filter");
                m_isValid = false;
                return false;
            }

            uint8_t subscribeOptions = 0;
            if (!reader.tryReadUint8(subscribeOptions))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read subscription options");
                m_isValid = false;
                return false;
            }

            const auto qos = static_cast<QualityOfService>(subscribeOptions & 0x03);
            const bool noLocal = (subscribeOptions & 0x04) != 0;
            const bool retainAsPublished = (subscribeOptions & 0x08) != 0;
            const auto retainHandling = static_cast<RetainHandlingOptions>((subscribeOptions & 0x30) >> 4);

            if ((subscribeOptions & 0xC0) != 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Invalid subscription options: reserved bits set");
                m_isValid = false;
                return false;
            }

            m_topicFilters.emplace_back(
                std::move(topicFilterStr),
                qos,
                noLocal,
                retainAsPublished,
                retainHandling);
        }

        return true;
    }
} // namespace reactormq::mqtt::packets
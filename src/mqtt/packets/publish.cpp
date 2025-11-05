#include "publish.h"

namespace reactormq::mqtt::packets
{
    void Publish<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Publish] MQTT 3.1.1");
        m_fixedHeader.encode(writer);
        serialize::encodeString(m_topicName, writer);

        if (static_cast<uint8_t>(getQualityOfService()) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            writer.writeUint16(m_packetIdentifier);
        }

        writer.writeBytes(reinterpret_cast<const std::byte*>(m_payload.data()), m_payload.size());
    }

    bool Publish<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Publish] MQTT 3.1.1");

        if (m_fixedHeader.getPacketType() != PacketType::Publish)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Publish", "[Publish] Invalid packet type: expected Publish, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        auto payloadSize = static_cast<int32_t>(m_fixedHeader.getRemainingLength());
        payloadSize -= 2;

        if (!serialize::decodeString(reader, m_topicName))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Failed to decode topic name");
            return false;
        }
        payloadSize -= static_cast<int32_t>(m_topicName.length());

        if (static_cast<uint8_t>(getQualityOfService()) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            if (!reader.tryReadUint16(m_packetIdentifier))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Failed to read packet identifier");
                return false;
            }
            payloadSize -= 2;
        }

        if (payloadSize < 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Publish", "[Publish] Invalid payload size: %d", payloadSize);
            return false;
        }

        if (payloadSize > reader.getRemaining())
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Publish", "[Publish] Not enough data for payload (need %d, have %zu)", payloadSize,
                reader.getRemaining());
            return false;
        }

        m_payload.resize(payloadSize);
        const auto dst = std::as_writable_bytes(std::span{ m_payload });
        reader.readBytes(dst);

        return true;
    }

    void Publish<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Publish] MQTT 5");
        m_fixedHeader.encode(writer);
        serialize::encodeString(m_topicName, writer);

        if (static_cast<uint8_t>(getQualityOfService()) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            writer.writeUint16(m_packetIdentifier);
        }

        m_properties.encode(writer);

        writer.writeBytes(reinterpret_cast<const std::byte*>(m_payload.data()), m_payload.size());
    }

    bool Publish<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Publish] MQTT 5");

        if (m_fixedHeader.getPacketType() != PacketType::Publish)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Publish", "[Publish] Invalid packet type: expected Publish, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        auto payloadSize = static_cast<int32_t>(m_fixedHeader.getRemainingLength());
        payloadSize -= 2;

        if (!serialize::decodeString(reader, m_topicName))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Failed to decode topic name");
            return false;
        }
        payloadSize -= static_cast<int32_t>(m_topicName.length());

        if (static_cast<uint8_t>(getQualityOfService()) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            if (!reader.tryReadUint16(m_packetIdentifier))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Failed to read packet identifier");
                return false;
            }
            payloadSize -= 2;
        }

        m_properties.decode(reader);
        payloadSize -= static_cast<int32_t>(m_properties.getLength());

        if (payloadSize < 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Publish", "[Publish] Invalid payload size: %d", payloadSize);
            return false;
        }

        const auto size = static_cast<size_t>(payloadSize);

        if (size > reader.getRemaining())
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Publish", "[Publish] Not enough data for payload (need %d, have %zu)", payloadSize,
                reader.getRemaining());
            return false;
        }

        m_payload.resize(size);

        const std::span dst{ reinterpret_cast<std::byte*>(m_payload.data()), size };

        reader.readBytes(dst);

        return true;
    }
} // namespace reactormq::mqtt::packets
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "publish.h"

namespace reactormq::mqtt::packets
{
    using serialize::ByteReader;
    using serialize::ByteWriter;

    template<ProtocolVersion TProtocolVersion>
    Publish<TProtocolVersion>::Publish(ByteReader& reader, const FixedHeader& fixedHeader)
        : IPublishPacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    Publish<TProtocolVersion>::Publish(
        std::string topicName,
        std::vector<uint8_t> payload,
        const QualityOfService qos,
        const bool shouldRetain,
        const uint16_t packetIdentifier,
        const bool isDuplicate)
        : m_topicName(std::move(topicName))
        , m_packetIdentifier(packetIdentifier)
        , m_payload(std::move(payload))
    {
        const auto remainingLength
            = getLength(static_cast<uint32_t>(m_topicName.length()), static_cast<uint32_t>(m_payload.size()), getPropertiesLength(), qos);

        this->setFixedHeader(FixedHeader::create(this, remainingLength, shouldRetain, qos, isDuplicate));
    }

    template<ProtocolVersion TProtocolVersion>
    Publish<TProtocolVersion>::Publish(
        std::string topicName,
        std::vector<uint8_t> payload,
        const QualityOfService qos,
        const bool shouldRetain,
        const uint16_t packetIdentifier,
        const typename detail::PublishTraits<TProtocolVersion>::PropertiesType& properties,
        const bool isDuplicate)
        requires(detail::PublishTraits<TProtocolVersion>::HasProperties)
        : m_topicName(std::move(topicName))
        , m_packetIdentifier(packetIdentifier)
        , m_payload(std::move(payload))
        , m_properties(properties)
    {
        const auto remainingLength
            = getLength(static_cast<uint32_t>(m_topicName.length()), static_cast<uint32_t>(m_payload.size()), getPropertiesLength(), qos);

        this->setFixedHeader(FixedHeader::create(this, remainingLength, shouldRetain, qos, isDuplicate));
    }

    template<ProtocolVersion TProtocolVersion>
    bool Publish<TProtocolVersion>::getIsDuplicate() const
    {
        const auto flags = static_cast<std::byte>(this->getFixedHeader().getFlags());
        return getQualityOfService() != QualityOfService::AtMostOnce && (flags & kDupBit) == kDupBit;
    }

    template<ProtocolVersion TProtocolVersion>
    bool Publish<TProtocolVersion>::getShouldRetain() const
    {
        const auto flags = static_cast<std::byte>(this->getFixedHeader().getFlags());
        return (flags & kRetainBit) == kRetainBit;
    }

    template<ProtocolVersion TProtocolVersion>
    QualityOfService Publish<TProtocolVersion>::getQualityOfService() const
    {
        const auto flags = static_cast<std::byte>(this->getFixedHeader().getFlags());
        const auto qosBits = (flags >> kQosShift) & kQosMask;
        return static_cast<QualityOfService>(std::to_integer<uint8_t>(qosBits));
    }

    template<ProtocolVersion TProtocolVersion>
    const std::string& Publish<TProtocolVersion>::getTopicName() const
    {
        return m_topicName;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t Publish<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::vector<uint8_t>& Publish<TProtocolVersion>::getPayload() const
    {
        return m_payload;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::PublishTraits<TProtocolVersion>::PropertiesType& Publish<TProtocolVersion>::getProperties() const
        requires(detail::PublishTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Publish<TProtocolVersion>::getLength() const
    {
        return getLength(
            static_cast<uint32_t>(m_topicName.length()),
            static_cast<uint32_t>(m_payload.size()),
            getPropertiesLength(),
            getQualityOfService());
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Publish<TProtocolVersion>::getLength(
        const uint32_t topicNameLength, const uint32_t payloadLength, const uint32_t propertiesLength, QualityOfService qos)
    {
        uint32_t length = kStringLengthFieldSize + topicNameLength + payloadLength;

        if constexpr (detail::PublishTraits<TProtocolVersion>::HasProperties)
        {
            length += propertiesLength;
        }

        if (static_cast<uint8_t>(qos) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            length += sizeof(uint16_t);
        }

        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void Publish<TProtocolVersion>::setPayload(std::vector<uint8_t>&& payload)
    {
        m_payload = std::move(payload);
    }

    template<ProtocolVersion TProtocolVersion>
    void Publish<TProtocolVersion>::setPacketId(const uint16_t packetId)
    {
        m_packetIdentifier = packetId;
    }

    template<ProtocolVersion TProtocolVersion>
    void Publish<TProtocolVersion>::setTopicName(std::string&& topicName)
    {
        m_topicName = std::move(topicName);
    }

    template<ProtocolVersion TProtocolVersion>
    void Publish<TProtocolVersion>::readPayload(ByteReader reader, const std::vector<uint8_t>::size_type payloadSize)
    {
        m_payload.resize(payloadSize);
        const auto dst = std::as_writable_bytes(std::span{ m_payload });
        reader.readBytes(dst);
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Publish<TProtocolVersion>::getPropertiesLength() const
    {
        if constexpr (detail::PublishTraits<TProtocolVersion>::HasProperties)
        {
            return m_properties.getLength();
        }
        else
        {
            return 0;
        }
    }

    template<ProtocolVersion TProtocolVersion>
    void Publish<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Publish] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Publish] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        serialize::encodeString(m_topicName, writer);

        if (static_cast<uint8_t>(getQualityOfService()) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            writer.writeUint16(m_packetIdentifier);
        }

        if constexpr (detail::PublishTraits<TProtocolVersion>::HasProperties)
        {
            m_properties.encode(writer);
        }

        writer.writeBytes(reinterpret_cast<const std::byte*>(m_payload.data()), m_payload.size());
    }

    template<ProtocolVersion TProtocolVersion>
    bool Publish<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Publish] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Publish] MQTT 3.1.1");
        }

        if (this->getFixedHeader().getPacketType() != PacketType::Publish)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Publish] Invalid packet type: expected Publish, got %s",
                packetTypeToString(this->getFixedHeader().getPacketType()));
            return false;
        }

        auto payloadSize = static_cast<int32_t>(this->getFixedHeader().getRemainingLength());
        payloadSize -= 2;

        std::string topicName;
        if (!serialize::decodeString(reader, topicName))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Failed to decode topic name");
            return false;
        }

        setTopicName(std::move(topicName));
        payloadSize -= static_cast<int32_t>(m_topicName.length());

        if (static_cast<uint8_t>(getQualityOfService()) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
        {
            uint16_t packetId = 0;
            if (!reader.tryReadUint16(packetId))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Failed to read packet identifier");
                return false;
            }

            setPacketId(packetId);
            payloadSize -= 2;
        }

        if constexpr (detail::PublishTraits<TProtocolVersion>::HasProperties)
        {
            m_properties.decode(reader);
            payloadSize -= static_cast<int32_t>(m_properties.getLength());
        }

        if (payloadSize < 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Publish] Invalid payload size: %d", payloadSize);
            return false;
        }

        const auto size = static_cast<size_t>(payloadSize);

        if (size > reader.getRemaining())
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Publish] Not enough data for payload (need %d, have %zu)",
                payloadSize,
                reader.getRemaining());
            return false;
        }

        readPayload(reader, size);
        return true;
    }

    template<ProtocolVersion V>
    void encodePublishToWriter(
        ByteWriter& writer,
        const std::string& topic,
        const std::vector<uint8_t>& payload,
        QualityOfService qos,
        bool shouldRetain,
        std::uint16_t packetId,
        bool isDuplicate)
    {
        using Traits = detail::PublishTraits<V>;
        using PublishT = Publish<V>;

        if constexpr (Traits::HasProperties)
        {
            properties::Properties props{};
            PublishT publishPacket(topic, payload, qos, shouldRetain, packetId, props, isDuplicate);
            publishPacket.encode(writer);
        }
        else
        {
            PublishT publishPacket(topic, payload, qos, shouldRetain, packetId, isDuplicate);
            publishPacket.encode(writer);
        }
    }

    template class Publish<ProtocolVersion::V311>;
    template class Publish<ProtocolVersion::V5>;

    template void encodePublishToWriter<ProtocolVersion::V311>(
        ByteWriter&, const std::string&, const std::vector<uint8_t>&, QualityOfService, bool, std::uint16_t, bool);

    template void encodePublishToWriter<ProtocolVersion::V5>(
        ByteWriter&, const std::string&, const std::vector<uint8_t>&, QualityOfService, bool, std::uint16_t, bool);
} // namespace reactormq::mqtt::packets
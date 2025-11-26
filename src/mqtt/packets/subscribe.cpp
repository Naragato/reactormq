//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "subscribe.h"

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
    Subscribe<TProtocolVersion>::Subscribe(std::vector<TopicFilter> topicFilters, const uint16_t packetId)
        : m_packetIdentifier(packetId)
        , m_topicFilters(std::move(topicFilters))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    Subscribe<TProtocolVersion>::Subscribe(
        std::vector<TopicFilter> topicFilters,
        const uint16_t packetId,
        typename detail::SubscribeTraits<TProtocolVersion>::PropertiesType&& properties)
        requires(detail::SubscribeTraits<TProtocolVersion>::HasProperties)
        : m_packetIdentifier(packetId)
        , m_topicFilters(std::move(topicFilters))
        , m_properties(std::move(properties))
    {
        this->setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    Subscribe<TProtocolVersion>::Subscribe(ByteReader& reader, const FixedHeader& fixedHeader)
        : ISubscribePacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Subscribe<TProtocolVersion>::getLength() const
    {
        uint32_t length = 2; // Packet ID

        if constexpr (Traits::HasProperties)
        {
            length += m_properties.getLength(true);
        }

        for (const auto& topicFilter : m_topicFilters)
        {
            length += kStringLengthFieldSize;
            length += static_cast<uint32_t>(topicFilter.getFilter().length());
            length += 1; // QoS or Options byte (always 1 byte)
        }
        return length;
    }

    template<ProtocolVersion TProtocolVersion>
    void Subscribe<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Subscribe] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Subscribe] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if constexpr (Traits::HasProperties)
        {
            m_properties.encode(writer);
        }

        for (const auto& topicFilter : m_topicFilters)
        {
            serialize::encodeString(topicFilter.getFilter(), writer);

            if constexpr (TProtocolVersion == ProtocolVersion::V5)
            {
                std::byte subscribeOptions{};
                subscribeOptions |= static_cast<std::byte>(topicFilter.getQualityOfService()) & kQosMask;
                if (topicFilter.getIsNoLocal())
                {
                    subscribeOptions |= kNoLocalBit;
                }
                if (topicFilter.getIsRetainAsPublished())
                {
                    subscribeOptions |= kRetainAsPublishedBit;
                }
                const auto retainHandling = static_cast<std::byte>(topicFilter.getRetainHandlingOptions()) & kRetainHandlingMask;
                subscribeOptions |= retainHandling << kRetainHandlingShift;

                writer.writeUint8(std::to_integer<uint8_t>(subscribeOptions));
            }
            else
            {
                writer.writeUint8(static_cast<uint8_t>(topicFilter.getQualityOfService()));
            }
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool Subscribe<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Subscribe] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Subscribe] MQTT 3.1.1");
        }

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            return false;
        }

        if constexpr (Traits::HasProperties)
        {
            m_properties.decode(reader);
        }

        while (reader.getRemaining() > 0)
        {
            std::string topicFilterStr;
            if (!serialize::decodeString(reader, topicFilterStr))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read topic filter");
                return false;
            }

            uint8_t optionsByte = 0;
            if (!reader.tryReadUint8(optionsByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read options/QoS");
                return false;
            }

            if constexpr (TProtocolVersion == ProtocolVersion::V5)
            {
                const auto subscribeOptions = static_cast<std::byte>(optionsByte);
                const auto qos = static_cast<QualityOfService>(std::to_integer<uint8_t>(subscribeOptions & kQosMask));
                bool noLocal = (subscribeOptions & kNoLocalBit) != std::byte{};
                bool retainAsPublished = (subscribeOptions & kRetainAsPublishedBit) != std::byte{};
                const auto retainHandling
                    = static_cast<RetainHandlingOptions>(subscribeOptions >> kRetainHandlingShift & kRetainHandlingMask);

                if ((subscribeOptions & kReservedMask) != std::byte{})
                {
                    REACTORMQ_LOG(logging::LogLevel::Error, "Invalid subscription options: reserved bits set");
                    return false;
                }

                m_topicFilters.emplace_back(std::move(topicFilterStr), qos, noLocal, retainAsPublished, retainHandling);
            }
            else
            {
                auto qos = static_cast<QualityOfService>(optionsByte);
                m_topicFilters.emplace_back(std::move(topicFilterStr), qos);
            }
        }

        return true;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t Subscribe<TProtocolVersion>::getPacketId() const
    {
        return m_packetIdentifier;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::vector<TopicFilter>& Subscribe<TProtocolVersion>::getTopicFilters() const
    {
        return m_topicFilters;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::SubscribeTraits<TProtocolVersion>::PropertiesType& Subscribe<TProtocolVersion>::getProperties() const
        requires(detail::SubscribeTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Subscribe<TProtocolVersion>::getPropertiesLength() const
    {
        if constexpr (Traits::HasProperties)
        {
            return m_properties.getLength(false);
        }
        return 0;
    }

    template class Subscribe<ProtocolVersion::V311>;
    template class Subscribe<ProtocolVersion::V5>;

    template<ProtocolVersion V>
    void encodeSubscribeToWriter(
        ByteWriter& writer,
        const std::vector<TopicFilter>& topicFilters,
        uint16_t packetId,
        typename detail::SubscribeTraits<V>::PropertiesType&& properties)
    {
        using Traits = detail::SubscribeTraits<V>;
        if constexpr (Traits::HasProperties)
        {
            Subscribe<V> packet(topicFilters, packetId, std::move(properties));
            packet.encode(writer);
        }
        else
        {
            Subscribe<V> packet(topicFilters, packetId);
            packet.encode(writer);
        }
    }

    template void encodeSubscribeToWriter<ProtocolVersion::V311>(
        ByteWriter&, const std::vector<TopicFilter>&, uint16_t, typename detail::SubscribeTraits<ProtocolVersion::V311>::PropertiesType&&);
    template void encodeSubscribeToWriter<ProtocolVersion::V5>(
        ByteWriter&, const std::vector<TopicFilter>&, uint16_t, typename detail::SubscribeTraits<ProtocolVersion::V5>::PropertiesType&&);
} // namespace reactormq::mqtt::packets

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
#include "connect.h"

namespace reactormq::mqtt::packets
{
    uint32_t Connect<ProtocolVersion::V311>::getLength() const
    {
        constexpr uint32_t kVariableHeaderLength = 10;
        uint32_t payloadLength = 0;

        payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_clientId.size());

        if (!m_willTopic.empty())
        {
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_willTopic.size());
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_willMessage.size());
        }

        if (!m_username.empty())
        {
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_username.size());
        }

        if (!m_password.empty())
        {
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_password.size());
        }

        return kVariableHeaderLength + payloadLength;
    }

    void Connect<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Connect] MQTT 3.1.1");
        m_fixedHeader.encode(writer);
        serialize::encodeString("MQTT", writer);

        constexpr auto kProtocolLevel = static_cast<uint8_t>(ProtocolVersion::V311);
        writer.writeUint8(kProtocolLevel);

        std::byte flags{};

        if (m_bCleanSession)
        {
            flags |= kCleanSessionBit;
        }

        const bool bHasWill = !m_willTopic.empty();
        if (bHasWill)
        {
            const auto qosBits = (static_cast<std::byte>(static_cast<uint8_t>(m_willQos)) & kWillQosMask) << kWillQosShift;

            flags |= qosBits | kWillFlagBit;

            if (m_bRetainWill)
            {
                flags |= kWillRetainBit;
            }
        }

        const bool bHasUsername = !m_username.empty();
        if (bHasUsername)
        {
            flags |= kUsernameBit;
        }

        const bool bHasPassword = !m_password.empty();
        if (bHasPassword)
        {
            flags |= kPasswordBit;
        }

        writer.writeUint8(std::to_integer<uint8_t>(flags));
        writer.writeUint16(m_keepAliveSeconds);

        if (!m_clientId.empty())
        {
            serialize::encodeString(m_clientId, writer);
        }
        else
        {
            writer.writeUint16(0);
        }

        if (bHasWill)
        {
            serialize::encodeString(m_willTopic, writer);
            serialize::encodeString(m_willMessage, writer);
        }

        if (bHasUsername)
        {
            serialize::encodeString(m_username, writer);
        }

        if (bHasPassword)
        {
            serialize::encodeString(m_password, writer);
        }
    }

    bool Connect<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Connect] MQTT 3.1.1");
        if (m_fixedHeader.getPacketType() != PacketType::Connect)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Connect", "[Connect] Invalid packet type: expected Connect, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        std::string protocolName;
        if (!serialize::decodeString(reader, protocolName))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode protocol name");
            return false;
        }

        if (protocolName != "MQTT")
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Connect", "[Connect] Invalid protocol name: %s", protocolName.c_str());
            return false;
        }

        uint8_t protocolLevel = reader.readUint8();
        if (constexpr uint8_t kExpectedProtocolLevel = 4; protocolLevel != kExpectedProtocolLevel)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Connect", "[Connect] Invalid protocol version: %d (expected %d)", protocolLevel,
                kExpectedProtocolLevel);
            return false;
        }

        std::byte flags = static_cast<std::byte>(reader.readUint8());

        m_bCleanSession = (flags & kCleanSessionBit) != std::byte{};

        if (!reader.tryReadUint16(m_keepAliveSeconds))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to read keep alive");
            return false;
        }

        if (!serialize::decodeString(reader, m_clientId))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode client ID");
            return false;
        }

        if ((flags & kWillFlagBit) != std::byte{})
        {
            auto qosByte = ((flags >> kWillQosShift) & kWillQosMask);
            m_willQos = static_cast<QualityOfService>(std::to_integer<uint8_t>(qosByte));

            m_bRetainWill = (flags & kWillRetainBit) != std::byte{};

            if (!serialize::decodeString(reader, m_willTopic))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode will topic");
                return false;
            }

            if (!serialize::decodeString(reader, m_willMessage))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode will message");
                return false;
            }

            if (m_willTopic.empty())
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will topic is empty");
                return false;
            }

            if (m_willMessage.empty())
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will message is empty");
                return false;
            }
        }
        else
        {
            if (((flags >> kWillQosShift) & kWillQosMask) != std::byte{})
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will QoS must be 0 when will flag is 0");
                return false;
            }

            if ((flags & kWillRetainBit) != std::byte{})
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will retain must be 0 when will flag is 0");
                return false;
            }
        }

        if ((flags & kUsernameBit) != std::byte{} && !serialize::decodeString(reader, m_username))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode username");
            return false;
        }

        if ((flags & kPasswordBit) != std::byte{} && !serialize::decodeString(reader, m_password))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode password");
            return false;
        }

        return true;
    }

    uint32_t Connect<ProtocolVersion::V5>::getLength() const
    {
        constexpr uint32_t kVariableHeaderLength = 10;
        uint32_t payloadLength = 0;

        payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_clientId.size());

        if (!m_willTopic.empty())
        {
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_willTopic.size());
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_willMessage.size());
        }

        if (!m_username.empty())
        {
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_username.size());
        }

        if (!m_password.empty())
        {
            payloadLength += kStringLengthFieldSize + static_cast<uint32_t>(m_password.size());
        }

        payloadLength += m_properties.getLength();

        if (!m_willTopic.empty())
        {
            payloadLength += m_willProperties.getLength();
        }

        return kVariableHeaderLength + payloadLength;
    }

    void Connect<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Connect] MQTT 5");
        m_fixedHeader.encode(writer);

        serialize::encodeString("MQTT", writer);

        constexpr auto kProtocolLevel = static_cast<uint8_t>(ProtocolVersion::V5);
        writer.writeUint8(kProtocolLevel);

        std::byte flags{};

        if (m_bCleanSession)
        {
            flags |= kCleanSessionBit;
        }

        const bool bHasWill = !m_willTopic.empty();
        if (bHasWill)
        {
            const auto qosBits = (static_cast<std::byte>(static_cast<uint8_t>(m_willQos)) & kWillQosMask) << kWillQosShift;

            flags |= qosBits | kWillFlagBit;

            if (m_bRetainWill)
            {
                flags |= kWillRetainBit;
            }
        }

        const bool bHasUsername = !m_username.empty();
        if (bHasUsername)
        {
            flags |= kUsernameBit;
        }

        const bool bHasPassword = !m_password.empty();
        if (bHasPassword)
        {
            flags |= kPasswordBit;
        }

        writer.writeUint8(std::to_integer<uint8_t>(flags));
        writer.writeUint16(m_keepAliveSeconds);
        m_properties.encode(writer);

        if (!m_clientId.empty())
        {
            serialize::encodeString(m_clientId, writer);
        }
        else
        {
            // Empty client ID: just write 0x0000 for length
            writer.writeUint16(0);
        }

        if (bHasWill)
        {
            m_willProperties.encode(writer);
            serialize::encodeString(m_willTopic, writer);
            serialize::encodeString(m_willMessage, writer);
        }

        if (bHasUsername)
        {
            serialize::encodeString(m_username, writer);
        }

        if (bHasPassword)
        {
            serialize::encodeString(m_password, writer);
        }
    }

    bool Connect<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Connect] MQTT 5");

        if (m_fixedHeader.getPacketType() != PacketType::Connect)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Connect", "[Connect] Invalid packet type: expected Connect, got %d",
                static_cast<int>(m_fixedHeader.getPacketType()));
            return false;
        }

        std::string protocolName;
        if (!serialize::decodeString(reader, protocolName))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode protocol name");
            return false;
        }

        if (protocolName != "MQTT")
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Connect", "[Connect] Invalid protocol name: %s", protocolName.c_str());
            return false;
        }

        uint8_t protocolLevel = reader.readUint8();
        if (constexpr uint8_t kExpectedProtocolLevel = 5; protocolLevel != kExpectedProtocolLevel)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error, "Connect", "[Connect] Invalid protocol version: %d (expected %d)", protocolLevel,
                kExpectedProtocolLevel);
            return false;
        }

        std::byte flags = static_cast<std::byte>(reader.readUint8());

        m_bCleanSession = (flags & kCleanSessionBit) != std::byte{};

        if (!reader.tryReadUint16(m_keepAliveSeconds))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to read keep alive");
            return false;
        }

        m_properties.decode(reader);

        if (!serialize::decodeString(reader, m_clientId))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode client ID");
            return false;
        }

        if ((flags & kWillFlagBit) != std::byte{})
        {
            auto qosByte = ((flags >> kWillQosShift) & kWillQosMask);
            m_willQos = static_cast<QualityOfService>(std::to_integer<uint8_t>(qosByte));

            m_bRetainWill = (flags & kWillRetainBit) != std::byte{};

            m_willProperties.decode(reader);

            if (!serialize::decodeString(reader, m_willTopic))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode will topic");
                return false;
            }

            if (!serialize::decodeString(reader, m_willMessage))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode will message");
                return false;
            }

            if (m_willTopic.empty())
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will topic is empty");
                return false;
            }

            if (m_willMessage.empty())
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will message is empty");
                return false;
            }
        }
        else
        {
            if (((flags >> kWillQosShift) & kWillQosMask) != std::byte{})
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will QoS must be 0 when will flag is 0");
                return false;
            }

            if ((flags & kWillRetainBit) != std::byte{})
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Will retain must be 0 when will flag is 0");
                return false;
            }
        }

        if ((flags & kUsernameBit) != std::byte{} && !serialize::decodeString(reader, m_username))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode username");
            return false;
        }

        if ((flags & kPasswordBit) != std::byte{} && !serialize::decodeString(reader, m_password))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode password");
            return false;
        }

        return true;
    }
} // namespace reactormq::mqtt::packets
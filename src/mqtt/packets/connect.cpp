//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "connect.h"

#include "util/logging/logging.h"

namespace reactormq::mqtt::packets
{
    using serialize::ByteReader;
    using serialize::ByteWriter;

    template<ProtocolVersion TProtocolVersion>
    Connect<TProtocolVersion>::Connect(
        std::string clientId,
        const uint16_t keepAliveSeconds,
        std::string username,
        std::string password,
        const bool cleanSession,
        const bool retainWill,
        std::string willTopic,
        std::string willMessage,
        const QualityOfService willQos)
        : IConnectPacket()
        , m_clientId(std::move(clientId))
        , m_keepAliveSeconds(keepAliveSeconds)
        , m_username(std::move(username))
        , m_password(std::move(password))
        , m_cleanSession(cleanSession)
        , m_retainWill(retainWill)
        , m_willTopic(std::move(willTopic))
        , m_willMessage(std::move(willMessage))
        , m_willQos(willQos)
    {
        setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    Connect<TProtocolVersion>::Connect(
        std::string clientId,
        const uint16_t keepAliveSeconds,
        std::string username,
        std::string password,
        const bool cleanSession,
        const bool retainWill,
        std::string willTopic,
        std::string willMessage,
        const QualityOfService willQos,
        typename detail::ConnectTraits<TProtocolVersion>::WillPropertiesType willProperties,
        typename detail::ConnectTraits<TProtocolVersion>::PropertiesType properties)
        requires(detail::ConnectTraits<TProtocolVersion>::HasProperties)
        : IConnectPacket()
        , m_clientId(std::move(clientId))
        , m_keepAliveSeconds(keepAliveSeconds)
        , m_username(std::move(username))
        , m_password(std::move(password))
        , m_cleanSession(cleanSession)
        , m_retainWill(retainWill)
        , m_willTopic(std::move(willTopic))
        , m_willMessage(std::move(willMessage))
        , m_willQos(willQos)
        , m_willProperties(std::move(willProperties))
        , m_properties(std::move(properties))
    {
        setFixedHeader(FixedHeader::create(this));
    }

    template<ProtocolVersion TProtocolVersion>
    Connect<TProtocolVersion>::Connect(ByteReader& reader, const FixedHeader& fixedHeader)
        : IConnectPacket(fixedHeader)
    {
        this->setIsValid(decode(reader));
    }

    template<ProtocolVersion TProtocolVersion>
    const std::string& Connect<TProtocolVersion>::getClientId() const
    {
        return m_clientId;
    }

    template<ProtocolVersion TProtocolVersion>
    uint16_t Connect<TProtocolVersion>::getKeepAliveSeconds() const
    {
        return m_keepAliveSeconds;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::string& Connect<TProtocolVersion>::getUsername() const
    {
        return m_username;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::string& Connect<TProtocolVersion>::getPassword() const
    {
        return m_password;
    }

    template<ProtocolVersion TProtocolVersion>
    bool Connect<TProtocolVersion>::getCleanSession() const
    {
        return m_cleanSession;
    }

    template<ProtocolVersion TProtocolVersion>
    bool Connect<TProtocolVersion>::getRetainWill() const
    {
        return m_retainWill;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::string& Connect<TProtocolVersion>::getWillTopic() const
    {
        return m_willTopic;
    }

    template<ProtocolVersion TProtocolVersion>
    const std::string& Connect<TProtocolVersion>::getWillMessage() const
    {
        return m_willMessage;
    }

    template<ProtocolVersion TProtocolVersion>
    QualityOfService Connect<TProtocolVersion>::getWillQos() const
    {
        return m_willQos;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::ConnectTraits<TProtocolVersion>::WillPropertiesType& Connect<TProtocolVersion>::getWillProperties() const
        requires(detail::ConnectTraits<TProtocolVersion>::HasProperties)
    {
        return m_willProperties;
    }

    template<ProtocolVersion TProtocolVersion>
    const typename detail::ConnectTraits<TProtocolVersion>::PropertiesType& Connect<TProtocolVersion>::getProperties() const
        requires(detail::ConnectTraits<TProtocolVersion>::HasProperties)
    {
        return m_properties;
    }

    template<ProtocolVersion TProtocolVersion>
    uint32_t Connect<TProtocolVersion>::getLength() const
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

        if constexpr (detail::ConnectTraits<TProtocolVersion>::HasProperties)
        {
            payloadLength += m_properties.getLength();

            if (!m_willTopic.empty())
            {
                payloadLength += m_willProperties.getLength();
            }
        }

        return kVariableHeaderLength + payloadLength;
    }

    template<ProtocolVersion TProtocolVersion>
    void Connect<TProtocolVersion>::encode(ByteWriter& writer) const
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Connect] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][Connect] MQTT 3.1.1");
        }

        this->getFixedHeader().encode(writer);

        serialize::encodeString("MQTT", writer);

        constexpr auto kProtocolLevel = static_cast<uint8_t>(TProtocolVersion);
        writer.writeUint8(kProtocolLevel);

        std::byte flags{};

        if (m_cleanSession)
        {
            flags |= kCleanSessionBit;
        }

        const bool hasWill = !m_willTopic.empty();
        if (hasWill)
        {
            const auto qosBits = (static_cast<std::byte>(static_cast<uint8_t>(m_willQos)) & kWillQosMask) << kWillQosShift;

            flags |= qosBits | kWillFlagBit;

            if (m_retainWill)
            {
                flags |= kWillRetainBit;
            }
        }

        const bool hasUsername = !m_username.empty();
        if (hasUsername)
        {
            flags |= kUsernameBit;
        }

        const bool hasPassword = !m_password.empty();
        if (hasPassword)
        {
            flags |= kPasswordBit;
        }

        writer.writeUint8(std::to_integer<uint8_t>(flags));
        writer.writeUint16(m_keepAliveSeconds);

        if constexpr (detail::ConnectTraits<TProtocolVersion>::HasProperties)
        {
            m_properties.encode(writer);
        }

        if (!m_clientId.empty())
        {
            serialize::encodeString(m_clientId, writer);
        }
        else
        {
            writer.writeUint16(0);
        }

        if (hasWill)
        {
            if constexpr (detail::ConnectTraits<TProtocolVersion>::HasProperties)
            {
                m_willProperties.encode(writer);
            }

            serialize::encodeString(m_willTopic, writer);
            serialize::encodeString(m_willMessage, writer);
        }

        if (hasUsername)
        {
            serialize::encodeString(m_username, writer);
        }

        if (hasPassword)
        {
            serialize::encodeString(m_password, writer);
        }
    }

    template<ProtocolVersion TProtocolVersion>
    bool Connect<TProtocolVersion>::decode(ByteReader& reader)
    {
        if constexpr (TProtocolVersion == ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Connect] MQTT 5");
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][Connect] MQTT 3.1.1");
        }

        if (this->getFixedHeader().getPacketType() != PacketType::Connect)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Connect] Invalid packet type: expected Connect, got %s",
                packetTypeToString(this->getFixedHeader().getPacketType()));
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
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Invalid protocol name: %s", protocolName.c_str());
            return false;
        }

        uint8_t protocolLevel = reader.readUint8();
        if (constexpr auto kExpectedProtocolLevel = static_cast<uint8_t>(TProtocolVersion); protocolLevel != kExpectedProtocolLevel)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "[Connect] Invalid protocol version: %d (expected %d)",
                protocolLevel,
                kExpectedProtocolLevel);
            return false;
        }

        const auto flags = static_cast<std::byte>(reader.readUint8());

        m_cleanSession = (flags & kCleanSessionBit) != std::byte{};

        if (!reader.tryReadUint16(m_keepAliveSeconds))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to read keep alive");
            return false;
        }

        if constexpr (detail::ConnectTraits<TProtocolVersion>::HasProperties)
        {
            m_properties.decode(reader);
        }

        if (!serialize::decodeString(reader, m_clientId))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[Connect] Failed to decode client ID");
            return false;
        }

        if ((flags & kWillFlagBit) != std::byte{})
        {
            const auto qosByte = ((flags >> kWillQosShift) & kWillQosMask);
            m_willQos = static_cast<QualityOfService>(std::to_integer<uint8_t>(qosByte));

            m_retainWill = (flags & kWillRetainBit) != std::byte{};

            if constexpr (detail::ConnectTraits<TProtocolVersion>::HasProperties)
            {
                m_willProperties.decode(reader);
            }

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

    template<ProtocolVersion V>
    void encodeConnectToWriter(
        ByteWriter& writer,
        const std::string& clientId,
        std::uint16_t keepAliveSeconds,
        const std::string& username,
        const std::string& password,
        bool cleanSession,
        const std::string& authMethod,
        const std::vector<std::uint8_t>& initialAuthData)
    {
        if constexpr (V == ProtocolVersion::V5)
        {
            properties::Properties connectProperties;

            if (!authMethod.empty())
            {
                std::vector<properties::Property> propList;
                propList.emplace_back(
                    properties::Property::create<properties::PropertyIdentifier::AuthenticationMethod, std::string>(authMethod));

                if (!initialAuthData.empty())
                {
                    propList.emplace_back(
                        properties::Property::create<properties::PropertyIdentifier::AuthenticationData, std::vector<std::uint8_t>>(
                            initialAuthData));
                }

                connectProperties = properties::Properties(propList);
            }

            properties::Properties willProps{};
            Connect<V> connectPacket(
                clientId,
                keepAliveSeconds,
                username,
                password,
                cleanSession,
                false,
                {},
                {},
                QualityOfService::AtMostOnce,
                std::move(willProps),
                std::move(connectProperties));
            connectPacket.encode(writer);
        }
        else
        {
            Connect<V>
                connectPacket(clientId, keepAliveSeconds, username, password, cleanSession, false, {}, {}, QualityOfService::AtMostOnce);
            connectPacket.encode(writer);
        }
    }

    template class Connect<ProtocolVersion::V311>;
    template class Connect<ProtocolVersion::V5>;

    template void encodeConnectToWriter<ProtocolVersion::V311>(
        ByteWriter&,
        const std::string&,
        std::uint16_t,
        const std::string&,
        const std::string&,
        bool,
        const std::string&,
        const std::vector<std::uint8_t>&);

    template void encodeConnectToWriter<ProtocolVersion::V5>(
        ByteWriter&,
        const std::string&,
        std::uint16_t,
        const std::string&,
        const std::string&,
        bool,
        const std::string&,
        const std::vector<std::uint8_t>&);
} // namespace reactormq::mqtt::packets
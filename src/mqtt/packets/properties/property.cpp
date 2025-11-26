//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "property.h"

#include "util/logging/logging.h"

#include <serialize/mqtt_codec.h>
#include <span>
#include <sstream>

namespace reactormq::mqtt::packets::properties
{
    using namespace reactormq::serialize;

    struct VariantEqualVisitor
    {
        template<class T, class U>
        bool operator()(const T&, const U&) const noexcept
        {
            return false;
        }

        template<class T>
        bool operator()(const T& a, const T& b) const noexcept(noexcept(a == b))
        {
            return a == b;
        }
    };

    bool operator==(
        const std::
            variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::string>>&
                lhs,
        const std::
            variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::string>>&
                rhs)
    {
        if (lhs.valueless_by_exception() && rhs.valueless_by_exception())
        {
            return true;
        }

        if (lhs.index() != rhs.index())
        {
            return false;
        }

        return std::visit(VariantEqualVisitor{}, lhs, rhs);
    }

    Property::Property(ByteReader& inReader)
        : m_data{ std::monostate{} }
        , m_identifier{ PropertyIdentifier::Unknown }
    {
        decode(inReader);
    }

    void Property::encode(ByteWriter& w) const
    {
        w.writeUint8(static_cast<uint8_t>(m_identifier));

        switch (m_identifier)
        {
            using enum PropertyIdentifier;
        case SubscriptionIdentifier:
            encodeVariableByteInteger(std::get<uint32_t>(m_data), w);
            break;

        case AuthenticationData:
        case CorrelationData:
            {
                const auto& data = std::get<std::vector<uint8_t>>(m_data);
                w.writeUint16(static_cast<uint16_t>(data.size()));
                w.writeBytes(reinterpret_cast<const std::byte*>(data.data()), data.size());
                break;
            }

        case ContentType:
        case ResponseTopic:
        case AssignedClientIdentifier:
        case AuthenticationMethod:
        case ResponseInformation:
        case ServerReference:
        case ReasonString:
            encodeString(std::get<std::string>(m_data), w);
            break;

        case ServerKeepAlive:
        case ReceiveMaximum:
        case TopicAliasMaximum:
        case TopicAlias:
            w.writeUint16(std::get<uint16_t>(m_data));
            break;

        case UserProperty:
            {
                const auto& [fst, snd] = std::get<std::pair<std::string, std::string>>(m_data);
                encodeString(fst, w);
                encodeString(snd, w);
                break;
            }

        case MessageExpiryInterval:
        case SessionExpiryInterval:
        case WillDelayInterval:
        case MaximumPacketSize:
            w.writeUint32(std::get<uint32_t>(m_data));
            break;

        case PayloadFormatIndicator:
        case RequestProblemInformation:
        case RequestResponseInformation:
        case MaximumQoS:
        case RetainAvailable:
        case WildcardSubscriptionAvailable:
        case SubscriptionIdentifierAvailable:
        case SharedSubscriptionAvailable:
            w.writeUint8(std::get<uint8_t>(m_data));
            break;

        case Max:
        case Unknown:
        default:
            REACTORMQ_LOG(logging::LogLevel::Warn, "Invalid property identifier in encode");
            break;
        }
    }

    bool Property::decode(ByteReader& r)
    {
        m_identifier = decodePropertyIdentifier(r);

        switch (m_identifier)
        {
            using enum PropertyIdentifier;
        case PayloadFormatIndicator:
        case RequestProblemInformation:
        case RequestResponseInformation:
        case MaximumQoS:
        case RetainAvailable:
        case WildcardSubscriptionAvailable:
        case SubscriptionIdentifierAvailable:
        case SharedSubscriptionAvailable:
            {
                uint8_t value = r.readUint8();
                m_data = value;
                break;
            }

        case ServerKeepAlive:
        case ReceiveMaximum:
        case TopicAliasMaximum:
        case TopicAlias:
            {
                uint16_t value = r.readUint16();
                m_data = value;
                break;
            }

        case MessageExpiryInterval:
        case SessionExpiryInterval:
        case WillDelayInterval:
        case MaximumPacketSize:
            {
                uint32_t value = r.readUint32();
                m_data = value;
                break;
            }

        case SubscriptionIdentifier:
            m_data = decodeVariableByteInteger(r);
            break;

        case CorrelationData:
        case AuthenticationData:
            {
                const uint16_t dataLength = r.readUint16();
                std::vector<uint8_t> value(dataLength);

                if (!value.empty())
                {
                    const auto dst = std::as_writable_bytes(std::span{ value });
                    r.readBytes(dst);
                }

                m_data = std::move(value);
                break;
            }

        case ContentType:
        case ResponseTopic:
        case AssignedClientIdentifier:
        case ResponseInformation:
        case AuthenticationMethod:
        case ServerReference:
        case ReasonString:
            {
                std::string value;
                if (!decodeString(r, value))
                {
                    REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to decode string property");
                    return false;
                }
                m_data = value;
                break;
            }

        case UserProperty:
            {
                std::string key;
                std::string val;
                if (!decodeString(r, key) || !decodeString(r, val))
                {
                    REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to decode user property");
                    return false;
                }
                m_data = std::make_pair(key, val);
                break;
            }

        case Max:
        case Unknown:
            m_data = std::monostate{};
            REACTORMQ_LOG(logging::LogLevel::Warn, "Invalid property identifier in decode");
            return false;
        }

        return true;
    }

    uint32_t Property::getLength() const
    {
        switch (m_identifier)
        {
            using enum PropertyIdentifier;
        case PayloadFormatIndicator:
        case RequestProblemInformation:
        case RequestResponseInformation:
        case MaximumQoS:
        case RetainAvailable:
        case WildcardSubscriptionAvailable:
        case SubscriptionIdentifierAvailable:
        case SharedSubscriptionAvailable:
            return sizeof(uint8_t) + 1; // 1 byte for identifier + 1 byte for value

        case ServerKeepAlive:
        case ReceiveMaximum:
        case TopicAliasMaximum:
        case TopicAlias:
            return sizeof(uint16_t) + 1; // 1 byte for identifier + 2 bytes for value

        case MessageExpiryInterval:
        case SessionExpiryInterval:
        case WillDelayInterval:
        case MaximumPacketSize:
            return sizeof(uint32_t) + 1; // 1 byte for identifier + 4 bytes for value

        case SubscriptionIdentifier:
            return variableByteIntegerSize(std::get<uint32_t>(m_data)) + 1; // 1 byte id + VBI data

        case CorrelationData:
        case AuthenticationData:
            return static_cast<uint32_t>(sizeof(uint16_t) + std::get<std::vector<uint8_t>>(m_data).size() + 1);

        case ContentType:
        case ResponseTopic:
        case AssignedClientIdentifier:
        case AuthenticationMethod:
        case ResponseInformation:
        case ServerReference:
        case ReasonString:
            return static_cast<uint32_t>(sizeof(uint16_t) + std::get<std::string>(m_data).size() + 1); // +1 id

        case UserProperty:
            {
                const auto& [fst, snd] = std::get<std::pair<std::string, std::string>>(m_data);
                return static_cast<uint32_t>(sizeof(uint16_t) + fst.size() + sizeof(uint16_t) + snd.size() + 1);
                // +1 id
            }

        case Max:
        case Unknown:
        default:
            REACTORMQ_LOG(logging::LogLevel::Warn, "Invalid property identifier in getLength");
            return 0;
        }
    }

    bool Property::tryGetValue(uint8_t& outValue) const
    {
        if (std::holds_alternative<uint8_t>(m_data))
        {
            outValue = std::get<uint8_t>(m_data);
            return true;
        }
        return false;
    }

    bool Property::tryGetValue(uint16_t& outValue) const
    {
        if (std::holds_alternative<uint16_t>(m_data))
        {
            outValue = std::get<uint16_t>(m_data);
            return true;
        }
        return false;
    }

    bool Property::tryGetValue(uint32_t& outValue) const
    {
        if (std::holds_alternative<uint32_t>(m_data))
        {
            outValue = std::get<uint32_t>(m_data);
            return true;
        }
        return false;
    }

    bool Property::tryGetValue(std::vector<uint8_t>& outValue) const
    {
        if (std::holds_alternative<std::vector<uint8_t>>(m_data))
        {
            outValue = std::get<std::vector<uint8_t>>(m_data);
            return true;
        }
        return false;
    }

    bool Property::tryGetValue(std::string& outValue) const
    {
        if (std::holds_alternative<std::string>(m_data))
        {
            outValue = std::get<std::string>(m_data);
            return true;
        }
        return false;
    }

    bool Property::tryGetValue(std::pair<std::string, std::string>& outValue) const
    {
        if (std::holds_alternative<std::pair<std::string, std::string>>(m_data))
        {
            outValue = std::get<std::pair<std::string, std::string>>(m_data);
            return true;
        }
        return false;
    }

    std::string Property::toString() const
    {
        if (std::holds_alternative<uint8_t>(m_data))
        {
            return std::to_string(std::get<uint8_t>(m_data));
        }

        if (std::holds_alternative<uint16_t>(m_data))
        {
            return std::to_string(std::get<uint16_t>(m_data));
        }

        if (std::holds_alternative<uint32_t>(m_data))
        {
            return std::to_string(std::get<uint32_t>(m_data));
        }

        if (std::holds_alternative<std::vector<uint8_t>>(m_data))
        {
            std::ostringstream oss;
            for (const auto& vec = std::get<std::vector<uint8_t>>(m_data); const auto& byte : vec)
            {
                oss << static_cast<int>(byte);
            }
            return oss.str();
        }

        if (std::holds_alternative<std::string>(m_data))
        {
            return std::get<std::string>(m_data);
        }

        if (std::holds_alternative<std::pair<std::string, std::string>>(m_data))
        {
            const auto& [fst, snd] = std::get<std::pair<std::string, std::string>>(m_data);
            return fst + ", " + snd;
        }

        return "Unknown";
    }

    PropertyIdentifier Property::decodePropertyIdentifier(ByteReader& reader)
    {
        uint8_t intCode = reader.readUint8();

        if (!isValidPropertyIdentifier(intCode))
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "Invalid property identifier: %u", intCode);
            return PropertyIdentifier::Unknown;
        }

        return static_cast<PropertyIdentifier>(intCode);
    }
} // namespace reactormq::mqtt::packets::properties
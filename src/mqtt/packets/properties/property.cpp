#include "property.h"

#include "util/logging/logging.h"

#include <sstream>
#include <serialize/mqtt_codec.h>

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
        const std::variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::
            string>>& lhs,
        const std::variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::
            string>>& rhs)
    {
        if (lhs.valueless_by_exception() && rhs.valueless_by_exception())
            return true;

        if (lhs.index() != rhs.index())
            return false;

        return std::visit(VariantEqualVisitor{}, lhs, rhs);
    }

    Property::Property(ByteReader& inReader)
        : m_data{ std::monostate{} }, m_identifier{ PropertyIdentifier::Unknown }
    {
        decode(inReader);
    }

    void Property::encode(ByteWriter& w) const
    {
        w.writeUint8(static_cast<uint8_t>(m_identifier));

        switch (m_identifier)
        {
        case PropertyIdentifier::SubscriptionIdentifier:
            encodeVariableByteInteger(std::get<uint32_t>(m_data), w);
            break;

        case PropertyIdentifier::AuthenticationData:
        case PropertyIdentifier::CorrelationData: {
            const auto& data = std::get<std::vector<uint8_t>>(m_data);
            w.writeUint16(static_cast<uint16_t>(data.size()));
            w.writeBytes(reinterpret_cast<const std::byte*>(data.data()), data.size());
            break;
        }

        case PropertyIdentifier::ContentType:
        case PropertyIdentifier::ResponseTopic:
        case PropertyIdentifier::AssignedClientIdentifier:
        case PropertyIdentifier::AuthenticationMethod:
        case PropertyIdentifier::ResponseInformation:
        case PropertyIdentifier::ServerReference:
        case PropertyIdentifier::ReasonString:
            encodeString(std::get<std::string>(m_data), w);
            break;

        case PropertyIdentifier::ServerKeepAlive:
        case PropertyIdentifier::ReceiveMaximum:
        case PropertyIdentifier::TopicAliasMaximum:
        case PropertyIdentifier::TopicAlias:
            w.writeUint16(std::get<uint16_t>(m_data));
            break;

        case PropertyIdentifier::UserProperty: {
            const auto& [fst, snd] = std::get<std::pair<std::string, std::string>>(m_data);
            encodeString(fst, w);
            encodeString(snd, w);
            break;
        }

        case PropertyIdentifier::MessageExpiryInterval:
        case PropertyIdentifier::SessionExpiryInterval:
        case PropertyIdentifier::WillDelayInterval:
        case PropertyIdentifier::MaximumPacketSize:
            w.writeUint32(std::get<uint32_t>(m_data));
            break;

        case PropertyIdentifier::PayloadFormatIndicator:
        case PropertyIdentifier::RequestProblemInformation:
        case PropertyIdentifier::RequestResponseInformation:
        case PropertyIdentifier::MaximumQoS:
        case PropertyIdentifier::RetainAvailable:
        case PropertyIdentifier::WildcardSubscriptionAvailable:
        case PropertyIdentifier::SubscriptionIdentifierAvailable:
        case PropertyIdentifier::SharedSubscriptionAvailable:
            w.writeUint8(std::get<uint8_t>(m_data));
            break;

        case PropertyIdentifier::Max:
        case PropertyIdentifier::Unknown:
        default:
            REACTORMQ_LOG(LogLevel::Warn, "Invalid property identifier in encode");
            break;
        }
    }

    bool Property::decode(ByteReader& r)
    {
        m_identifier = decodePropertyIdentifier(r);

        switch (m_identifier)
        {
        case PropertyIdentifier::PayloadFormatIndicator:
        case PropertyIdentifier::RequestProblemInformation:
        case PropertyIdentifier::RequestResponseInformation:
        case PropertyIdentifier::MaximumQoS:
        case PropertyIdentifier::RetainAvailable:
        case PropertyIdentifier::WildcardSubscriptionAvailable:
        case PropertyIdentifier::SubscriptionIdentifierAvailable:
        case PropertyIdentifier::SharedSubscriptionAvailable: {
            uint8_t value = r.readUint8();
            m_data = value;
            break;
        }

        case PropertyIdentifier::ServerKeepAlive:
        case PropertyIdentifier::ReceiveMaximum:
        case PropertyIdentifier::TopicAliasMaximum:
        case PropertyIdentifier::TopicAlias: {
            uint16_t value = r.readUint16();
            m_data = value;
            break;
        }

        case PropertyIdentifier::MessageExpiryInterval:
        case PropertyIdentifier::SessionExpiryInterval:
        case PropertyIdentifier::WillDelayInterval:
        case PropertyIdentifier::MaximumPacketSize: {
            uint32_t value = r.readUint32();
            m_data = value;
            break;
        }

        case PropertyIdentifier::SubscriptionIdentifier:
            m_data = decodeVariableByteInteger(r);
            break;

        case PropertyIdentifier::CorrelationData:
        case PropertyIdentifier::AuthenticationData: {
            const uint16_t dataLength = r.readUint16();
            std::vector<uint8_t> value(dataLength);
            r.readBytes(reinterpret_cast<std::byte*>(value.data()), dataLength);
            m_data = value;
            break;
        }

        case PropertyIdentifier::ContentType:
        case PropertyIdentifier::ResponseTopic:
        case PropertyIdentifier::AssignedClientIdentifier:
        case PropertyIdentifier::ResponseInformation:
        case PropertyIdentifier::AuthenticationMethod:
        case PropertyIdentifier::ServerReference:
        case PropertyIdentifier::ReasonString: {
            std::string value;
            if (!decodeString(r, value))
            {
                REACTORMQ_LOG(
                    reactormq::logging::LogLevel::Error, "Property",
                    "Failed to decode string property");
                return false;
            }
            m_data = value;
            break;
        }

        case PropertyIdentifier::UserProperty: {
            std::string key, val;
            if (!decodeString(r, key) || !decodeString(r, val))
            {
                REACTORMQ_LOG(
                    reactormq::logging::LogLevel::Error, "Property",
                    "Failed to decode user property");
                return false;
            }
            m_data = std::make_pair(key, val);
            break;
        }

        case PropertyIdentifier::Max:
        case PropertyIdentifier::Unknown:
            m_data = std::monostate{};
            REACTORMQ_LOG(
                LogLevel::Warn, "Property",
                "Invalid property identifier in decode");
            return false;
        }

        return true;
    }

    uint32_t Property::getLength() const
    {
        switch (m_identifier)
        {
        case PropertyIdentifier::PayloadFormatIndicator:
        case PropertyIdentifier::RequestProblemInformation:
        case PropertyIdentifier::RequestResponseInformation:
        case PropertyIdentifier::MaximumQoS:
        case PropertyIdentifier::RetainAvailable:
        case PropertyIdentifier::WildcardSubscriptionAvailable:
        case PropertyIdentifier::SubscriptionIdentifierAvailable:
        case PropertyIdentifier::SharedSubscriptionAvailable:
            return sizeof(uint8_t) + 1; // 1 byte for identifier + 1 byte for value

        case PropertyIdentifier::ServerKeepAlive:
        case PropertyIdentifier::ReceiveMaximum:
        case PropertyIdentifier::TopicAliasMaximum:
        case PropertyIdentifier::TopicAlias:
            return sizeof(uint16_t) + 1; // 1 byte for identifier + 2 bytes for value

        case PropertyIdentifier::MessageExpiryInterval:
        case PropertyIdentifier::SessionExpiryInterval:
        case PropertyIdentifier::WillDelayInterval:
        case PropertyIdentifier::MaximumPacketSize:
            return sizeof(uint32_t) + 1; // 1 byte for identifier + 4 bytes for value

        case PropertyIdentifier::SubscriptionIdentifier:
            return variableByteIntegerSize(std::get<uint32_t>(m_data)) + 1; // 1 byte id + VBI data

        case PropertyIdentifier::CorrelationData:
        case PropertyIdentifier::AuthenticationData:
            return static_cast<uint32_t>(sizeof(uint16_t) + std::get<std::vector<uint8_t>>(m_data).size() + 1);

        case PropertyIdentifier::ContentType:
        case PropertyIdentifier::ResponseTopic:
        case PropertyIdentifier::AssignedClientIdentifier:
        case PropertyIdentifier::AuthenticationMethod:
        case PropertyIdentifier::ResponseInformation:
        case PropertyIdentifier::ServerReference:
        case PropertyIdentifier::ReasonString:
            return static_cast<uint32_t>(sizeof(uint16_t) + std::get<std::string>(m_data).size() + 1); // +1 id

        case PropertyIdentifier::UserProperty: {
            const auto& [fst, snd] = std::get<std::pair<std::string, std::string>>(m_data);
            return static_cast<uint32_t>(sizeof(uint16_t) + fst.size() +
                sizeof(uint16_t) + snd.size() + 1); // +1 id
        }

        case PropertyIdentifier::Max:
        case PropertyIdentifier::Unknown:
        default:
            REACTORMQ_LOG(
                LogLevel::Warn, "Property",
                "Invalid property identifier in getLength");
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
            REACTORMQ_LOG(
                LogLevel::Warn, "Property",
                "Invalid property identifier: %u", intCode);
            return PropertyIdentifier::Unknown;
        }

        return static_cast<PropertyIdentifier>(intCode);
    }
} // namespace reactormq::mqtt::packets::properties
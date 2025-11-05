#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"
#include "util/logging/logging.h"
#include <string>
#include <utility>
#include <vector>
#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT PUBLISH packets.
     * 
     * PUBLISH is used to transport application messages from client to server or server to client.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901100
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718037
     */
    class PublishBase : public TControlPacket<PacketType::Publish>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit PublishBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
              , m_packetIdentifier(0)
        {
        }

        /**
         * @brief Constructor with payload, topic, and packet identifier.
         * @param payload The message payload.
         * @param topicName The topic name.
         * @param packetIdentifier The packet identifier (for QoS > 0).
         */
        explicit PublishBase(
            std::vector<uint8_t> payload,
            std::string topicName,
            const uint16_t packetIdentifier)
            : m_topicName(std::move(topicName))
              , m_packetIdentifier(packetIdentifier)
              , m_payload(std::move(payload))
        {
        }

        /**
         * @brief Get the is duplicate flag.
         * @return The is duplicate flag.
         */
        [[nodiscard]] bool getIsDuplicate() const
        {
            return getQualityOfService() != QualityOfService::AtMostOnce
                   && (m_fixedHeader.getFlags() & 0x08) == 0x08;
        }

        /**
         * @brief Get the should retain flag.
         * @return The should retain flag.
         */
        [[nodiscard]] bool getShouldRetain() const
        {
            return (m_fixedHeader.getFlags() & 0x01) == 0x01;
        }

        /**
         * @brief Get the quality of service.
         * @return The quality of service.
         */
        [[nodiscard]] QualityOfService getQualityOfService() const
        {
            return static_cast<QualityOfService>((m_fixedHeader.getFlags() & 0x06) >> 1);
        }

        /**
         * @brief Get the topic name.
         * @return The topic name.
         */
        [[nodiscard]] const std::string& getTopicName() const
        {
            return m_topicName;
        }

        /**
         * @brief Get the packet identifier.
         * @return The packet identifier.
         */
        [[nodiscard]] uint16_t getPacketId() const override
        {
            return m_packetIdentifier;
        }

        /**
         * @brief Get the payload.
         * @return The payload.
         */
        [[nodiscard]] const std::vector<uint8_t>& getPayload() const
        {
            return m_payload;
        }

    protected:
        std::string m_topicName;
        uint16_t m_packetIdentifier;
        std::vector<uint8_t> m_payload;
    };

    template<ProtocolVersion TProtocolVersion>
    class Publish;

    /**
     * @brief MQTT 3.1.1 PUBLISH packet.
     * 
     */
    template<>
    class Publish<ProtocolVersion::V311> final : public PublishBase
    {
    public:
        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Publish(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : PublishBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Constructor with topic, payload, QoS, retain, and packet identifier.
         * @param topicName The topic name.
         * @param payload The message payload.
         * @param qos Quality of service level.
         * @param shouldRetain Whether to retain the message.
         * @param packetIdentifier The packet identifier (required if QoS > 0).
         * @param isDuplicate Whether this is a duplicate message.
         */
        explicit Publish(
            std::string topicName,
            std::vector<uint8_t> payload,
            const QualityOfService qos,
            const bool shouldRetain,
            const uint16_t packetIdentifier = 0,
            const bool isDuplicate = false)
            : PublishBase(std::move(payload), std::move(topicName), packetIdentifier)
        {
            const uint32_t remainingLength = getLength(
                static_cast<uint32_t>(m_topicName.length()),
                static_cast<uint32_t>(m_payload.size()),
                qos);

            m_fixedHeader = FixedHeader::create(
                this,
                remainingLength,
                shouldRetain,
                qos,
                isDuplicate);
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * Static version with parameters.
         * @param topicNameLength Length of the topic name.
         * @param payloadLength Length of the payload.
         * @param qos Quality of service level.
         * @return The length in bytes.
         */
        static uint32_t getLength(
            const uint32_t topicNameLength,
            const uint32_t payloadLength,
            const QualityOfService qos)
        {
            uint32_t length = payloadLength + kStringLengthFieldSize + topicNameLength;

            if (static_cast<uint8_t>(qos) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
            {
                length += 2; // 2 bytes for packet identifier
            }

            return length;
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return getLength(
                static_cast<uint32_t>(m_topicName.length()),
                static_cast<uint32_t>(m_payload.size()),
                getQualityOfService());
        }

        /**
         * @brief Encode the packet to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;
    };

    /**
     * @brief MQTT 5 PUBLISH packet.
     * 
     */
    template<>
    class Publish<ProtocolVersion::V5> final : public PublishBase
    {
    public:
        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Publish(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : PublishBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Constructor with topic, payload, QoS, retain, packet identifier, and properties.
         * @param topicName The topic name.
         * @param payload The message payload.
         * @param qos Quality of service level.
         * @param shouldRetain Whether to retain the message.
         * @param packetIdentifier The packet identifier (required if QoS > 0).
         * @param properties MQTT 5 properties.
         * @param isDuplicate Whether this is a duplicate message.
         */
        explicit Publish(
            std::string topicName,
            std::vector<uint8_t> payload,
            const QualityOfService qos,
            const bool shouldRetain,
            const uint16_t packetIdentifier = 0,
            properties::Properties properties = properties::Properties{},
            const bool isDuplicate = false)
            : PublishBase(std::move(payload), std::move(topicName), packetIdentifier)
              , m_properties(std::move(properties))
        {
            const uint32_t remainingLength = getLength(
                static_cast<uint32_t>(m_topicName.length()),
                static_cast<uint32_t>(m_payload.size()),
                m_properties.getLength(),
                qos);

            m_fixedHeader = FixedHeader::create(
                this,
                remainingLength,
                shouldRetain,
                qos,
                isDuplicate);
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * Static version with parameters.
         * @param topicNameLength Length of the topic name.
         * @param payloadLength Length of the payload.
         * @param propertiesLength Length of the properties section.
         * @param qos Quality of service level.
         * @return The length in bytes.
         */
        static uint32_t getLength(
            const uint32_t topicNameLength,
            const uint32_t payloadLength,
            const uint32_t propertiesLength,
            const QualityOfService qos)
        {
            uint32_t length = kStringLengthFieldSize + topicNameLength + propertiesLength + payloadLength;

            if (static_cast<uint8_t>(qos) > static_cast<uint8_t>(QualityOfService::AtMostOnce))
            {
                length += sizeof(uint16_t); // 2 bytes for packet identifier
            }

            return length;
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return getLength(
                static_cast<uint32_t>(m_topicName.length()),
                static_cast<uint32_t>(m_payload.size()),
                m_properties.getLength(),
                getQualityOfService());
        }

        /**
         * @brief Get the properties.
         * @return The properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
        }

        /**
         * @brief Encode the packet to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;

    protected:
        properties::Properties m_properties;
    };

    using Publish3 = Publish<ProtocolVersion::V311>;
    using Publish5 = Publish<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

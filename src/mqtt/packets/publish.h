//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"

#include <cstddef>
#include <string>
#include <vector>

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        template<ProtocolVersion V>
        struct PublishTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 PUBLISH packets.
         */
        template<>
        struct PublishTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            using PropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 PUBLISH packets.
         */
        template<>
        struct PublishTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            using PropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT PUBLISH packets.
     */
    class IPublishPacket : public TControlPacket<PacketType::Publish>
    {
    public:
        IPublishPacket() = default;

        using TControlPacket::TControlPacket;

        /**
         * @brief Get the is duplicate flag.
         * @return The is duplicate flag.
         */
        [[nodiscard]] virtual bool getIsDuplicate() const = 0;

        /**
         * @brief Get the should retain flag.
         * @return The should retain flag.
         */
        [[nodiscard]] virtual bool getShouldRetain() const = 0;

        /**
         * @brief Get the quality of service.
         * @return The quality of service.
         */
        [[nodiscard]] virtual QualityOfService getQualityOfService() const = 0;

        /**
         * @brief Get the topic name.
         * @return The topic name.
         */
        [[nodiscard]] virtual const std::string& getTopicName() const = 0;

        /**
         * @brief Get the payload.
         * @return The payload.
         */
        [[nodiscard]] virtual const std::vector<uint8_t>& getPayload() const = 0;
    };

    /**
     * @brief MQTT PUBLISH packet for a specific protocol version.
     *
     * PUBLISH is used to transport application messages from client to server or server to client.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901100
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718037
     */
    template<ProtocolVersion TProtocolVersion>
    class Publish final : public IPublishPacket
    {
        using Traits = detail::PublishTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;

    public:
        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Publish(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

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
            QualityOfService qos,
            bool shouldRetain,
            uint16_t packetIdentifier = 0,
            bool isDuplicate = false);

        /**
         * @brief Constructor with topic, payload, QoS, retain, packet identifier, and properties.
         * @tparam V Protocol version.
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
            QualityOfService qos,
            bool shouldRetain,
            uint16_t packetIdentifier,
            const typename Traits::PropertiesType& properties,
            bool isDuplicate = false)
            requires(detail::PublishTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Get the is duplicate flag.
         * @return The is duplicate flag.
         */
        [[nodiscard]] bool getIsDuplicate() const override;

        /**
         * @brief Get the should retain flag.
         * @return The should retain flag.
         */
        [[nodiscard]] bool getShouldRetain() const override;

        /**
         * @brief Get the quality of service.
         * @return The quality of service.
         */
        [[nodiscard]] QualityOfService getQualityOfService() const override;

        /**
         * @brief Get the topic name.
         * @return The topic name.
         */
        [[nodiscard]] const std::string& getTopicName() const override;

        /**
         * @brief Get the packet identifier.
         * @return The packet identifier.
         */
        [[nodiscard]] uint16_t getPacketId() const override;

        /**
         * @brief Get the payload.
         * @return The payload.
         */
        [[nodiscard]] const std::vector<uint8_t>& getPayload() const override;

        /**
         * @brief Get the properties for MQTT 5 PUBLISH packets.
         * @return The properties.
         */
        [[nodiscard]] const typename Traits::PropertiesType& getProperties() const
            requires(detail::PublishTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override;

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

        /**
         * @brief Get the length of the packet payload (remaining length).
         * Static version with parameters.
         * @param topicNameLength Length of the topic name.
         * @param payloadLength Length of the payload.
         * @param propertiesLength Length of the properties section.
         * @param qos Quality of service level.
         * @return The length in bytes.
         */
        static uint32_t getLength(uint32_t topicNameLength, uint32_t payloadLength, uint32_t propertiesLength, QualityOfService qos);

        [[nodiscard]] PacketType getPacketType() const override
        {
            return PacketType::Publish;
        }

    private:
        void setPayload(std::vector<uint8_t>&& payload);

        void setPacketId(uint16_t packetId);

        void setTopicName(std::string&& topicName);

        void readPayload(serialize::ByteReader reader, std::vector<uint8_t>::size_type payloadSize);

        [[nodiscard]] uint32_t getPropertiesLength() const;

        std::string m_topicName;
        uint16_t m_packetIdentifier{};
        std::vector<uint8_t> m_payload;
        PropertiesStorage m_properties{};

        static constexpr std::byte kRetainBit{ std::byte{ 0x1 } << 0 };
        static constexpr std::byte kDupBit{ std::byte{ 0x1 } << 3 };
        static constexpr std::byte kQosMask{ std::byte{ 0x3 } };
        static constexpr int kQosShift{ 1 };
    };

    /**
     * @brief Encode a PUBLISH packet to the writer.
     * @tparam V Protocol version.
     * @param writer Writer to encode to.
     * @param topic Topic name.
     * @param payload Message payload.
     * @param qos Quality of Service.
     * @param shouldRetain Retain flag.
     * @param packetId Packet identifier.
     * @param isDuplicate Duplicate flag.
     */
    template<ProtocolVersion V>
    void encodePublishToWriter(
        serialize::ByteWriter& writer,
        const std::string& topic,
        const std::vector<uint8_t>& payload,
        QualityOfService qos,
        bool shouldRetain,
        std::uint16_t packetId,
        bool isDuplicate);

    /**
     * @brief Alias for MQTT 3.1.1 PUBLISH packet.
     */
    using Publish3 = Publish<ProtocolVersion::V311>;

    /**
     * @brief Alias for MQTT 5 PUBLISH packet.
     */
    using Publish5 = Publish<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets
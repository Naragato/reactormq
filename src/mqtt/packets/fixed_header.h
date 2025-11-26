//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/interface/control_packet.h"
#include "mqtt/packets/packet_type.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"
#include "util/logging/logging.h"

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Fixed header for an MQTT control packet.
     * Encodes/decodes the first byte and Remaining Length per MQTT 3.1.1 and 5.0.
     */
    class FixedHeader
    {
    public:
        FixedHeader()
            : m_flags(0)
            , m_remainingLength(0)
        {
        }

        /**
         * @brief Remaining Length from the fixed header.
         * @return Payload length in bytes.
         */
        [[nodiscard]] uint32_t getRemainingLength() const
        {
            return m_remainingLength;
        }

        /**
         * @brief Packet type extracted from the header flags.
         * @return PacketType value.
         */
        [[nodiscard]] PacketType getPacketType() const
        {
            const auto flags = static_cast<std::byte>(m_flags);
            return static_cast<PacketType>(std::to_integer<uint8_t>((flags & kTypeMask) >> kTypeShift));
        }

        /**
         * @brief Raw header flags byte.
         * @return Flags value.
         */
        [[nodiscard]] uint8_t getFlags() const
        {
            return m_flags;
        }

        /**
         * @brief Parse a fixed header from a reader.
         * @param reader ByteReader positioned at the header.
         * @return Parsed fixed header.
         */
        static FixedHeader create(serialize::ByteReader& reader)
        {
            FixedHeader header;
            header.decode(reader);
            return header;
        }

        /**
         * @brief Build a fixed header from a packet using default flags.
         * @param packet Packet to inspect.
         * @return New fixed header.
         */
        static FixedHeader create(const IControlPacket* packet)
        {
            using enum PacketType;
            if (packet == nullptr)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Control packet is null");
                return {};
            }

            auto flags = static_cast<std::byte>(packet->getPacketType()) << kTypeShift;
            const uint32_t remainingLength = packet->getLength();

            if (packet->getPacketType() == PubRel || packet->getPacketType() == Subscribe || packet->getPacketType() == Unsubscribe)
            {
                flags |= kPubRelSubUnsubFlags;
            }

            return { std::to_integer<uint8_t>(flags), remainingLength };
        }

        /**
         * @brief Build a fixed header from a packet with explicit flags.
         * @param packet Source packet.
         * @param remainingLength Body size in bytes (excludes fixed header).
         * @param shouldRetain RETAIN flag.
         * @param qos QoS bits.
         * @param isDuplicated DUP flag.
         * @return New fixed header.
         */
        static FixedHeader create(
            const IControlPacket* packet,
            const uint32_t remainingLength,
            const bool shouldRetain,
            const QualityOfService qos,
            const bool isDuplicated)
        {
            if (packet == nullptr)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Control packet is null");
                return {};
            }

            auto flags = static_cast<std::byte>(packet->getPacketType()) << kTypeShift;

            if (shouldRetain)
            {
                flags |= kRetainBit;
            }

            const auto qosBits = (static_cast<std::byte>(static_cast<uint8_t>(qos)) & kQosMask) << kQosShift;
            flags |= qosBits;

            if (isDuplicated && qos != QualityOfService::AtMostOnce)
            {
                flags |= kDupBit;
            }

            return { std::to_integer<uint8_t>(flags), remainingLength };
        }

        /**
         * @brief Encode the fixed header to a writer.
         * @param writer Writer for serialization.
         */
        void encode(const serialize::ByteWriter& writer) const
        {
            writer.writeUint8(m_flags);
            serialize::encodeVariableByteInteger(m_remainingLength, writer);
        }

        /**
         * @brief Decode the fixed header from a reader.
         * @param reader Reader for deserialization.
         */
        void decode(serialize::ByteReader& reader)
        {
            uint8_t tempFlags = 0;
            if (!reader.tryReadUint8(tempFlags))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read flags byte");
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            const auto temp = static_cast<std::byte>(tempFlags);
            const auto rawPacketType = std::to_integer<uint8_t>((temp & kTypeMask) >> kTypeShift);

            if (constexpr auto maxAllowedType = static_cast<uint8_t>(PacketType::Auth); rawPacketType > maxAllowedType)
            {
                const std::bitset<8> bits(tempFlags);
                REACTORMQ_LOG(logging::LogLevel::Error, "FixedHeader Invalid packet type. Flags: 0b%s", bits.to_string().c_str());
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            if (const auto packetType = static_cast<PacketType>(rawPacketType); packetType == PacketType::None)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Invalid packet type: None");
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            m_remainingLength = serialize::decodeVariableByteInteger(reader);

            if (const uint8_t remainingLengthFieldSize = serialize::variableByteIntegerSize(m_remainingLength);
                remainingLengthFieldSize == 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "FixedHeader Invalid remaining length: %u", m_remainingLength);
                m_flags = 0;
                m_remainingLength = 0;
                return;
            }

            m_flags = tempFlags;
        }

    private:
        /**
         * @brief Private constructor for creating fixed headers with specific values.
         * @param flags Packet-specific flags.
         * @param remainingLength Length of the remaining payload.
         */
        FixedHeader(const uint8_t flags, const uint32_t remainingLength)
            : m_flags(flags)
            , m_remainingLength(remainingLength)
        {
        }

        static constexpr std::byte kTypeMask{ std::byte{ 0xF0 } };
        static constexpr int kTypeShift{ 4 };
        static constexpr std::byte kRetainBit{ std::byte{ 0x1 } << 0 };
        static constexpr std::byte kDupBit{ std::byte{ 0x1 } << 3 };
        static constexpr std::byte kQosMask{ std::byte{ 0x3 } };
        static constexpr int kQosShift{ 1 };
        static constexpr std::byte kPubRelSubUnsubFlags{ std::byte{ 0x02 } };
        uint8_t m_flags;
        uint32_t m_remainingLength;
    };
} // namespace reactormq::mqtt::packets
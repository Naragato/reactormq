//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/client/mqtt_version_mapping.h"

#include "mqtt/packets/auth.h"
#include "mqtt/packets/ping_req.h"
#include "mqtt/packets/ping_resp.h"

#include <reactormq/mqtt/message.h>

namespace reactormq::mqtt::client
{
    template<packets::ProtocolVersion V>
    std::unique_ptr<packets::IControlPacket> parsePacketImpl(
        serialize::ByteReader& reader, const packets::FixedHeader& fixedHeader, const packets::PacketType packetType)
    {
        using Mapping = MqttVersionMapping<V>;

        switch (packetType)
        {
            using enum packets::PacketType;
        case Connect:
            {
                return std::make_unique<typename Mapping::Connect>(reader, fixedHeader);
            }
        case ConnAck:
            {
                return std::make_unique<typename Mapping::ConnAck>(reader, fixedHeader);
            }
        case Publish:
            {
                return std::make_unique<typename Mapping::Publish>(reader, fixedHeader);
            }
        case PubAck:
            {
                return std::make_unique<typename Mapping::PubAck>(reader, fixedHeader);
            }
        case PubRec:
            {
                return std::make_unique<typename Mapping::PubRec>(reader, fixedHeader);
            }
        case PubRel:
            {
                return std::make_unique<typename Mapping::PubRel>(reader, fixedHeader);
            }
        case PubComp:
            {
                return std::make_unique<typename Mapping::PubComp>(reader, fixedHeader);
            }
        case Subscribe:
            {
                return std::make_unique<typename Mapping::Subscribe>(reader, fixedHeader);
            }
        case SubAck:
            {
                return std::make_unique<typename Mapping::SubAck>(reader, fixedHeader);
            }
        case Unsubscribe:
            {
                return std::make_unique<typename Mapping::Unsubscribe>(reader, fixedHeader);
            }
        case UnsubAck:
            {
                return std::make_unique<typename Mapping::UnsubAck>(reader, fixedHeader);
            }
        case PingReq:
            {
                return std::make_unique<packets::PingReq>(fixedHeader);
            }
        case PingResp:
            {
                return std::make_unique<packets::PingResp>(fixedHeader);
            }
        case Disconnect:
            {
                return std::make_unique<typename Mapping::Disconnect>(reader, fixedHeader);
            }
        case Auth:
            {
                if constexpr (Mapping::supportsAuth())
                {
                    return std::make_unique<packets::Auth>(reader, fixedHeader);
                }
                else
                {
                    return nullptr;
                }
            }
        case None:
        case Max:
        default:
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Unknown or invalid packet type: %d", packetType);
                return nullptr;
            }
        }
    }

    void PublishEncoder<packets::ProtocolVersion::V311>::encode(
        Message const& message, const std::uint16_t packetId, serialize::ByteWriter& writer)
    {
        using Mapping = MqttVersionMapping<packets::ProtocolVersion::V311>;

        const Mapping::Publish publishPacket(
            message.getTopic(),
            message.getPayload(),
            message.getQualityOfService(),
            message.shouldRetain(),
            packetId,
            true // dup
        );

        publishPacket.encode(writer);
    }

    void PublishEncoder<packets::ProtocolVersion::V5>::encode(
        Message const& message, const std::uint16_t packetId, serialize::ByteWriter& writer)
    {
        using Mapping = MqttVersionMapping<packets::ProtocolVersion::V5>;

        const Mapping::Publish publishPacket(
            message.getTopic(),
            message.getPayload(),
            message.getQualityOfService(),
            message.shouldRetain(),
            packetId,
            {},
            // properties
            true // dup
        );

        publishPacket.encode(writer);
    }

    template std::unique_ptr<packets::IControlPacket> parsePacketImpl<packets::ProtocolVersion::V311>(
        serialize::ByteReader& reader, const packets::FixedHeader& fixedHeader, packets::PacketType packetType);

    template std::unique_ptr<packets::IControlPacket> parsePacketImpl<packets::ProtocolVersion::V5>(
        serialize::ByteReader& reader, const packets::FixedHeader& fixedHeader, packets::PacketType packetType);
} // namespace reactormq::mqtt::client
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "incoming_publish.h"

#include "mqtt/client/context.h"
#include "mqtt/client/mqtt_version_mapping.h"
#include "mqtt/packets/pub_ack.h"
#include "mqtt/packets/pub_rec.h"
#include "mqtt/packets/publish.h"
#include "reactormq/mqtt/message.h"
#include "serialize/bytes.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

namespace reactormq::mqtt::client::incoming::publish
{
    namespace
    {
        StateTransition handleQos0(Context& context, const packets::IPublishPacket& publish)
        {
            Message message(publish.getTopicName(), publish.getPayload(), publish.getShouldRetain(), QualityOfService::AtMostOnce);

            context.invokeCallback(
                [&ctx = context, msg = std::move(message)]() mutable
                {
                    ctx.getOnMessage().broadcast(msg);
                });

            return StateTransition::noTransition();
        }

        StateTransition handleQos1(Context& context, const packets::IPublishPacket& publish)
        {
            const std::uint16_t packetId = publish.getPacketId();
            if (!context.trackIncomingPacketId(packetId))
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 1 PUBLISH packet ID: %u", packetId);
                return StateTransition::noTransition();
            }

            Message message(publish.getTopicName(), publish.getPayload(), publish.getShouldRetain(), QualityOfService::AtLeastOnce);

            context.invokeCallback(
                [&ctx = context, msg = std::move(message)]() mutable
                {
                    ctx.getOnMessage().broadcast(msg);
                });

            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);
            withMqttVersion(
                context.getProtocolVersion(),
                [&writer, &packetId]<typename VersionTag>(VersionTag)
                {
                    constexpr auto kV = VersionTag::value;
                    packets::encodePubAckToWriter<kV>(writer, packetId);
                });

            if (const auto sock = context.getSocket())
            {
                sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
            }

            context.releaseIncomingPacketId(packetId);

            return StateTransition::noTransition();
        }

        StateTransition handleQos2(Context& context, const packets::IPublishPacket& publish)
        {
            const std::uint16_t packetId = publish.getPacketId();
            if (!context.trackIncomingPacketId(packetId))
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 2 PUBLISH packet ID: %u", packetId);
                return StateTransition::noTransition();
            }

            Message message(publish.getTopicName(), publish.getPayload(), publish.getShouldRetain(), QualityOfService::ExactlyOnce);

            context.storePendingIncomingQos2Message(packetId, std::move(message));

            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);
            withMqttVersion(
                context.getProtocolVersion(),
                [&writer, &packetId]<typename VersionTag>(VersionTag)
                {
                    constexpr auto kV = VersionTag::value;
                    packets::encodePubRecToWriter<kV>(writer, packetId);
                });

            if (const auto sock = context.getSocket())
            {
                sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
            }

            return StateTransition::noTransition();
        }
    } // namespace

    [[nodiscard]] StateTransition broadcast(Context& context, const packets::IControlPacket& packet)
    {
        if (packet.getPacketType() != packets::PacketType::Publish)
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "Unexpected packet type: Publish {}", packetTypeToString(packet.getPacketType()));
            return StateTransition::noTransition();
        }

        switch (const auto publish = static_cast<const packets::IPublishPacket*>(&packet); publish->getQualityOfService())
        {
            using enum QualityOfService;
        case AtMostOnce:
            return handleQos0(context, *publish);
        case AtLeastOnce:
            return handleQos1(context, *publish);
        case ExactlyOnce:
            return handleQos2(context, *publish);
        default:
            REACTORMQ_LOG(logging::LogLevel::Warn, "Invalid QoS level in PUBLISH packet");
            return StateTransition::noTransition();
        }
    }
} // namespace reactormq::mqtt::client::incoming::publish
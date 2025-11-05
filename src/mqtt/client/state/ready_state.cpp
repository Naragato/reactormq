#include "ready_state.h"

#include "closing_state.h"
#include "disconnected_state.h"
#include "mqtt/client/command.h"
#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/ping_req.h"
#include "mqtt/packets/publish.h"
#include "mqtt/packets/pub_ack.h"
#include "mqtt/packets/pub_comp.h"
#include "mqtt/packets/subscribe.h"
#include "mqtt/packets/sub_ack.h"
#include "mqtt/packets/unsub_ack.h"
#include "serialize/bytes.h"
#include "socket/socket.h"

#include <ranges>
#include <mqtt/client/mqtt_version_mapping.h>

namespace reactormq::mqtt::client
{
    namespace
    {
        template<packets::ProtocolVersion V> void encodePublish(
            serialize::ByteWriter& writer,
            const Message& message,
            const std::uint16_t packetId,
            const bool isDuplicate)
        {
            using Mapping = MqttVersionMapping<V>;
            if constexpr (V == packets::ProtocolVersion::V5)
            {
                typename Mapping::Publish publishPacket(
                    message.getTopic(),
                    message.getPayload(),
                    message.getQualityOfService(),
                    message.shouldRetain(),
                    packetId,
                    {},
                    isDuplicate);
                publishPacket.encode(writer);
            }
            else
            {
                typename Mapping::Publish publishPacket(
                    message.getTopic(),
                    message.getPayload(),
                    message.getQualityOfService(),
                    message.shouldRetain(),
                    packetId,
                    isDuplicate);
                publishPacket.encode(writer);
            }
        }

        template<packets::ProtocolVersion V> void encodeSubscribe(
            serialize::ByteWriter& writer,
            const std::vector<TopicFilter>& filters,
            const std::uint16_t packetId)
        {
            using Mapping = MqttVersionMapping<V>;
            if constexpr (V == packets::ProtocolVersion::V5)
            {
                typename Mapping::Subscribe subscribePacket(filters, packetId, {});
                subscribePacket.encode(writer);
            }
            else
            {
                typename Mapping::Subscribe subscribePacket(filters, packetId);
                subscribePacket.encode(writer);
            }
        }

        template<packets::ProtocolVersion V> void encodeUnsubscribe(
            serialize::ByteWriter& writer,
            const std::vector<std::string>& topics,
            const std::uint16_t packetId)
        {
            using Mapping = MqttVersionMapping<V>;
            if constexpr (V == packets::ProtocolVersion::V5)
            {
                typename Mapping::Unsubscribe unsubscribePacket(topics, packetId, {});
                unsubscribePacket.encode(writer);
            }
            else
            {
                typename Mapping::Unsubscribe unsubscribePacket(topics, packetId);
                unsubscribePacket.encode(writer);
            }
        }

        template<packets::ProtocolVersion V> void encodePubAck(serialize::ByteWriter& writer, const std::uint16_t packetId)
        {
            using Mapping = MqttVersionMapping<V>;
            typename Mapping::PubAck pubAck(packetId);
            pubAck.encode(writer);
        }

        template<packets::ProtocolVersion V> void encodePubRec(serialize::ByteWriter& writer, const std::uint16_t packetId)
        {
            using Mapping = MqttVersionMapping<V>;
            typename Mapping::PubRec pubRec(packetId);
            pubRec.encode(writer);
        }

        template<packets::ProtocolVersion V> void encodePubRel(serialize::ByteWriter& writer, const std::uint16_t packetId)
        {
            using Mapping = MqttVersionMapping<V>;
            typename Mapping::PubRel pubRel(packetId);
            pubRel.encode(writer);
        }

        template<packets::ProtocolVersion V> void encodePubComp(serialize::ByteWriter& writer, const std::uint16_t packetId)
        {
            using Mapping = MqttVersionMapping<V>;
            if constexpr (V == packets::ProtocolVersion::V5)
            {
                typename Mapping::PubComp pubComp(packetId, ReasonCode::Success);
                pubComp.encode(writer);
            }
            else
            {
                typename Mapping::PubComp pubComp(packetId);
                pubComp.encode(writer);
            }
        }
    }

    StateTransition ReadyState::onEnter(Context& context)
    {
        context.invokeCallback([&ctx = context]
        {
            ctx.getOnConnect().broadcast(true);
        });
        context.recordActivity();

        context.retransmitPendingPublishes();

        return StateTransition::noTransition();
    }

    void ReadyState::onExit(Context& context)
    {
        context.setPingPending(false);
    }

    StateTransition ReadyState::handleCommand(Context& context, Command& command)
    {
        const auto sock = context.getSocket();
        if (!sock)
        {
            return StateTransition::noTransition();
        }

        if (auto* publishCmd = std::get_if<PublishCommand>(&command))
        {
            return handlePublishCommand(context, *sock, *publishCmd);
        }

        if (auto* subscribeCmd = std::get_if<SubscribeCommand>(&command))
        {
            return handleSubscribeCommand(context, *sock, *subscribeCmd);
        }

        if (auto* subscribesCmd = std::get_if<SubscribesCommand>(&command))
        {
            return handleSubscribesCommand(context, *sock, *subscribesCmd);
        }

        if (auto* unsubscribesCmd = std::get_if<UnsubscribesCommand>(&command))
        {
            return handleUnsubscribesCommand(context, *sock, *unsubscribesCmd);
        }

        if (auto* disconnectCmd = std::get_if<DisconnectCommand>(&command))
        {
            auto& [promise] = *disconnectCmd;
            return StateTransition::transitionTo(std::make_unique<ClosingState>(std::move(promise)));
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::onSocketConnected(Context& context)
    {
        return StateTransition::noTransition();
    }

    StateTransition ReadyState::onSocketDisconnected(Context& context)
    {
        return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
    }

    StateTransition ReadyState::onDataReceived(Context& context, const uint8_t* data, const uint32_t size)
    {
        auto packet = context.parsePacket(data, size);
        if (packet == nullptr)
        {
            if (const auto settings = context.getSettings(); settings && settings->isStrictMode())
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
            return StateTransition::noTransition();
        }

        if (!packet->isValid())
        {
            if (const auto settings = context.getSettings(); settings && settings->isStrictMode())
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
            return StateTransition::noTransition();
        }

        const auto packetType = packet->getPacketType();
        const auto protocolVersion = context.getProtocolVersion();

        const auto controlPacket = packet.get();
        switch (packetType)
        {
            using enum packets::PacketType;
        case PubAck:
            return handlePubAck(context, *controlPacket);

        case SubAck:
            return handleSubAck(context, *controlPacket, protocolVersion);

        case UnsubAck:
            return handleUnsubAck(context, *controlPacket, protocolVersion);

        case Publish:
            return handlePublish(context, *controlPacket, protocolVersion);

        case PubRec:
            return handlePubRec(context, *controlPacket, protocolVersion);

        case PubRel:
            return handlePubRel(context, *controlPacket, protocolVersion);

        case PubComp:
            return handlePubComp(context, *controlPacket);

        case PingResp:
            return handlePingResp(context);

        default: REACTORMQ_LOG(logging::LogLevel::Warn, "Unexpected packet type %d in Ready state", static_cast<int>(packetType));

            {
                auto settings = context.getSettings();
                if (settings && settings->isStrictMode())
                {
                    return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
                }
            }
            return StateTransition::noTransition();
        }
    }

    StateTransition ReadyState::onTick(Context& context)
    {
        const auto settings = context.getSettings();
        if (!settings)
        {
            return StateTransition::noTransition();
        }

        const std::uint16_t keepaliveSeconds = settings->getKeepAliveIntervalSeconds();
        if (keepaliveSeconds == 0)
        {
            return StateTransition::noTransition();
        }

        const auto timeSinceActivity = context.getTimeSinceLastActivity();
        const auto keepaliveMs = std::chrono::milliseconds(keepaliveSeconds * 1000);

        if (context.isPingPending())
        {
            const auto pingTimeout = keepaliveMs + keepaliveMs / 2;
            if (timeSinceActivity >= pingTimeout)
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
        }
        else if (timeSinceActivity >= keepaliveMs)
        {
            if (const auto sock = context.getSocket())
            {
                std::vector<std::byte> buffer;
                serialize::ByteWriter writer(buffer);
                const packets::PingReq pingReq;
                pingReq.encode(writer);

                sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

                context.setPingPending(true);
                context.recordActivity();
            }
        }

        constexpr std::chrono::milliseconds kPublishTimeout{ 30000 };
        std::vector<std::uint16_t> timedOutPacketIds;

        for (const auto& packetId : context.getPendingPublishes() | std::views::keys)
        {
            if (context.getPublishElapsedTime(packetId) >= kPublishTimeout)
            {
                timedOutPacketIds.push_back(packetId);
            }
        }

        for (const auto packetId : timedOutPacketIds)
        {
            auto cmd = context.takePendingPublish(packetId);
            if (cmd.has_value())
            {
                context.releasePacketId(packetId);
                context.clearPublishTimeout(packetId);
                cmd->promise.set_value(Result<void>::failure("Publish timeout"));
            }
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePublishCommand(Context& context, socket::Socket& sock, PublishCommand& publishCmd)
    {
        const auto& message = publishCmd.message;
        const auto qos = message.getQualityOfService();

        std::uint16_t packetId = 0;
        if (qos == QualityOfService::AtLeastOnce || qos == QualityOfService::ExactlyOnce)
        {
            if (!context.canAddPendingCommand())
            {
                publishCmd.promise.set_value(Result<void>::failure("Max pending commands limit exceeded"));
                return StateTransition::noTransition();
            }

            packetId = context.allocatePacketId();
            if (packetId == 0)
            {
                publishCmd.promise.set_value(Result<void>::failure("Packet ID pool exhausted"));
                return StateTransition::noTransition();
            }
        }

        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        withMqttVersion(
            context.getProtocolVersion(),
            [&writer, &message, packetId]<typename VersionTag>(VersionTag)
            {
                constexpr auto kV = VersionTag::value;
                encodePublish<kV>(writer, message, packetId, false);
            });

        const std::size_t packetSize = buffer.size();
        if (!context.canAddToOutboundQueue(packetSize))
        {
            if (qos != QualityOfService::AtMostOnce)
            {
                context.releasePacketId(packetId);
            }
            publishCmd.promise.set_value(Result<void>::failure("Outbound queue full"));
            return StateTransition::noTransition();
        }

        sock.send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

        context.addOutboundQueueSize(packetSize);

        if (qos == QualityOfService::AtMostOnce)
        {
            publishCmd.promise.set_value(Result<void>::success());
            context.subtractOutboundQueueSize(packetSize);
        }
        else
        {
            context.storePendingPublish(packetId, std::move(publishCmd));

            context.recordPublishSent(packetId);

            context.subtractOutboundQueueSize(packetSize);
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handleSubscribeCommand(Context& context, socket::Socket& sock, SubscribeCommand& subscribeCmd)
    {
        if (!context.canAddPendingCommand())
        {
            subscribeCmd.promise.set_value(Result<SubscribeResult>::failure("Max pending commands limit exceeded"));
            return StateTransition::noTransition();
        }

        const std::uint16_t packetId = context.allocatePacketId();
        if (packetId == 0)
        {
            subscribeCmd.promise.set_value(Result<SubscribeResult>::failure("Packet ID pool exhausted"));
            return StateTransition::noTransition();
        }

        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        const std::vector filters = { subscribeCmd.topicFilter };
        withMqttVersion(
            context.getProtocolVersion(),
            [&writer, &filters, &packetId]<typename VersionTag>(VersionTag)
            {
                constexpr auto kV = VersionTag::value;
                encodeSubscribe<kV>(writer, filters, packetId);
            });

        sock.send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

        context.storePendingSubscribe(packetId, std::move(subscribeCmd));

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handleSubscribesCommand(Context& context, socket::Socket& sock, SubscribesCommand& subscribesCmd)
    {
        if (!context.canAddPendingCommand())
        {
            subscribesCmd.promise.set_value(Result<std::vector<SubscribeResult>>::failure("Max pending commands limit exceeded"));
            return StateTransition::noTransition();
        }

        const std::uint16_t packetId = context.allocatePacketId();
        if (packetId == 0)
        {
            subscribesCmd.promise.set_value(Result<std::vector<SubscribeResult>>::failure("Packet ID pool exhausted"));
            return StateTransition::noTransition();
        }

        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        withMqttVersion(
            context.getProtocolVersion(),
            [&writer, &filters = subscribesCmd.topicFilters, &packetId]<typename VersionTag>(VersionTag)
            {
                constexpr auto kV = VersionTag::value;
                encodeSubscribe<kV>(writer, filters, packetId);
            });

        sock.send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

        context.storePendingSubscribes(packetId, std::move(subscribesCmd));

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handleUnsubscribesCommand(Context& context, socket::Socket& sock, UnsubscribesCommand& unsubscribesCmd)
    {
        if (!context.canAddPendingCommand())
        {
            unsubscribesCmd.promise.set_value(Result<std::vector<UnsubscribeResult>>::failure("Max pending commands limit exceeded"));
            return StateTransition::noTransition();
        }

        const std::uint16_t packetId = context.allocatePacketId();
        if (packetId == 0)
        {
            unsubscribesCmd.promise.set_value(Result<std::vector<UnsubscribeResult>>::failure("Packet ID pool exhausted"));
            return StateTransition::noTransition();
        }

        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        withMqttVersion(
            context.getProtocolVersion(),
            [&writer, &topics = unsubscribesCmd.topics, &packetId]<typename VersionTag>(VersionTag)
            {
                constexpr auto kV = VersionTag::value;
                encodeUnsubscribe<kV>(writer, topics, packetId);
            });

        sock.send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

        context.storePendingUnsubscribes(packetId, std::move(unsubscribesCmd));

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePubAck(Context& context, const packets::IControlPacket& packet)
    {
        const std::uint16_t packetId = packet.getPacketId();
        context.clearPublishTimeout(packetId);

        if (auto pendingPublish = context.takePendingPublish(packetId); pendingPublish.has_value())
        {
            context.releasePacketId(packetId);
            pendingPublish->promise.set_value(Result<void>::success());
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handleSubAck(
        Context& context,
        const packets::IControlPacket& packet,
        packets::ProtocolVersion protocolVersion)
    {
        const std::uint16_t packetId = packet.getPacketId();

        if (protocolVersion == packets::ProtocolVersion::V5)
        {
            auto* subAck = static_cast<const packets::SubAck<packets::ProtocolVersion::V5>*>(&packet);
            if (nullptr != subAck)
            {
                if (auto pendingSingleSubscribe = context.takePendingSubscribe(packetId); pendingSingleSubscribe.has_value())
                {
                    context.releasePacketId(packetId);

                    if (const auto& reasonCodes = subAck->getReasonCodes(); !reasonCodes.empty())
                    {
                        using enum ReasonCode;
                        const auto reasonCode = reasonCodes[0];
                        const bool wasSuccessful = reasonCode == Success || reasonCode == GrantedQualityOfService1 || reasonCode ==
                            GrantedQualityOfService2;
                        SubscribeResult result{ pendingSingleSubscribe->topicFilter, wasSuccessful };
                        pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::success(std::move(result)));
                    }
                    else
                    {
                        pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::failure("Empty SUBACK"));
                    }
                    return StateTransition::noTransition();
                }

                auto pendingMultiSubscribe = context.takePendingSubscribes(packetId);
                if (pendingMultiSubscribe.has_value())
                {
                    context.releasePacketId(packetId);

                    const auto& reasonCodes = subAck->getReasonCodes();
                    std::vector<SubscribeResult> results;
                    for (std::size_t i = 0; i < reasonCodes.size() && i < pendingMultiSubscribe->topicFilters.size(); ++i)
                    {
                        using enum ReasonCode;
                        const auto reasonCode = reasonCodes[i];
                        const bool wasSuccessful = reasonCode == Success || reasonCode == GrantedQualityOfService1 || reasonCode ==
                            GrantedQualityOfService2;
                        results.emplace_back(pendingMultiSubscribe->topicFilters[i], wasSuccessful);
                    }
                    pendingMultiSubscribe->promise.set_value(Result<std::vector<SubscribeResult>>::success(std::move(results)));
                }
            }
        }
        else
        {
            auto* subAck = static_cast<const packets::SubAck<packets::ProtocolVersion::V311>*>(&packet);
            if (nullptr != subAck)
            {
                if (auto pendingSingleSubscribe = context.takePendingSubscribe(packetId); pendingSingleSubscribe.has_value())
                {
                    context.releasePacketId(packetId);

                    if (const auto& returnCodes = subAck->getReturnCodes(); !returnCodes.empty())
                    {
                        const auto returnCode = returnCodes[0];
                        const bool wasSuccessful = returnCode != SubscribeReturnCode::Failure;
                        SubscribeResult result{ pendingSingleSubscribe->topicFilter, wasSuccessful };
                        pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::success(std::move(result)));
                    }
                    else
                    {
                        pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::failure("Empty SUBACK"));
                    }
                    return StateTransition::noTransition();
                }

                auto pendingMultiSubscribe = context.takePendingSubscribes(packetId);
                if (pendingMultiSubscribe.has_value())
                {
                    context.releasePacketId(packetId);

                    const auto& returnCodes = subAck->getReturnCodes();
                    std::vector<SubscribeResult> results;
                    for (std::size_t i = 0; i < returnCodes.size() && i < pendingMultiSubscribe->topicFilters.size(); ++i)
                    {
                        const bool wasSuccessful = returnCodes[i] != SubscribeReturnCode::Failure;
                        results.emplace_back(pendingMultiSubscribe->topicFilters[i], wasSuccessful);
                    }
                    pendingMultiSubscribe->promise.set_value(Result<std::vector<SubscribeResult>>::success(std::move(results)));
                }
            }
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handleUnsubAck(
        Context& context,
        const packets::IControlPacket& packet,
        packets::ProtocolVersion protocolVersion)
    {
        const std::uint16_t packetId = packet.getPacketId();

        if (protocolVersion == packets::ProtocolVersion::V5)
        {
            auto* unsuback = static_cast<const packets::UnsubAck<packets::ProtocolVersion::V5>*>(&packet);
            if (nullptr != unsuback)
            {
                auto pendingUnsubscribe = context.takePendingUnsubscribes(packetId);
                if (pendingUnsubscribe.has_value())
                {
                    context.releasePacketId(packetId);

                    const auto& reasonCodes = unsuback->getReasonCodes();
                    std::vector<UnsubscribeResult> results;
                    for (std::size_t i = 0; i < reasonCodes.size() && i < pendingUnsubscribe->topics.size(); ++i)
                    {
                        const bool wasSuccessful = reasonCodes[i] == ReasonCode::Success;
                        TopicFilter filter(pendingUnsubscribe->topics[i]);
                        UnsubscribeResult result(filter, wasSuccessful);
                        results.push_back(std::move(result));
                    }
                    pendingUnsubscribe->promise.set_value(Result<std::vector<UnsubscribeResult>>::success(std::move(results)));
                }
            }
        }
        else
        {
            auto* unsuback = static_cast<const packets::UnsubAck<packets::ProtocolVersion::V311>*>(&packet);
            if (nullptr != unsuback)
            {
                auto pendingUnsubscribe = context.takePendingUnsubscribes(packetId);
                if (pendingUnsubscribe.has_value())
                {
                    context.releasePacketId(packetId);

                    std::vector<UnsubscribeResult> results;
                    for (const auto& topic : pendingUnsubscribe->topics)
                    {
                        TopicFilter filter(topic);
                        UnsubscribeResult result(filter, true);
                        results.push_back(std::move(result));
                    }
                    pendingUnsubscribe->promise.set_value(Result<std::vector<UnsubscribeResult>>::success(std::move(results)));
                }
            }
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePublish(
        Context& context,
        const packets::IControlPacket& packet,
        packets::ProtocolVersion protocolVersion)
    {
        if (protocolVersion == packets::ProtocolVersion::V5)
        {
            auto* publish = static_cast<const packets::Publish<packets::ProtocolVersion::V5>*>(&packet);
            if (nullptr != publish)
            {
                const auto qos = publish->getQualityOfService();

                if (qos == QualityOfService::ExactlyOnce)
                {
                    const std::uint16_t packetId = publish->getPacketId();
                    if (!context.trackIncomingPacketId(packetId))
                    {
                        REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 2 PUBLISH packet ID: %u", packetId);
                        return StateTransition::noTransition();
                    }

                    Message message(publish->getTopicName(), publish->getPayload(), publish->getShouldRetain(), qos);

                    context.storePendingIncomingQos2Message(packetId, std::move(message));

                    std::vector<std::byte> buffer;
                    serialize::ByteWriter writer(buffer);
                    withMqttVersion(
                        protocolVersion,
                        [&writer, &packetId]<typename VersionTag>(VersionTag)
                        {
                            constexpr auto kV = VersionTag::value;
                            encodePubRec<kV>(writer, packetId);
                        });

                    if (auto sock = context.getSocket())
                    {
                        sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
                    }
                }
                else if (qos == QualityOfService::AtLeastOnce)
                {
                    const std::uint16_t packetId = publish->getPacketId();
                    if (!context.trackIncomingPacketId(packetId))
                    {
                        REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 1 PUBLISH packet ID: %u", packetId);
                        return StateTransition::noTransition();
                    }

                    context.invokeCallback(
                        [&ctx = context, msg = Message{ publish->getTopicName(), publish->getPayload(), publish->getShouldRetain(), qos }
                        ]() mutable
                        {
                            ctx.getOnMessage().broadcast(msg);
                        });

                    std::vector<std::byte> buffer;
                    serialize::ByteWriter writer(buffer);
                    withMqttVersion(
                        protocolVersion,
                        [&writer, &packetId]<typename VersionTag>(VersionTag)
                        {
                            constexpr auto kV = VersionTag::value;
                            encodePubAck<kV>(writer, packetId);
                        });

                    if (auto sock = context.getSocket())
                    {
                        sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
                    }

                    context.releaseIncomingPacketId(packetId);
                }
                else
                {
                    Message message(publish->getTopicName(), publish->getPayload(), publish->getShouldRetain(), qos);

                    context.invokeCallback(
                        [&ctx = context, msg = std::move(message)]() mutable
                        {
                            ctx.getOnMessage().broadcast(msg);
                        });
                }
            }
        }
        else
        {
            auto* publish = static_cast<const packets::Publish<packets::ProtocolVersion::V311>*>(&packet);
            if (nullptr != publish)
            {
                const auto qos = publish->getQualityOfService();

                if (qos == QualityOfService::ExactlyOnce)
                {
                    const std::uint16_t packetId = publish->getPacketId();
                    if (!context.trackIncomingPacketId(packetId))
                    {
                        REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 2 PUBLISH packet ID: %u", packetId);
                        return StateTransition::noTransition();
                    }

                    Message message(publish->getTopicName(), publish->getPayload(), publish->getShouldRetain(), qos);

                    context.storePendingIncomingQos2Message(packetId, std::move(message));

                    std::vector<std::byte> buffer;
                    serialize::ByteWriter writer(buffer);
                    withMqttVersion(
                        protocolVersion,
                        [&writer, &packetId]<typename VersionTag>(VersionTag)
                        {
                            constexpr auto kV = VersionTag::value;
                            encodePubRec<kV>(writer, packetId);
                        });

                    if (auto sock = context.getSocket())
                    {
                        sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
                    }
                }
                else if (qos == QualityOfService::AtLeastOnce)
                {
                    const std::uint16_t packetId = publish->getPacketId();
                    if (!context.trackIncomingPacketId(packetId))
                    {
                        REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 1 PUBLISH packet ID: %u", packetId);
                        return StateTransition::noTransition();
                    }

                    Message message(publish->getTopicName(), publish->getPayload(), publish->getShouldRetain(), qos);

                    context.invokeCallback(
                        [&ctx = context, msg = std::move(message)]() mutable
                        {
                            ctx.getOnMessage().broadcast(msg);
                        });

                    std::vector<std::byte> buffer;
                    serialize::ByteWriter writer(buffer);
                    withMqttVersion(
                        protocolVersion,
                        [&writer, &packetId]<typename VersionTag>(VersionTag)
                        {
                            constexpr auto kV = VersionTag::value;
                            encodePubAck<kV>(writer, packetId);
                        });

                    if (auto sock = context.getSocket())
                    {
                        sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
                    }

                    context.releaseIncomingPacketId(packetId);
                }
                else
                {
                    Message message(publish->getTopicName(), publish->getPayload(), publish->getShouldRetain(), qos);

                    context.invokeCallback(
                        [&ctx = context, msg = std::move(message)]() mutable
                        {
                            ctx.getOnMessage().broadcast(msg);
                        });
                }
            }
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePubRec(
        const Context& context,
        const packets::IControlPacket& packet,
        const packets::ProtocolVersion protocolVersion)
    {
        const std::uint16_t packetId = packet.getPacketId();

        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        withMqttVersion(
            protocolVersion,
            [&writer, &packetId]<typename VersionTag>(VersionTag)
            {
                constexpr auto kV = VersionTag::value;
                encodePubRel<kV>(writer, packetId);
            });

        if (auto sock = context.getSocket())
        {
            sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePubRel(
        Context& context,
        const packets::IControlPacket& packet,
        const packets::ProtocolVersion protocolVersion)
    {
        const std::uint16_t packetId = packet.getPacketId();

        if (auto message = context.takePendingIncomingQos2Message(packetId); message.has_value())
        {
            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);

            withMqttVersion(
                protocolVersion,
                [&writer, &packetId]<typename VersionTag>(VersionTag)
                {
                    constexpr auto kV = VersionTag::value;
                    encodePubComp<kV>(writer, packetId);
                });

            if (auto sock = context.getSocket())
            {
                sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
            }

            context.invokeCallback(
                [&ctx = context, msg = std::move(message.value())]() mutable
                {
                    ctx.getOnMessage().broadcast(msg);
                });

            context.releaseIncomingPacketId(packetId);
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePubComp(Context& context, const packets::IControlPacket& packet)
    {
        const std::uint16_t packetId = packet.getPacketId();

        context.clearPublishTimeout(packetId);

        if (auto pendingPublish = context.takePendingPublish(packetId); pendingPublish.has_value())
        {
            context.releasePacketId(packetId);
            pendingPublish->promise.set_value(Result<void>::success());
        }

        return StateTransition::noTransition();
    }

    StateTransition ReadyState::handlePingResp(Context& context)
    {
        context.setPingPending(false);
        context.recordActivity();

        return StateTransition::noTransition();
    }
} // namespace reactormq::mqtt::client
#include "ready_state.h"
#include "disconnected_state.h"
#include "closing_state.h"
#include "mqtt/client/command.h"
#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/publish.h"
#include "mqtt/packets/subscribe.h"
#include "mqtt/packets/unsubscribe.h"
#include "mqtt/packets/pub_ack.h"
#include "mqtt/packets/pub_rec.h"
#include "mqtt/packets/pub_rel.h"
#include "mqtt/packets/pub_comp.h"
#include "mqtt/packets/sub_ack.h"
#include "mqtt/packets/unsub_ack.h"
#include "mqtt/packets/ping_req.h"
#include "serialize/bytes.h"
#include "socket/socket_base.h"

namespace reactormq::mqtt::client
{
    StateTransition ReadyState::onEnter(Context& context)
    {
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
        auto sock = context.getSocket();
        if (!sock)
        {
            return StateTransition::noTransition();
        }

        if (std::holds_alternative<PublishCommand>(command))
        {
            auto& publishCmd = std::get<PublishCommand>(command);
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

            const auto protocolVersion = context.getProtocolVersion();
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                packets::Publish<packets::ProtocolVersion::V5> publishPacket(
                    message.getTopic(),
                    message.getPayload(),
                    qos,
                    message.shouldRetain(),
                    packetId,
                    {},
                    false
                    );
                publishPacket.encode(writer);
            }
            else
            {
                packets::Publish<packets::ProtocolVersion::V311> publishPacket(
                    message.getTopic(),
                    message.getPayload(),
                    qos,
                    message.shouldRetain(),
                    packetId,
                    false
                    );
                publishPacket.encode(writer);
            }

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

            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

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

        if (std::holds_alternative<SubscribeCommand>(command))
        {
            auto& subscribeCmd = std::get<SubscribeCommand>(command);

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

            const auto protocolVersion = context.getProtocolVersion();
            std::vector filters = { subscribeCmd.topicFilter };

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                packets::Subscribe<packets::ProtocolVersion::V5> subscribePacket(filters, packetId, {});
                subscribePacket.encode(writer);
            }
            else
            {
                packets::Subscribe<packets::ProtocolVersion::V311> subscribePacket(filters, packetId);
                subscribePacket.encode(writer);
            }

            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

            context.storePendingSubscribe(packetId, std::move(subscribeCmd));

            return StateTransition::noTransition();
        }

        if (std::holds_alternative<SubscribesCommand>(command))
        {
            auto& subscribesCmd = std::get<SubscribesCommand>(command);

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

            const auto protocolVersion = context.getProtocolVersion();

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                packets::Subscribe<packets::ProtocolVersion::V5> subscribePacket(subscribesCmd.topicFilters, packetId, {});
                subscribePacket.encode(writer);
            }
            else
            {
                packets::Subscribe<packets::ProtocolVersion::V311> subscribePacket(subscribesCmd.topicFilters, packetId);
                subscribePacket.encode(writer);
            }

            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

            context.storePendingSubscribes(packetId, std::move(subscribesCmd));

            return StateTransition::noTransition();
        }

        if (std::holds_alternative<UnsubscribesCommand>(command))
        {
            auto& unsubscribesCmd = std::get<UnsubscribesCommand>(command);

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

            const auto protocolVersion = context.getProtocolVersion();

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                packets::Unsubscribe<packets::ProtocolVersion::V5> unsubscribePacket(unsubscribesCmd.topics, packetId, {});
                unsubscribePacket.encode(writer);
            }
            else
            {
                packets::Unsubscribe<packets::ProtocolVersion::V311> unsubscribePacket(unsubscribesCmd.topics, packetId);
                unsubscribePacket.encode(writer);
            }

            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

            context.storePendingUnsubscribes(packetId, std::move(unsubscribesCmd));

            return StateTransition::noTransition();
        }

        if (std::holds_alternative<DisconnectCommand>(command))
        {
            auto& disconnectCmd = std::get<DisconnectCommand>(command);
            return StateTransition::transitionTo(std::make_unique<ClosingState>(std::move(disconnectCmd.promise)));
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

    StateTransition ReadyState::onDataReceived(Context& context, const uint8_t* data, uint32_t size)
    {
        auto packet = context.parsePacket(data, size);
        if (packet == nullptr)
        {
            auto settings = context.getSettings();
            if (settings && settings->isStrictMode())
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
            return StateTransition::noTransition();
        }

        if (!packet->isValid())
        {
            auto settings = context.getSettings();
            if (settings && settings->isStrictMode())
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
            return StateTransition::noTransition();
        }

        const auto packetType = packet->getPacketType();
        const auto protocolVersion = context.getProtocolVersion();

        switch (packetType)
        {
        case packets::PacketType::PubAck: {
            const std::uint16_t packetId = packet->getPacketId();
            context.clearPublishTimeout(packetId);

            auto pendingPublish = context.takePendingPublish(packetId);
            if (pendingPublish.has_value())
            {
                context.releasePacketId(packetId);
                pendingPublish->promise.set_value(Result<void>::success());
            }
            break;
        }

        case packets::PacketType::SubAck: {
            const std::uint16_t packetId = packet->getPacketId();

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                auto* subAck = reinterpret_cast<packets::SubAck<packets::ProtocolVersion::V5>*>(packet.get());
                if (subAck)
                {
                    auto pendingSingleSubscribe = context.takePendingSubscribe(packetId);
                    if (pendingSingleSubscribe.has_value())
                    {
                        context.releasePacketId(packetId);

                        const auto& reasonCodes = subAck->getReasonCodes();
                        if (!reasonCodes.empty())
                        {
                            const auto reasonCode = reasonCodes[0];
                            const bool wasSuccessful = (reasonCode == ReasonCode::Success ||
                                reasonCode == ReasonCode::GrantedQualityOfService1 ||
                                reasonCode == ReasonCode::GrantedQualityOfService2);
                            SubscribeResult result{ pendingSingleSubscribe->topicFilter, wasSuccessful };
                            pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::success(std::move(result)));
                        }
                        else
                        {
                            pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::failure("Empty SUBACK"));
                        }
                        break;
                    }

                    auto pendingMultiSubscribe = context.takePendingSubscribes(packetId);
                    if (pendingMultiSubscribe.has_value())
                    {
                        context.releasePacketId(packetId);

                        const auto& reasonCodes = subAck->getReasonCodes();
                        std::vector<SubscribeResult> results;
                        for (std::size_t i = 0; i < reasonCodes.size() && i < pendingMultiSubscribe->topicFilters.size(); ++i)
                        {
                            const auto reasonCode = reasonCodes[i];
                            const bool wasSuccessful = (reasonCode == ReasonCode::Success ||
                                reasonCode == ReasonCode::GrantedQualityOfService1 ||
                                reasonCode == ReasonCode::GrantedQualityOfService2);
                            results.emplace_back(pendingMultiSubscribe->topicFilters[i], wasSuccessful);
                        }
                        pendingMultiSubscribe->promise.set_value(Result<std::vector<SubscribeResult>>::success(std::move(results)));
                    }
                }
            }
            else
            {
                auto* subAck = reinterpret_cast<packets::SubAck<packets::ProtocolVersion::V311>*>(packet.get());
                if (subAck)
                {
                    auto pendingSingleSubscribe = context.takePendingSubscribe(packetId);
                    if (pendingSingleSubscribe.has_value())
                    {
                        context.releasePacketId(packetId);

                        const auto& returnCodes = subAck->getReturnCodes();
                        if (!returnCodes.empty())
                        {
                            const auto returnCode = returnCodes[0];
                            const bool wasSuccessful = (returnCode != SubscribeReturnCode::Failure);
                            SubscribeResult result{ pendingSingleSubscribe->topicFilter, wasSuccessful };
                            pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::success(std::move(result)));
                        }
                        else
                        {
                            pendingSingleSubscribe->promise.set_value(Result<SubscribeResult>::failure("Empty SUBACK"));
                        }
                        break;
                    }

                    auto pendingMultiSubscribe = context.takePendingSubscribes(packetId);
                    if (pendingMultiSubscribe.has_value())
                    {
                        context.releasePacketId(packetId);

                        const auto& returnCodes = subAck->getReturnCodes();
                        std::vector<SubscribeResult> results;
                        for (std::size_t i = 0; i < returnCodes.size() && i < pendingMultiSubscribe->topicFilters.size(); ++i)
                        {
                            const bool wasSuccessful = (returnCodes[i] != SubscribeReturnCode::Failure);
                            results.emplace_back(pendingMultiSubscribe->topicFilters[i], wasSuccessful);
                        }
                        pendingMultiSubscribe->promise.set_value(Result<std::vector<SubscribeResult>>::success(std::move(results)));
                    }
                }
            }
            break;
        }

        case packets::PacketType::UnsubAck: {
            const std::uint16_t packetId = packet->getPacketId();

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                auto* unsuback = reinterpret_cast<packets::UnsubAck<packets::ProtocolVersion::V5>*>(packet.get());
                if (unsuback)
                {
                    auto pendingUnsubscribe = context.takePendingUnsubscribes(packetId);
                    if (pendingUnsubscribe.has_value())
                    {
                        context.releasePacketId(packetId);

                        const auto& reasonCodes = unsuback->getReasonCodes();
                        std::vector<UnsubscribeResult> results;
                        for (std::size_t i = 0; i < reasonCodes.size() && i < pendingUnsubscribe->topics.size(); ++i)
                        {
                            const bool wasSuccessful = (reasonCodes[i] == ReasonCode::Success);
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
                auto* unsuback = reinterpret_cast<packets::UnsubAck<packets::ProtocolVersion::V311>*>(packet.get());
                if (unsuback)
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
            break;
        }

        case packets::PacketType::Publish: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                auto* publish = reinterpret_cast<packets::Publish<packets::ProtocolVersion::V5>*>(packet.get());
                if (publish)
                {
                    const auto qos = publish->getQualityOfService();

                    if (qos == QualityOfService::ExactlyOnce)
                    {
                        const std::uint16_t packetId = publish->getPacketId();
                        if (!context.trackIncomingPacketId(packetId))
                        {
                            REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 2 PUBLISH packet ID: %u", packetId);
                            break;
                        }

                        Message message(
                            publish->getTopicName(),
                            publish->getPayload(),
                            publish->getShouldRetain(),
                            qos
                            );

                        context.storePendingIncomingQos2Message(packetId, std::move(message));

                        std::vector<std::byte> buffer;
                        serialize::ByteWriter writer(buffer);
                        packets::PubRec<packets::ProtocolVersion::V5> pubRec(packetId);
                        pubRec.encode(writer);

                        auto sock = context.getSocket();
                        if (sock)
                        {
                            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
                        }
                    }
                    else if (qos == QualityOfService::AtLeastOnce)
                    {
                        const std::uint16_t packetId = publish->getPacketId();
                        if (!context.trackIncomingPacketId(packetId))
                        {
                            REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 1 PUBLISH packet ID: %u", packetId);
                            break;
                        }

                        Message message(
                            publish->getTopicName(),
                            publish->getPayload(),
                            publish->getShouldRetain(),
                            qos
                            );

                        context.invokeCallback(
                            [&ctx = context, msg = std::move(message)]() mutable
                            {
                                ctx.getOnMessage().broadcast(msg);
                            });

                        std::vector<std::byte> buffer;
                        serialize::ByteWriter writer(buffer);
                        packets::PubAck<packets::ProtocolVersion::V5> pubAck(packetId);
                        pubAck.encode(writer);

                        auto sock = context.getSocket();
                        if (sock)
                        {
                            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
                        }

                        context.releaseIncomingPacketId(packetId);
                    }
                    else
                    {
                        Message message(
                            publish->getTopicName(),
                            publish->getPayload(),
                            publish->getShouldRetain(),
                            qos
                            );

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
                auto* publish = reinterpret_cast<packets::Publish<packets::ProtocolVersion::V311>*>(packet.get());
                if (publish)
                {
                    const auto qos = publish->getQualityOfService();

                    if (qos == QualityOfService::ExactlyOnce)
                    {
                        const std::uint16_t packetId = publish->getPacketId();
                        if (!context.trackIncomingPacketId(packetId))
                        {
                            REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 2 PUBLISH packet ID: %u", packetId);
                            break;
                        }

                        Message message(
                            publish->getTopicName(),
                            publish->getPayload(),
                            publish->getShouldRetain(),
                            qos
                            );

                        context.storePendingIncomingQos2Message(packetId, std::move(message));

                        std::vector<std::byte> buffer;
                        serialize::ByteWriter writer(buffer);
                        packets::PubRec<packets::ProtocolVersion::V311> pubRec(packetId);
                        pubRec.encode(writer);

                        auto sock = context.getSocket();
                        if (sock)
                        {
                            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
                        }
                    }
                    else if (qos == QualityOfService::AtLeastOnce)
                    {
                        const std::uint16_t packetId = publish->getPacketId();
                        if (!context.trackIncomingPacketId(packetId))
                        {
                            REACTORMQ_LOG(logging::LogLevel::Warn, "Duplicate QoS 1 PUBLISH packet ID: %u", packetId);
                            break;
                        }

                        Message message(
                            publish->getTopicName(),
                            publish->getPayload(),
                            publish->getShouldRetain(),
                            qos
                            );

                        context.invokeCallback(
                            [&ctx = context, msg = std::move(message)]() mutable
                            {
                                ctx.getOnMessage().broadcast(msg);
                            });

                        std::vector<std::byte> buffer;
                        serialize::ByteWriter writer(buffer);
                        packets::PubAck<packets::ProtocolVersion::V311> pubAck(packetId);
                        pubAck.encode(writer);

                        auto sock = context.getSocket();
                        if (sock)
                        {
                            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
                        }

                        context.releaseIncomingPacketId(packetId);
                    }
                    else
                    {
                        Message message(
                            publish->getTopicName(),
                            publish->getPayload(),
                            publish->getShouldRetain(),
                            qos
                            );

                        context.invokeCallback(
                            [&ctx = context, msg = std::move(message)]() mutable
                            {
                                ctx.getOnMessage().broadcast(msg);
                            });
                    }
                }
            }
            break;
        }

        case packets::PacketType::PubRec: {
            const std::uint16_t packetId = packet->getPacketId();

            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                packets::PubRel<packets::ProtocolVersion::V5> pubRel(packetId);
                pubRel.encode(writer);
            }
            else
            {
                packets::PubRel<packets::ProtocolVersion::V311> pubRel(packetId);
                pubRel.encode(writer);
            }

            auto sock = context.getSocket();
            if (sock)
            {
                sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
            }
            break;
        }

        case packets::PacketType::PubRel: {
            const std::uint16_t packetId = packet->getPacketId();

            auto message = context.takePendingIncomingQos2Message(packetId);
            if (message.has_value())
            {
                std::vector<std::byte> buffer;
                serialize::ByteWriter writer(buffer);

                if (protocolVersion == packets::ProtocolVersion::V5)
                {
                    packets::PubComp<packets::ProtocolVersion::V5> pubComp(packetId, ReasonCode::Success);
                    pubComp.encode(writer);
                }
                else
                {
                    packets::PubComp<packets::ProtocolVersion::V311> pubComp(packetId);
                    pubComp.encode(writer);
                }

                auto sock = context.getSocket();
                if (sock)
                {
                    sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
                }

                context.invokeCallback(
                    [&ctx = context, msg = std::move(message.value())]() mutable
                    {
                        ctx.getOnMessage().broadcast(msg);
                    });

                context.releaseIncomingPacketId(packetId);
            }
            break;
        }

        case packets::PacketType::PubComp: {
            const std::uint16_t packetId = packet->getPacketId();

            context.clearPublishTimeout(packetId);

            auto pendingPublish = context.takePendingPublish(packetId);
            if (pendingPublish.has_value())
            {
                context.releasePacketId(packetId);
                pendingPublish->promise.set_value(Result<void>::success());
            }
            break;
        }

        case packets::PacketType::PingResp: {
            context.setPingPending(false);
            context.recordActivity();
            break;
        }

        default: {
            REACTORMQ_LOG(
                logging::LogLevel::Warn,
                "Unexpected packet type %d in Ready state",
                static_cast<int>(packetType));

            auto settings = context.getSettings();
            if (settings && settings->isStrictMode())
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
            break;
        }
        }

        return StateTransition::noTransition();
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
            const auto pingTimeout = keepaliveMs + (keepaliveMs / 2);
            if (timeSinceActivity >= pingTimeout)
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
        }
        else if (timeSinceActivity >= keepaliveMs)
        {
            const auto sock = context.getSocket();
            if (sock)
            {
                std::vector<std::byte> buffer;
                serialize::ByteWriter writer(buffer);
                const packets::PingReq pingReq;
                pingReq.encode(writer);

                sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

                context.setPingPending(true);
                context.recordActivity();
            }
        }

        // TODO: Move to settings?
        constexpr std::chrono::milliseconds kPublishTimeout{ 30000 };
        std::vector<std::uint16_t> timedOutPacketIds;

        for (const auto& [packetId, publishCmd] : context.getPendingPublishes())
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
} // namespace reactormq::mqtt::client
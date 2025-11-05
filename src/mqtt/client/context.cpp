#include "context.h"
#include "command.h"
#include "mqtt/client/reactor.h"

#include "mqtt/packets/interface/control_packet.h"
#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/connect.h"
#include "mqtt/packets/conn_ack.h"
#include "mqtt/packets/publish.h"
#include "mqtt/packets/pub_ack.h"
#include "mqtt/packets/pub_rec.h"
#include "mqtt/packets/pub_rel.h"
#include "mqtt/packets/pub_comp.h"
#include "mqtt/packets/subscribe.h"
#include "mqtt/packets/sub_ack.h"
#include "mqtt/packets/unsubscribe.h"
#include "mqtt/packets/unsub_ack.h"
#include "mqtt/packets/ping_req.h"
#include "mqtt/packets/ping_resp.h"
#include "mqtt/packets/disconnect.h"
#include "mqtt/packets/auth.h"
#include "serialize/bytes.h"

#include <util/logging/logging.h>

namespace reactormq::mqtt::client
{
    Context::Context(ConnectionSettingsPtr settings)
        : m_settings(std::move(settings))
    {
    }

    std::uint16_t Context::allocatePacketId()
    {
        std::scoped_lock lock(packetIdMutex);
        constexpr std::uint16_t kMaxPacketId = 65535;

        if (m_packetIdsInUse.size() >= kMaxPacketId)
        {
            return 0;
        }

        std::uint16_t attempts = 0;
        while (attempts < kMaxPacketId)
        {
            constexpr std::uint16_t kMinPacketId = 1;
            if (!m_packetIdsInUse.contains(m_nextPacketId))
            {
                const std::uint16_t allocatedId = m_nextPacketId;
                m_packetIdsInUse.insert(allocatedId);

                ++m_nextPacketId;
                if (m_nextPacketId == 0)
                {
                    m_nextPacketId = kMinPacketId;
                }

                return allocatedId;
            }

            ++m_nextPacketId;
            if (m_nextPacketId == 0)
            {
                m_nextPacketId = kMinPacketId;
            }

            ++attempts;
        }

        return 0;
    }

    void Context::releasePacketId(const std::uint16_t packetId)
    {
        std::scoped_lock lock(packetIdMutex);
        m_packetIdsInUse.erase(packetId);
    }

    bool Context::isPacketIdInUse(const std::uint16_t packetId) const
    {
        std::scoped_lock lock(packetIdMutex);
        return m_packetIdsInUse.contains(packetId);
    }

    void Context::storePendingPublish(const std::uint16_t packetId, PublishCommand command)
    {
        m_pendingPublishes.emplace(packetId, std::move(command));
    }

    std::optional<PublishCommand> Context::takePendingPublish(const std::uint16_t packetId)
    {
        const auto it = m_pendingPublishes.find(packetId);
        if (it == m_pendingPublishes.end())
        {
            return std::nullopt;
        }

        PublishCommand command = std::move(it->second);
        m_pendingPublishes.erase(it);
        return command;
    }

    void Context::storePendingSubscribe(const std::uint16_t packetId, SubscribeCommand command)
    {
        m_pendingSubscribes.emplace(packetId, std::move(command));
    }

    std::optional<SubscribeCommand> Context::takePendingSubscribe(const std::uint16_t packetId)
    {
        const auto it = m_pendingSubscribes.find(packetId);
        if (it == m_pendingSubscribes.end())
        {
            return std::nullopt;
        }

        SubscribeCommand command = std::move(it->second);
        m_pendingSubscribes.erase(it);
        return command;
    }

    void Context::storePendingSubscribes(const std::uint16_t packetId, SubscribesCommand command)
    {
        m_pendingSubscribesMulti.emplace(packetId, std::move(command));
    }

    std::optional<SubscribesCommand> Context::takePendingSubscribes(const std::uint16_t packetId)
    {
        const auto it = m_pendingSubscribesMulti.find(packetId);
        if (it == m_pendingSubscribesMulti.end())
        {
            return std::nullopt;
        }

        SubscribesCommand command = std::move(it->second);
        m_pendingSubscribesMulti.erase(it);
        return command;
    }

    void Context::storePendingUnsubscribes(const std::uint16_t packetId, UnsubscribesCommand command)
    {
        m_pendingUnsubscribes.emplace(packetId, std::move(command));
    }

    std::optional<UnsubscribesCommand> Context::takePendingUnsubscribes(const std::uint16_t packetId)
    {
        const auto it = m_pendingUnsubscribes.find(packetId);
        if (it == m_pendingUnsubscribes.end())
        {
            return std::nullopt;
        }

        UnsubscribesCommand command = std::move(it->second);
        m_pendingUnsubscribes.erase(it);
        return command;
    }

    void Context::storePendingIncomingQos2Message(const std::uint16_t packetId, Message message)
    {
        m_pendingIncomingQos2Messages.emplace(packetId, std::move(message));
    }

    std::optional<Message> Context::takePendingIncomingQos2Message(const std::uint16_t packetId)
    {
        const auto it = m_pendingIncomingQos2Messages.find(packetId);
        if (it == m_pendingIncomingQos2Messages.end())
        {
            return std::nullopt;
        }

        Message message = std::move(it->second);
        m_pendingIncomingQos2Messages.erase(it);
        return message;
    }

    std::unique_ptr<packets::IControlPacket> Context::parsePacket(const std::uint8_t* data, const std::uint32_t size) const
    {
        if (data == nullptr)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to parse packet: null data pointer");
            return nullptr;
        }

        if (size < 2)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to parse packet: insufficient data (size: %u)", size);
            return nullptr;
        }

        if (m_settings && m_settings->shouldEnforceMaxPacketSize())
        {
            const std::uint32_t maxPacketSize = m_settings->getMaxPacketSize();
            if (size > maxPacketSize)
            {
                REACTORMQ_LOG(
                    reactormq::logging::LogLevel::Error,
                    "Packet size (%u bytes) exceeds maximum allowed (%u bytes)", size, maxPacketSize);
                return nullptr;
            }
        }

        serialize::ByteReader reader(reinterpret_cast<const std::byte*>(data), size);
        packets::FixedHeader fixedHeader;
        fixedHeader.decode(reader);

        const packets::PacketType packetType = fixedHeader.getPacketType();
        const packets::ProtocolVersion protocolVersion = getProtocolVersion();

        std::unique_ptr<packets::IControlPacket> result = nullptr;

        switch (packetType)
        {
        case packets::PacketType::Connect: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::Connect<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::Connect<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::ConnAck: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::ConnAck<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::ConnAck<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::Publish: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::Publish<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::Publish<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::PubAck: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::PubAck<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::PubAck<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::PubRec: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::PubRec<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::PubRec<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::PubRel: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::PubRel<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::PubRel<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::PubComp: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::PubComp<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::PubComp<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::Subscribe: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::Subscribe<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::Subscribe<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::SubAck: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::SubAck<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::SubAck<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::Unsubscribe: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::Unsubscribe<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::Unsubscribe<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::UnsubAck: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::UnsubAck<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::UnsubAck<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::PingReq: {
            result = std::make_unique<packets::PingReq>(fixedHeader);
            break;
        }

        case packets::PacketType::PingResp: {
            result = std::make_unique<packets::PingResp>(fixedHeader);
            break;
        }

        case packets::PacketType::Disconnect: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::Disconnect<packets::ProtocolVersion::V5>>(reader, fixedHeader);
            }
            else
            {
                result = std::make_unique<packets::Disconnect<packets::ProtocolVersion::V311>>(reader, fixedHeader);
            }
            break;
        }

        case packets::PacketType::Auth: {
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                result = std::make_unique<packets::Auth>(reader, fixedHeader);
            }
            else
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "AUTH packet not supported in MQTT 3.1.1");
                return nullptr;
            }
            break;
        }

        case packets::PacketType::None:
        case packets::PacketType::Max:
        default: {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Unknown or invalid packet type: %d", static_cast<int>(packetType));
            return nullptr;
        }
        }

        if (result == nullptr)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to create packet of type: %d", static_cast<int>(packetType));
            return nullptr;
        }

        if (!result->isValid())
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Malformed packet of type: %d", static_cast<int>(packetType));
            return nullptr;
        }

        REACTORMQ_LOG(
            reactormq::logging::LogLevel::Debug, "Received packet - Type: %d, PacketId: %u",
            static_cast<int>(result->getPacketType()),
            result->getPacketId());

        return result;
    }

    void Context::recordActivity()
    {
        m_lastActivityTime = std::chrono::steady_clock::now();
    }

    std::chrono::milliseconds Context::getTimeSinceLastActivity() const
    {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastActivityTime);
    }

    void Context::recordPublishSent(const std::uint16_t packetId)
    {
        m_publishSentTimes[packetId] = std::chrono::steady_clock::now();
    }

    std::chrono::milliseconds Context::getPublishElapsedTime(const std::uint16_t packetId) const
    {
        const auto it = m_publishSentTimes.find(packetId);
        if (it == m_publishSentTimes.end())
        {
            return std::chrono::milliseconds(0);
        }

        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
    }

    void Context::clearPublishTimeout(const std::uint16_t packetId)
    {
        m_publishSentTimes.erase(packetId);
    }

    void Context::retransmitPendingPublishes()
    {
        if (!m_socket)
        {
            return;
        }

        for (auto& [packetId, publishCmd] : m_pendingPublishes)
        {
            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);

            const auto& message = publishCmd.message;
            const auto qos = message.getQualityOfService();

            if (m_protocolVersion == packets::ProtocolVersion::V5)
            {
                packets::Publish<packets::ProtocolVersion::V5> publishPacket(
                    message.getTopic(),
                    message.getPayload(),
                    qos,
                    message.shouldRetain(),
                    packetId,
                    {}, // properties
                    true // isDuplicate - set to true for retransmission
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
                    true // isDuplicate - set to true for retransmission
                    );
                publishPacket.encode(writer);
            }

            m_socket->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

            recordPublishSent(packetId);
        }
    }

    bool Context::trackIncomingPacketId(const std::uint16_t packetId)
    {
        if (m_incomingPacketIds.contains(packetId))
        {
            return false; // Duplicate packet ID
        }

        m_incomingPacketIds.insert(packetId);
        return true;
    }

    void Context::releaseIncomingPacketId(const std::uint16_t packetId)
    {
        m_incomingPacketIds.erase(packetId);
    }

    bool Context::hasIncomingPacketId(const std::uint16_t packetId) const
    {
        return m_incomingPacketIds.contains(packetId);
    }

    std::size_t Context::getPendingCommandCount() const
    {
        return m_pendingPublishes.size() +
            m_pendingSubscribes.size() +
            m_pendingSubscribesMulti.size() +
            m_pendingUnsubscribes.size();
    }

    bool Context::canAddPendingCommand() const
    {
        if (!m_settings)
        {
            return true; // No limit if settings unavailable
        }

        const std::size_t maxCommands = m_settings->getMaxPendingCommands();
        const std::size_t currentCount = getPendingCommandCount();

        return currentCount < maxCommands;
    }
} // namespace reactormq::mqtt::client
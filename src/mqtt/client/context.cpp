//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "context.h"

#include "command.h"
#include "mqtt/client/reactor.h"
#include "mqtt/packets/auth.h"
#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet.h"
#include "mqtt_version_mapping.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"

#include <span>

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
        m_pendingPublishes.try_emplace(packetId, std::move(command));
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
        m_pendingSubscribes.try_emplace(packetId, std::move(command));
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
        m_pendingSubscribesMulti.try_emplace(packetId, std::move(command));
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
        m_pendingUnsubscribes.try_emplace(packetId, std::move(command));
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
        m_pendingIncomingQos2Messages.try_emplace(packetId, std::move(message));
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

    std::unique_ptr<packets::IControlPacket> Context::parsePacket(const std::span<const std::byte> data) const
    {
        if (data.size() < 2)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to parse packet: insufficient data (size: %llu)", data.size());
            return nullptr;
        }

        if (m_settings && m_settings->shouldEnforceMaxPacketSize())
        {
            const std::uint32_t maxPacketSize = m_settings->getMaxPacketSize();
            if (data.size() > maxPacketSize)
            {
                REACTORMQ_LOG(
                    reactormq::logging::LogLevel::Error,
                    "Packet size (%llu bytes) exceeds maximum allowed (%u bytes)",
                    data.size(),
                    maxPacketSize);
                return nullptr;
            }
        }

        serialize::ByteReader reader(data);
        packets::FixedHeader fixedHeader;
        fixedHeader.decode(reader);

        const packets::PacketType packetType = fixedHeader.getPacketType();
        const packets::ProtocolVersion protocolVersion = getProtocolVersion();

        std::unique_ptr<packets::IControlPacket> result = withMqttVersion(
            protocolVersion,
            [&reader, &fixedHeader, &packetType]<typename VersionTag>(VersionTag) -> std::unique_ptr<packets::IControlPacket>
            {
                constexpr auto kV = VersionTag::value;
                return parsePacketImpl<kV>(reader, fixedHeader, packetType);
            });

        if (result == nullptr)
        {
            // Preserve previous early-return behavior for unsupported AUTH on MQTT 3.1.1
            if (packetType == packets::PacketType::Auth && protocolVersion == packets::ProtocolVersion::V311)
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "AUTH packet not supported in MQTT 3.1.1");
                return nullptr;
            }

            // Preserve previous early-return behavior for unknown/invalid types (already logged in impl)
            if (packetType == packets::PacketType::None || packetType == packets::PacketType::Max)
            {
                return nullptr;
            }

            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Failed to create packet of type: %d", packetType);
            return nullptr;
        }

        if (!result->isValid())
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Malformed packet of type: %d", packetType);
            return nullptr;
        }

        REACTORMQ_LOG(
            reactormq::logging::LogLevel::Debug,
            "Received packet - Type: %d, PacketId: %u",
            result->getPacketType(),
            result->getPacketId());

        return result;
    }

    std::unique_ptr<packets::IControlPacket> Context::parsePacket(const std::uint8_t* data, const std::uint32_t size) const
    {
        if (nullptr == data || 0 == size)
        {
            return nullptr;
        }

        const std::span u8{ data, size };
        const std::span<const std::byte> bytes = std::as_bytes(u8);

        return parsePacket(bytes);
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

        for (auto const& [packetId, publishCmd] : m_pendingPublishes)
        {
            auto buffer = encodePendingPublish(publishCmd, packetId);
            sendEncodedPublish(buffer);
            recordPublishSent(packetId);
        }
    }

    template<typename VersionTag, typename Message>
    void Context::encodePublish(Message const& message, std::uint16_t packetId, serialize::ByteWriter& writer) const
    {
        constexpr auto kV = VersionTag::value;
        PublishEncoder<kV>::encode(message, packetId, writer);
    }

    void Context::sendEncodedPublish(std::vector<std::byte> const& buffer) const
    {
        if (!m_socket || buffer.empty())
        {
            return;
        }

        const std::span bytes = buffer;
        m_socket->send(bytes);
    }

    std::vector<std::byte> Context::encodePendingPublish(PublishCommand const& publishCmd, const std::uint16_t packetId) const
    {
        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        encodePublishForCurrentVersion(publishCmd.message, packetId, writer);
        return buffer;
    }

    void Context::encodePublishForCurrentVersion(Message const& message, const std::uint16_t packetId, serialize::ByteWriter& writer) const
    {
        withMqttVersion(
            m_protocolVersion,
            [&message, &packetId, &writer, this]<typename VersionTag>(VersionTag)
            {
                encodePublish<VersionTag>(message, packetId, writer);
            });
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

    size_t Context::getPendingCommandCount() const
    {
        return m_pendingPublishes.size() + m_pendingSubscribes.size() + m_pendingSubscribesMulti.size() + m_pendingUnsubscribes.size();
    }

    bool Context::canAddPendingCommand() const
    {
        if (!m_settings)
        {
            return true; // No limit if settings unavailable
        }

        const size_t maxCommands = m_settings->getMaxPendingCommands();
        const size_t currentCount = getPendingCommandCount();

        return currentCount < maxCommands;
    }
} // namespace reactormq::mqtt::client
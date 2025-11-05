#pragma once

#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/delegates.h"
#include "reactormq/mqtt/message.h"
#include "reactormq/mqtt/protocol_version.h"
#include "socket/socket_base.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace reactormq::mqtt::packets
{
    class IControlPacket;
}

namespace reactormq::mqtt::client
{
    struct PublishCommand;
    struct SubscribeCommand;
    struct SubscribesCommand;
    struct UnsubscribesCommand;
    class Reactor;

    /**
     * @brief Shared context containing socket, settings, packet ID management,
     * queues, and callbacks. Accessed only on the reactor thread.
     */
    class Context
    {
    public:
        /**
         * @brief Constructor.
         * @param settings Connection settings for the MQTT client.
         */
        explicit Context(ConnectionSettingsPtr settings);

        /**
         * @brief Get the socket instance.
         * @return Shared pointer to the socket.
         */
        [[nodiscard]] socket::SocketPtr getSocket() const
        {
            return m_socket;
        }

        /**
         * @brief Set the socket instance.
         * @param socket New socket instance.
         */
        void setSocket(socket::SocketPtr socket)
        {
            m_socket = std::move(socket);
        }

        /**
         * @brief Get the connection settings.
         * @return Shared pointer to connection settings.
         */
        [[nodiscard]] ConnectionSettingsPtr getSettings() const
        {
            return m_settings;
        }

        /**
         * @brief Get the negotiated protocol version.
         * @return Protocol version.
         */
        [[nodiscard]] packets::ProtocolVersion getProtocolVersion() const
        {
            return m_protocolVersion;
        }

        /**
         * @brief Set the negotiated protocol version.
         * @param version Protocol version.
         */
        void setProtocolVersion(const packets::ProtocolVersion version)
        {
            m_protocolVersion = version;
        }

        /**
         * @brief Allocate a packet ID from the pool (1-65535).
         * @return Packet ID, or 0 if the pool is exhausted.
         */
        std::uint16_t allocatePacketId();

        /**
         * @brief Release a packet ID back to the pool.
         * @param packetId The packet ID to release.
         */
        void releasePacketId(std::uint16_t packetId);

        /**
         * @brief Check if a packet ID is currently in use.
         * @param packetId The packet ID to check.
         * @return True if in use, false otherwise.
         */
        [[nodiscard]] bool isPacketIdInUse(std::uint16_t packetId) const;

        /**
         * @brief Get the current size of the outbound queue in bytes.
         * @return Size in bytes.
         */
        [[nodiscard]] std::size_t getOutboundQueueSize() const
        {
            return m_outboundQueueSize;
        }

        /**
         * @brief Check if adding bytes to the outbound queue would exceed the limit.
         * @param bytes Number of bytes to potentially add.
         * @return True if adding would stay within limit, false if it would exceed.
         */
        [[nodiscard]] bool canAddToOutboundQueue(std::size_t bytes) const
        {
            if (!m_settings)
            {
                return true;
            }
            const std::size_t maxBytes = m_settings->getMaxOutboundQueueBytes();
            return (m_outboundQueueSize + bytes) <= maxBytes;
        }

        /**
         * @brief Add to the outbound queue size.
         * @param bytes Number of bytes to add.
         */
        void addOutboundQueueSize(std::size_t bytes)
        {
            m_outboundQueueSize += bytes;
        }

        /**
         * @brief Subtract from the outbound queue size.
         * @param bytes Number of bytes to subtract.
         */
        void subtractOutboundQueueSize(std::size_t bytes)
        {
            m_outboundQueueSize = (bytes > m_outboundQueueSize) ? 0 : m_outboundQueueSize - bytes;
        }

        /**
         * @brief Access the onConnect delegate.
         * @return Reference to the delegate.
         */
        OnConnect& getOnConnect()
        {
            return m_onConnect;
        }

        /**
         * @brief Access the onDisconnect delegate.
         * @return Reference to the delegate.
         */
        OnDisconnect& getOnDisconnect()
        {
            return m_onDisconnect;
        }

        /**
         * @brief Access the onPublish delegate.
         * @return Reference to the delegate.
         */
        OnPublish& getOnPublish()
        {
            return m_onPublish;
        }

        /**
         * @brief Access the onSubscribe delegate.
         * @return Reference to the delegate.
         */
        OnSubscribe& getOnSubscribe()
        {
            return m_onSubscribe;
        }

        /**
         * @brief Access the onUnsubscribe delegate.
         * @return Reference to the delegate.
         */
        OnUnsubscribe& getOnUnsubscribe()
        {
            return m_onUnsubscribe;
        }

        /**
         * @brief Access the onMessage delegate.
         * @return Reference to the delegate.
         */
        OnMessage& getOnMessage()
        {
            return m_onMessage;
        }

        /**
         * @brief Invoke a callback, either immediately or via the executor.
         * 
         * If ConnectionSettings has a CallbackExecutor set, the callback is marshalled.
         * Otherwise, it executes immediately on the reactor thread.
         * 
         * @tparam Callback The callback type (typically a lambda).
         * @param callback The callback to invoke.
         */
        template<typename Callback>
        void invokeCallback(Callback&& callback)
        {
            if (m_settings)
            {
                const auto& executor = m_settings->getCallbackExecutor();
                if (executor)
                {
                    executor(std::forward<Callback>(callback));
                    return;
                }
            }
            
            std::forward<Callback>(callback)();
        }

        /**
         * @brief Parse a complete MQTT packet from raw bytes.
         * 
         * This helper function decodes the fixed header, determines the packet type,
         * and constructs the appropriate packet object based on the protocol version.
         * 
         * @param data Pointer to the raw packet data.
         * @param size Size of the data buffer in bytes.
         * @return Unique pointer to the parsed packet, or nullptr on error.
         */
        [[nodiscard]] std::unique_ptr<packets::IControlPacket> parsePacket(const std::uint8_t* data, std::uint32_t size) const;

        /**
         * @brief Store a pending publish command indexed by packet ID.
         * @param packetId The packet ID.
         * @param command The publish command to store.
         */
        void storePendingPublish(std::uint16_t packetId, PublishCommand command);

        /**
         * @brief Retrieve and remove a pending publish command by packet ID.
         * @param packetId The packet ID.
         * @return Optional containing the command if found.
         */
        std::optional<PublishCommand> takePendingPublish(std::uint16_t packetId);

        /**
         * @brief Get all pending publish commands (for timeout checking).
         * @return Const reference to the map of pending publishes.
         */
        [[nodiscard]] const std::unordered_map<std::uint16_t, PublishCommand>& getPendingPublishes() const
        {
            return m_pendingPublishes;
        }

        /**
         * @brief Store a pending subscribe command indexed by packet ID.
         * @param packetId The packet ID.
         * @param command The subscribe command to store.
         */
        void storePendingSubscribe(std::uint16_t packetId, SubscribeCommand command);

        /**
         * @brief Retrieve and remove a pending subscribe command by packet ID.
         * @param packetId The packet ID.
         * @return Optional containing the command if found.
         */
        std::optional<SubscribeCommand> takePendingSubscribe(std::uint16_t packetId);

        /**
         * @brief Store a pending subscribes command indexed by packet ID.
         * @param packetId The packet ID.
         * @param command The subscribes command to store.
         */
        void storePendingSubscribes(std::uint16_t packetId, SubscribesCommand command);

        /**
         * @brief Retrieve and remove a pending subscribes command by packet ID.
         * @param packetId The packet ID.
         * @return Optional containing the command if found.
         */
        std::optional<SubscribesCommand> takePendingSubscribes(std::uint16_t packetId);

        /**
         * @brief Store a pending unsubscribes command indexed by packet ID.
         * @param packetId The packet ID.
         * @param command The unsubscribes command to store.
         */
        void storePendingUnsubscribes(std::uint16_t packetId, UnsubscribesCommand command);

        /**
         * @brief Retrieve and remove a pending unsubscribes command by packet ID.
         * @param packetId The packet ID.
         * @return Optional containing the command if found.
         */
        std::optional<UnsubscribesCommand> takePendingUnsubscribes(std::uint16_t packetId);

        /**
         * @brief Store an incoming QoS 2 message that has been PUBREC'd and is awaiting PUBREL.
         * @param packetId The packet ID.
         * @param message The incoming message to store.
         */
        void storePendingIncomingQos2Message(std::uint16_t packetId, Message message);

        /**
         * @brief Retrieve and remove an incoming QoS 2 message by packet ID (when PUBREL arrives).
         * @param packetId The packet ID.
         * @return Optional containing the message if found.
         */
        std::optional<Message> takePendingIncomingQos2Message(std::uint16_t packetId);

        /**
         * @brief Record activity (sent or received data).
         * Updates last activity time for keepalive tracking.
         */
        void recordActivity();

        /**
         * @brief Get the time since last activity.
         * @return Duration since last activity.
         */
        [[nodiscard]] std::chrono::milliseconds getTimeSinceLastActivity() const;

        /**
         * @brief Check if a PINGREQ is currently pending.
         * @return True if awaiting PINGRESP.
         */
        [[nodiscard]] bool isPingPending() const
        {
            return m_pingPending;
        }

        /**
         * @brief Set whether a PINGREQ is pending.
         * @param pending True if awaiting PINGRESP.
         */
        void setPingPending(bool pending)
        {
            m_pingPending = pending;
        }

        /**
         * @brief Record the time when a QoS 1/2 publish was sent.
         * @param packetId The packet ID for the publish.
         */
        void recordPublishSent(std::uint16_t packetId);

        /**
         * @brief Get the elapsed time since a publish was sent.
         * @param packetId The packet ID for the publish.
         * @return Elapsed time in milliseconds, or 0 if not found.
         */
        [[nodiscard]] std::chrono::milliseconds getPublishElapsedTime(std::uint16_t packetId) const;

        /**
         * @brief Clear the timeout tracking for a publish operation.
         * @param packetId The packet ID for the publish.
         */
        void clearPublishTimeout(std::uint16_t packetId);

        /**
         * @brief Retransmit all pending QoS 1/2 publishes with DUP flag set.
         * Called when reconnecting to resume in-flight publishes.
         */
        void retransmitPendingPublishes();

        /**
         * @brief Track an incoming QoS 1/2 packet ID to detect duplicates.
         * @param packetId The packet ID to track.
         * @return True if tracking succeeded (ID not already in use), false if duplicate detected.
         */
        bool trackIncomingPacketId(std::uint16_t packetId);

        /**
         * @brief Release an incoming packet ID after QoS flow completes.
         * @param packetId The packet ID to release.
         */
        void releaseIncomingPacketId(std::uint16_t packetId);

        /**
         * @brief Check if an incoming packet ID is currently being tracked.
         * @param packetId The packet ID to check.
         * @return True if the ID is in use.
         */
        [[nodiscard]] bool hasIncomingPacketId(std::uint16_t packetId) const;

        /**
         * @brief Get the current count of pending acknowledgeable commands.
         * Includes QoS 1/2 publishes, subscribes, and unsubscribes awaiting acknowledgment.
         * @return The number of pending commands.
         */
        [[nodiscard]] std::size_t getPendingCommandCount() const;

        /**
         * @brief Check if adding a new pending command would exceed the limit.
         * @return True if a new command can be added, false if limit would be exceeded.
         */
        [[nodiscard]] bool canAddPendingCommand() const;

    private:
        /**
         * @brief Socket instance.
         */
        socket::SocketPtr m_socket;

        /**
         * @brief Connection settings.
         */
        ConnectionSettingsPtr m_settings;

        /**
         * @brief Negotiated protocol version.
         */
        packets::ProtocolVersion m_protocolVersion = packets::ProtocolVersion::V5;

        /**
         * @brief Next packet ID to try allocating (circular pool).
         */
        std::uint16_t m_nextPacketId = 1;

        mutable std::mutex packetIdMutex;
        /**
         * @brief Set of packet IDs currently in use.
         */
        std::unordered_set<std::uint16_t> m_packetIdsInUse;

        /**
         * @brief Current size of the outbound queue in bytes.
         */
        std::size_t m_outboundQueueSize = 0;

        /**
         * @brief Event delegates.
         */
        OnConnect m_onConnect;
        OnDisconnect m_onDisconnect;
        OnPublish m_onPublish;
        OnSubscribe m_onSubscribe;
        OnUnsubscribe m_onUnsubscribe;
        OnMessage m_onMessage;

        /**
         * @brief Maps for tracking pending operations by packet ID.
         */
        std::unordered_map<std::uint16_t, PublishCommand> m_pendingPublishes;
        std::unordered_map<std::uint16_t, SubscribeCommand> m_pendingSubscribes;
        std::unordered_map<std::uint16_t, SubscribesCommand> m_pendingSubscribesMulti;
        std::unordered_map<std::uint16_t, UnsubscribesCommand> m_pendingUnsubscribes;

        /**
         * @brief Map for tracking incoming QoS 2 messages that have been PUBREC'd and are awaiting PUBREL.
         */
        std::unordered_map<std::uint16_t, Message> m_pendingIncomingQos2Messages;

        /**
         * @brief Last activity time (send or receive).
         */
        std::chrono::steady_clock::time_point m_lastActivityTime = std::chrono::steady_clock::now();

        /**
         * @brief Flag indicating if PINGREQ is pending.
         */
        bool m_pingPending = false;

        /**
         * @brief Map tracking sent times for QoS 1/2 publish operations (for timeout detection).
         */
        std::unordered_map<std::uint16_t, std::chrono::steady_clock::time_point> m_publishSentTimes;

        /**
         * @brief Set of incoming packet IDs currently being tracked (for duplicate detection).
         */
        std::unordered_set<std::uint16_t> m_incomingPacketIds;
    };

    } // namespace reactormq::mqtt::client

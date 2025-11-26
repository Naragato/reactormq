//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/delegates.h"
#include "reactormq/mqtt/message.h"
#include "reactormq/mqtt/protocol_version.h"
#include "serialize/bytes.h"
#include "socket/socket.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace reactormq::mqtt::packets
{
    class IControlPacket;

    template<ProtocolVersion V>
    class Connect;
    template<ProtocolVersion V>
    class ConnAck;
    template<ProtocolVersion V>
    class Publish;
    template<ProtocolVersion V>
    class PubAck;
    template<ProtocolVersion V>
    class PubRec;
    template<ProtocolVersion V>
    class PubRel;
    template<ProtocolVersion V>
    class PubComp;
    template<ProtocolVersion V>
    class Subscribe;
    template<ProtocolVersion V>
    class SubAck;
    template<ProtocolVersion V>
    class Unsubscribe;
    template<ProtocolVersion V>
    class UnsubAck;
    template<ProtocolVersion V>
    class Disconnect;
} // namespace reactormq::mqtt::packets

namespace reactormq::mqtt::client
{
    struct PublishCommand;
    struct SubscribeCommand;
    struct SubscribesCommand;
    struct UnsubscribesCommand;
    class Reactor;

    /**
     * @brief Internal delegate invoked when the socket is replaced.
     * Notifies the reactor to rebind socket callbacks.
     */
    using OnSocketReplaced = MulticastDelegate<void()>;

    /**
     * @brief Shared MQTT client state used by the reactor thread.
     * Holds the socket, connection settings, ID pools, queues, and user callbacks.
     */
    class Context
    {
    public:
        /**
         * @brief Construct a context bound to connection settings.
         * @param settings Connection settings for the client.
         */
        explicit Context(ConnectionSettingsPtr settings);

        /// @brief Access the socket instance.
        [[nodiscard]] socket::SocketPtr getSocket() const
        {
            return m_socket;
        }

        /**
         * @brief Replace the socket instance.
         * @param socket New socket to use.
         */
        void setSocket(socket::SocketPtr socket)
        {
            m_socket = std::move(socket);
            m_onSocketReplaced.broadcast();
        }

        /// @brief Access the connection settings.
        [[nodiscard]] ConnectionSettingsPtr getSettings() const
        {
            return m_settings;
        }

        /// @brief Get the negotiated protocol version.
        [[nodiscard]] packets::ProtocolVersion getProtocolVersion() const
        {
            return m_protocolVersion;
        }

        /// @brief Update the negotiated protocol version.
        void setProtocolVersion(const packets::ProtocolVersion version)
        {
            m_protocolVersion = version;
        }

        /**
         * @brief Allocate a packet ID from the pool (1-65535).
         * @return ID, or 0 if the pool is exhausted.
         */
        std::uint16_t allocatePacketId();

        /// @brief Release a packet ID back to the pool.
        void releasePacketId(std::uint16_t packetId);

        /// @brief Check whether a packet ID is currently in use.
        [[nodiscard]] bool isPacketIdInUse(std::uint16_t packetId) const;

        /// @brief Current size of the outbound queue in bytes.
        [[nodiscard]] size_t getOutboundQueueSize() const
        {
            return m_outboundQueueSize;
        }

        /**
         * @brief Check whether adding bytes would exceed the outbound limit.
         * @param bytes Number of bytes to potentially add.
         * @return True if within limit; false if it would exceed.
         */
        [[nodiscard]] bool canAddToOutboundQueue(const size_t bytes) const
        {
            if (!m_settings)
            {
                return true;
            }
            const size_t maxBytes = m_settings->getMaxOutboundQueueBytes();
            return m_outboundQueueSize + bytes <= maxBytes;
        }

        /// @brief Increase the outbound queue size by the given bytes.
        void addOutboundQueueSize(const size_t bytes)
        {
            m_outboundQueueSize += bytes;
        }

        /// @brief Decrease the outbound queue size by the given bytes.
        void subtractOutboundQueueSize(const size_t bytes)
        {
            m_outboundQueueSize = bytes > m_outboundQueueSize ? 0 : m_outboundQueueSize - bytes;
        }

        /// @brief Access the onConnect delegate.
        OnConnect& getOnConnect()
        {
            return m_onConnect;
        }

        /// @brief Access the onDisconnect delegate.
        OnDisconnect& getOnDisconnect()
        {
            return m_onDisconnect;
        }

        /// @brief Access the onPublish delegate.
        OnPublish& getOnPublish()
        {
            return m_onPublish;
        }

        /// @brief Access the onSubscribe delegate.
        OnSubscribe& getOnSubscribe()
        {
            return m_onSubscribe;
        }

        /// @brief Access the onUnsubscribe delegate.
        OnUnsubscribe& getOnUnsubscribe()
        {
            return m_onUnsubscribe;
        }

        /// @brief Access the onMessage delegate.
        OnMessage& getOnMessage()
        {
            return m_onMessage;
        }

        /// @brief Access the onSocketReplaced delegate (internal use by reactor).
        OnSocketReplaced& getOnSocketReplaced()
        {
            return m_onSocketReplaced;
        }

        /**
         * @brief Invoke a callback immediately or via the settings-provided executor.
         * If a CallbackExecutor is set, the call is marshalled to it; otherwise it runs on the reactor thread.
         * @tparam Callback Callable type (typically a lambda).
         * @param callback The callback to invoke.
         */
        template<typename Callback>
        void invokeCallback(Callback&& callback) const
        {
            if (m_settings)
            {
                if (const auto& executor = m_settings->getCallbackExecutor())
                {
                    executor(std::forward<Callback>(callback));
                    return;
                }
            }

            std::forward<Callback>(callback)();
        }

        /**
         * @brief Parse a complete MQTT control packet from raw bytes.
         * Returns a concrete packet instance or nullptr on parse failure.
         * @param data Pointer to the raw packet data.
         * @param size Buffer size in bytes.
         * @return Parsed packet, or nullptr on error.
         */
        [[nodiscard]] std::unique_ptr<packets::IControlPacket> parsePacket(const std::uint8_t* data, std::uint32_t size) const;

        /**
         * @brief Parse a complete MQTT control packet from raw bytes.
         * Returns a concrete packet instance or nullptr on parse failure.
         * @param data The packet being parsed
         * @return Parsed packet, or nullptr on error.
         */
        [[nodiscard]] std::unique_ptr<packets::IControlPacket> parsePacket(std::span<const std::byte> data) const;

        /// @brief Store a pending publish command by packet ID.
        void storePendingPublish(std::uint16_t packetId, PublishCommand command);

        /// @brief Take and remove a pending publish command by packet ID.
        std::optional<PublishCommand> takePendingPublish(std::uint16_t packetId);

        /// @brief Read-only view of pending publish commands (used for timeouts).
        [[nodiscard]] const std::unordered_map<std::uint16_t, PublishCommand>& getPendingPublishes() const
        {
            return m_pendingPublishes;
        }

        /// @brief Store a pending subscribe command by packet ID.
        void storePendingSubscribe(std::uint16_t packetId, SubscribeCommand command);

        /// @brief Take and remove a pending subscribe command by packet ID.
        std::optional<SubscribeCommand> takePendingSubscribe(std::uint16_t packetId);

        /// @brief Store a pending 'subscribes' command by packet ID.
        void storePendingSubscribes(std::uint16_t packetId, SubscribesCommand command);

        /// @brief Take and remove a pending 'subscribes' command by packet ID.
        std::optional<SubscribesCommand> takePendingSubscribes(std::uint16_t packetId);

        /// @brief Store a pending 'unsubscribes' command by packet ID.
        void storePendingUnsubscribes(std::uint16_t packetId, UnsubscribesCommand command);

        /// @brief Take and remove a pending 'unsubscribes' command by packet ID.
        std::optional<UnsubscribesCommand> takePendingUnsubscribes(std::uint16_t packetId);

        /// @brief Store an incoming QoS 2 message awaiting PUBREL (after PUBREC).
        void storePendingIncomingQos2Message(std::uint16_t packetId, Message message);

        /// @brief Take and remove a pending QoS 2 message by packet ID (on PUBREL).
        std::optional<Message> takePendingIncomingQos2Message(std::uint16_t packetId);

        /// @brief Record activity for keepalive tracking.
        void recordActivity();

        /// @brief Time since last activity.
        [[nodiscard]] std::chrono::milliseconds getTimeSinceLastActivity() const;

        /// @brief Whether a PINGREQ is currently pending.
        [[nodiscard]] bool isPingPending() const
        {
            return m_pingPending;
        }

        /// @brief Mark whether a PINGREQ is pending.
        void setPingPending(const bool pending)
        {
            m_pingPending = pending;
        }

        /// @brief Record when a QoS 1/2 publish was sent (for timeout tracking).
        void recordPublishSent(std::uint16_t packetId);

        /// @brief Elapsed time since a publish was sent, or 0 if unknown.
        [[nodiscard]] std::chrono::milliseconds getPublishElapsedTime(std::uint16_t packetId) const;

        /// @brief Clear timeout tracking for a publish.
        void clearPublishTimeout(std::uint16_t packetId);

        void encodePublishForCurrentVersion(Message const& message, std::uint16_t packetId, serialize::ByteWriter& writer) const;

        /// @brief Retransmit pending QoS 1/2 publishes with DUP set (on reconnect).
        void retransmitPendingPublishes();

        std::vector<std::byte> encodePendingPublish(PublishCommand const& publishCmd, std::uint16_t packetId) const;

        /// @brief encode packet to publish to the socket
        template<typename VersionTag, typename Message>
        void encodePublish(Message const& message, std::uint16_t packetId, serialize::ByteWriter& writer) const;

        /// @brief send packet to the socket
        void sendEncodedPublish(std::vector<std::byte> const& buffer) const;

        /// @brief Track an incoming QoS 1/2 packet ID; returns false if duplicate.
        bool trackIncomingPacketId(std::uint16_t packetId);

        /// @brief Release an incoming packet ID after the QoS flow completes.
        void releaseIncomingPacketId(std::uint16_t packetId);

        /// @brief Check whether an incoming packet ID is tracked.
        [[nodiscard]] bool hasIncomingPacketId(std::uint16_t packetId) const;

        /**
         * @brief Count of pending acknowledgeable commands.
         * Includes QoS 1/2 publishes, subscribes, and unsubscribes awaiting acknowledgment.
         */
        [[nodiscard]] size_t getPendingCommandCount() const;

        /// @brief Whether a new pending command would exceed the configured limit.
        [[nodiscard]] bool canAddPendingCommand() const;

        /**
         * @brief Get the assigned client identifier from the broker.
         * @return The broker-assigned client ID, or empty string if not assigned.
         */
        [[nodiscard]] const std::string& getAssignedClientId() const
        {
            return m_assignedClientId;
        }

        /**
         * @brief Set the assigned client identifier received from the broker.
         * @param clientId The client ID assigned by the broker.
         */
        void setAssignedClientId(std::string clientId)
        {
            m_assignedClientId = std::move(clientId);
        }

        /**
         * @brief Get the effective client identifier to use for connection.
         * Returns the broker-assigned ID if available, otherwise the user-supplied ID from settings.
         * @return The effective client ID.
         */
        [[nodiscard]] std::string getEffectiveClientId() const
        {
            if (!m_assignedClientId.empty())
            {
                return m_assignedClientId;
            }
            if (m_settings)
            {
                return m_settings->getClientId();
            }
            return std::string();
        }

    private:
        socket::SocketPtr m_socket;

        ConnectionSettingsPtr m_settings;

        packets::ProtocolVersion m_protocolVersion = packets::ProtocolVersion::V5;

        std::string m_assignedClientId;

        std::uint16_t m_nextPacketId = 1;

        mutable std::mutex packetIdMutex;

        std::unordered_set<std::uint16_t> m_packetIdsInUse;

        size_t m_outboundQueueSize = 0;

        OnConnect m_onConnect;
        OnDisconnect m_onDisconnect;
        OnPublish m_onPublish;
        OnSubscribe m_onSubscribe;
        OnUnsubscribe m_onUnsubscribe;
        OnMessage m_onMessage;
        OnSocketReplaced m_onSocketReplaced;

        std::unordered_map<std::uint16_t, PublishCommand> m_pendingPublishes; ///< Map for tracking pending publishes
        std::unordered_map<std::uint16_t, SubscribeCommand> m_pendingSubscribes;
        ///< Map for tracking pending subscribes
        std::unordered_map<std::uint16_t, SubscribesCommand> m_pendingSubscribesMulti;
        ///< Map for tracking pending multi-topic subscribes
        std::unordered_map<std::uint16_t, UnsubscribesCommand> m_pendingUnsubscribes;
        ///< Map for tracking pending unsubscribes

        /// @brief Map for tracking incoming QoS 2 messages that have been PUBREC'd and are awaiting PUBREL.
        std::unordered_map<std::uint16_t, Message> m_pendingIncomingQos2Messages;

        std::chrono::steady_clock::time_point m_lastActivityTime = std::chrono::steady_clock::now();

        bool m_pingPending = false;

        /// @brief Map tracking sent times for QoS 1/2 publish operations (for timeout detection).
        std::unordered_map<std::uint16_t, std::chrono::steady_clock::time_point> m_publishSentTimes;

        /// @brief Set of incoming packet IDs currently being tracked (for duplicate detection).
        std::unordered_set<std::uint16_t> m_incomingPacketIds;
    };
} // namespace reactormq::mqtt::client
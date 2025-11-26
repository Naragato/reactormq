//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <memory>
#include <string>

#include "reactormq/export.h"
#include "reactormq/mqtt/connection_protocol.h"
#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/credentials_provider.h"

namespace reactormq::mqtt
{
    /**
     * @brief Fluent builder for ConnectionSettings.
     *
     * Provides a chainable API to set connection parameters and produce an
     * immutable ConnectionSettings object with sensible defaults.
     */
    class REACTORMQ_API ConnectionSettingsBuilder final
    {
    public:
        /**
         * @brief Construct a builder
         */
        ConnectionSettingsBuilder() = default;

        explicit ConnectionSettingsBuilder(std::string host)
            : m_host(std::move(host))
        {
        }

        /**
         * @brief Set the host name or IP address.
         * @param host The host name or IP address.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setHost(std::string host)
        {
            m_host = std::move(host);
            return *this;
        }

        /**
         * @brief Set the port number for the connection.
         * @param port The port number.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setPort(const uint16_t port)
        {
            m_port = port;
            return *this;
        }

        /**
         * @brief Set the transport protocol.
         * @param protocol The transport: Tcp, Tls, Ws, or Wss.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setProtocol(const ConnectionProtocol protocol)
        {
            m_protocol = protocol;
            return *this;
        }

        /**
         * @brief Set the credentials provider.
         * @param provider Shared pointer to the credentials provider.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setCredentialsProvider(std::shared_ptr<ICredentialsProvider> provider)
        {
            m_credentialsProvider = std::move(provider);
            return *this;
        }

        /**
         * @brief Set the URI path (for WebSocket protocols).
         * @param path The path.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setPath(std::string path)
        {
            m_path = std::move(path);
            return *this;
        }

        /**
         * @brief Set the client ID.
         * @param clientId The unique client identifier.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setClientId(std::string clientId)
        {
            m_clientId = std::move(clientId);
            return *this;
        }

        /**
         * @brief Set the maximum packet size in bytes.
         * @param maxPacketSize The max packet size.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxPacketSize(const uint32_t maxPacketSize)
        {
            m_maxPacketSize = maxPacketSize;
            return *this;
        }

        /**
         * @brief Set the maximum inbound buffer size in bytes.
         * @param maxBufferSize The max buffer size.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxBufferSize(const uint32_t maxBufferSize)
        {
            m_maxBufferSize = maxBufferSize;
            return *this;
        }

        /**
         * @brief Set the maximum outbound queue size in bytes.
         * @param maxOutboundQueueBytes The max outbound queue size.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxOutboundQueueBytes(const uint32_t maxOutboundQueueBytes)
        {
            m_maxOutboundQueueBytes = maxOutboundQueueBytes;
            return *this;
        }

        /**
         * @brief Set the minimum packet retry interval in seconds.
         * @param seconds The packet retry interval.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setPacketRetryIntervalSeconds(const uint16_t seconds)
        {
            m_packetRetryIntervalSeconds = seconds;
            return *this;
        }

        /**
         * @brief Set the packet retry backoff multiplier.
         * @param multiplier The backoff multiplier.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setPacketRetryBackoffMultiplier(const double multiplier)
        {
            m_packetRetryBackoffMultiplier = multiplier;
            return *this;
        }

        /**
         * @brief Set the maximum packet retry interval in seconds.
         * @param seconds The max packet retry interval.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxPacketRetryIntervalSeconds(const uint16_t seconds)
        {
            m_maxPacketRetryIntervalSeconds = seconds;
            return *this;
        }

        /**
         * @brief Set the socket connection timeout in seconds.
         * @param seconds The socket connection timeout.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setSocketConnectionTimeoutSeconds(const uint16_t seconds)
        {
            m_socketConnectionTimeoutSeconds = seconds;
            return *this;
        }

        /**
         * @brief Set the MQTT keep-alive interval in seconds.
         * @param seconds The keep-alive interval.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setKeepAliveIntervalSeconds(const uint16_t seconds)
        {
            m_keepAliveIntervalSeconds = seconds;
            return *this;
        }

        /**
         * @brief Set the MQTT connection timeout in seconds.
         * @param seconds The MQTT connection timeout.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMqttConnectionTimeoutSeconds(const uint16_t seconds)
        {
            m_mqttConnectionTimeoutSeconds = seconds;
            return *this;
        }

        /**
         * @brief Set the initial retry interval for connection attempts in seconds.
         * @param seconds The initial retry interval.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setInitialRetryConnectionIntervalSeconds(const uint16_t seconds)
        {
            m_initialRetryConnectionIntervalSeconds = seconds;
            return *this;
        }

        /**
         * @brief Set the maximum number of connection retries.
         * @param retries The max connection retries.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxConnectionRetries(const uint8_t retries)
        {
            m_maxConnectionRetries = retries;
            return *this;
        }

        /**
         * @brief Set the maximum number of packet retries.
         * @param retries The max packet retries.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxPacketRetries(const uint8_t retries)
        {
            m_maxPacketRetries = retries;
            return *this;
        }

        /**
         * @brief Set whether to verify the server certificate for secure connections.
         * @param shouldVerify True to enable certificate verification.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setShouldVerifyServerCertificate(const bool shouldVerify)
        {
            m_shouldVerifyServerCertificate = shouldVerify;
            return *this;
        }

        /**
         * @brief Set the session expiry interval in seconds (MQTT 5).
         * @param interval The session expiry interval. 0 means session expires at disconnect.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setSessionExpiryInterval(const uint32_t interval)
        {
            m_sessionExpiryInterval = interval;
            return *this;
        }

        /**
         * @brief Enable or disable automatic reconnection after unexpected disconnection.
         * @param enabled True to enable auto-reconnect, false to disable.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setAutoReconnectEnabled(const bool enabled)
        {
            m_autoReconnectEnabled = enabled;
            return *this;
        }

        /**
         * @brief Set the initial delay before first reconnect attempt in milliseconds.
         * @param delayMs The initial reconnect delay in milliseconds.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setAutoReconnectInitialDelayMs(const uint32_t delayMs)
        {
            m_autoReconnectInitialDelayMs = delayMs;
            return *this;
        }

        /**
         * @brief Set the maximum delay between reconnect attempts in milliseconds.
         * @param delayMs The maximum reconnect delay in milliseconds.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setAutoReconnectMaxDelayMs(const uint32_t delayMs)
        {
            m_autoReconnectMaxDelayMs = delayMs;
            return *this;
        }

        /**
         * @brief Set the multiplier for exponential backoff between reconnect attempts.
         * @param multiplier The reconnect backoff multiplier.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setAutoReconnectMultiplier(const double multiplier)
        {
            m_autoReconnectMultiplier = multiplier;
            return *this;
        }

        /**
         * @brief Set strict error handling mode.
         * In strict mode, protocol violations cause immediate disconnect.
         * In lenient mode, violations are logged but processing continues.
         * @param strict True to enable strict mode.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setStrictMode(const bool strict)
        {
            m_strictMode = strict;
            return *this;
        }

        /**
         * @brief Set whether to enforce maxPacketSize limit on incoming packets.
         * @param enforce True to enforce the limit.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setEnforceMaxPacketSize(const bool enforce)
        {
            m_enforceMaxPacketSize = enforce;
            return *this;
        }

        /**
         * @brief Set the maximum number of inbound packets to process per tick.
         * @param maxPackets The max inbound packets per tick.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxInboundPacketsPerTick(const uint32_t maxPackets)
        {
            m_maxInboundPacketsPerTick = maxPackets;
            return *this;
        }

        /**
         * @brief Set the maximum number of pending acknowledgeable commands.
         * @param maxCommands The max pending commands limit.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setMaxPendingCommands(const uint32_t maxCommands)
        {
            m_maxPendingCommands = maxCommands;
            return *this;
        }

        /**
         * @brief Set the callback executor for marshalling callbacks to another thread.
         * Example usage:
         * **Custom thread pool:**
         * ```cpp
         * builder.setCallbackExecutor([&pool](auto cb) {
         *     pool.enqueue(std::move(cb));
         * });
         * ```
         * **Unreal Engine game thread:**
         * ```cpp
         * builder.setCallbackExecutor([](auto cb) {
         *     AsyncTask(ENamedThreads::GameThread, std::move(cb));
         * });
         * ```
         * **Qt main thread:**
         * ```cpp
         * builder.setCallbackExecutor([app](auto cb) {
         *     QMetaObject::invokeMethod(app, std::move(cb), Qt::QueuedConnection);
         * });
         * ```
         * @param executor Function that schedules callback execution. Pass nullptr for immediate execution (default).
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setCallbackExecutor(const CallbackExecutor& executor)
        {
            m_callbackExecutor = executor;
            return *this;
        }

        /**
         * @brief Set a custom SSL/TLS certificate verification callback.
         * This allows you to provide custom certificate verification logic during
         * the SSL/TLS handshake. The callback receives the result of OpenSSL's
         * built-in verification and the X509_STORE_CTX* (as void*) for inspection.
         * If not set (nullptr), a default verification callback is used that:
         * - Rejects self-signed certificates
         * - Accepts intermediate certificates despite verification failures
         * - Logs all verification attempts
         * @param callback Custom verification callback function, or nullptr for default behavior.
         * @return Reference to this builder for chaining.
         */
        ConnectionSettingsBuilder& setSslVerifyCallback(const SslVerifyCallback& callback)
        {
            m_sslVerifyCallback = callback;
            return *this;
        }

        /**
         * @brief Build the ConnectionSettings instance.
         * @return Shared a pointer to the constructed ConnectionSettings.
         */
        [[nodiscard]] std::shared_ptr<ConnectionSettings> build() const;

        /**
         * @brief Generate an MQTT client identifier.
         *
         * This helper creates a client id that combines machine-specific
         * information (e.g., hostname and process id) with an optional
         * random prefix to reduce the chance of collisions when multiple
         * clients connect from the same host.
         *
         * Using this function is optional for MQTT: if you pass an empty
         * client id to the broker (and the broker allows it), the broker
         * will generate a client id for you. However, supplying a stable
         * client id that you control is recommended when you want to
         * resume sessions, maintain subscriptions, or otherwise rely on
         * a persistent MQTT session state.
         *
         * @param withRandomPrefix If true, prepend a small (16-bit) random
         *        value to the generated id to further reduce collisions.
         * @return A generated client id string suitable for use in MQTT
         *         CONNECT packets.
         */
        static std::string generateClientId(bool withRandomPrefix);

    private:
        /// @brief MQTT host name or IP address (required).
        std::string m_host;

        /// @brief Port number for the connection (default: 1883 for MQTT, 8883 for MQTTS).
        uint16_t m_port = 1883;

        /// @brief Protocol to use for MQTT connection.
        ConnectionProtocol m_protocol = ConnectionProtocol::Tcp;

        /// @brief Provider to resolve credentials.
        std::shared_ptr<ICredentialsProvider> m_credentialsProvider;

        /// @brief URI path for the connection (for WebSocket protocols).
        std::string m_path;

        /// @brief Unique client identifier.
        std::string m_clientId;

        /// @brief Maximum size of a single MQTT packet in bytes.
        uint32_t m_maxPacketSize = 1 * 1024 * 1024;

        /// @brief Maximum inbound buffer size in bytes before parsing.
        uint32_t m_maxBufferSize = 64 * 1024 * 1024;

        /// @brief Maximum outbound queue size in bytes.
        uint32_t m_maxOutboundQueueBytes = 10 * 1024 * 1024;

        /// @brief Minimum seconds to wait before retrying a packet.
        uint16_t m_packetRetryIntervalSeconds = 5;

        /// @brief Backoff multiplier for packet retry interval.
        double m_packetRetryBackoffMultiplier = 1.5;

        /// @brief Maximum seconds to wait before retrying a packet.
        uint16_t m_maxPacketRetryIntervalSeconds = 60;

        /// @brief Timeout for socket connection attempts in seconds.
        uint16_t m_socketConnectionTimeoutSeconds = 30;

        /// @brief MQTT keep-alive interval in seconds.
        uint16_t m_keepAliveIntervalSeconds = 60;

        /// @brief Timeout for MQTT connection handshake in seconds.
        uint16_t m_mqttConnectionTimeoutSeconds = 30;

        /// @brief Initial retry interval for connection attempts in seconds.
        uint16_t m_initialRetryConnectionIntervalSeconds = 5;

        /// @brief Maximum number of connection retries before giving up.
        uint8_t m_maxConnectionRetries = 5;

        /// @brief Maximum number of times to retry sending a packet.
        uint8_t m_maxPacketRetries = 3;

        /// @brief Whether to verify the server certificate for secure connections.
        bool m_shouldVerifyServerCertificate = true;

        /// @brief Session expiry interval in seconds (MQTT 5).
        uint32_t m_sessionExpiryInterval = 0;

        /// @brief Whether to automatically reconnect after unexpected disconnection.
        bool m_autoReconnectEnabled = false;

        /// @brief Initial delay in milliseconds before first reconnect attempt.
        uint32_t m_autoReconnectInitialDelayMs = 1000;

        /// @brief Maximum delay in milliseconds between reconnect attempts.
        uint32_t m_autoReconnectMaxDelayMs = 60000;

        /// @brief Multiplier for exponential backoff between reconnect attempts.
        double m_autoReconnectMultiplier = 2.0;

        /// @brief Whether to use strict error handling (disconnect on violations) vs lenient (warn/drop).
        bool m_strictMode = false;

        /// @brief Whether to enforce maxPacketSize limit on incoming packets.
        bool m_enforceMaxPacketSize = true;

        /// @brief Maximum number of inbound packets to process per tick.
        uint32_t m_maxInboundPacketsPerTick = 100;

        /// @brief Maximum number of pending acknowledgeable commands.
        uint32_t m_maxPendingCommands = 1000;

        /// @brief Optional callback executor for marshalling callbacks to another thread.
        CallbackExecutor m_callbackExecutor;

        /// @brief Optional custom SSL/TLS certificate verification callback.
        SslVerifyCallback m_sslVerifyCallback;
    };
} // namespace reactormq::mqtt
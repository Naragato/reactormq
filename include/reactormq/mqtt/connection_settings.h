//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "reactormq/export.h"
#include "reactormq/mqtt/connection_protocol.h"
#include "reactormq/mqtt/credentials_provider.h"

namespace reactormq::mqtt
{
    /**
     * @brief Shared connection settings used by the socket.
     */
    using ConnectionSettingsPtr = std::shared_ptr<class ConnectionSettings>;

    /**
     * @brief Executor signature for callback marshalling.
     * Accepts a callback and schedules its execution on the desired thread.
     * If nullptr (default), callbacks execute immediately on the reactor thread.
     */
    using CallbackExecutor = std::function<void(std::function<void()>)>;

    /**
     * @brief SSL/TLS certificate verification callback.
     *
     * Called during the TLS handshake to decide whether a certificate should be accepted.
     * @param preverifyOk Result of the library's built-in verification (1 = pass, 0 = fail).
     * @param ctx Opaque pointer to the verification context (X509_STORE_CTX*).
     * @return 1 to accept the certificate, 0 to reject it.
     *
     * Note: ctx is kept as void* to avoid exposing OpenSSL types in the public API.
     */
    using SslVerifyCallback = std::function<int32_t(int32_t, void*)>;

    /**
     * @brief All settings for establishing and running an MQTT connection.
     *
     * Covers addressing, authentication, timeouts/retries, and safety limits. Defaults favor
     * safe and predictable behavior. Instances are immutable after construction.
     */
    class REACTORMQ_API ConnectionSettings final
    {
    public:
        /**
         * @brief Constructor with all connection parameters.
         * @param host MQTT host name or IP address.
         * @param port Port number for the connection.
         * @param protocol Protocol to use (Mqtt, Mqtts, Ws, Wss).
         * @param credentialsProvider Provider to resolve credentials.
         * @param path URI path for the connection (for WebSocket protocols).
         * @param clientId Unique client identifier. If empty, one will be generated.
         * @param maxPacketSize Maximum size of a single MQTT packet in bytes (default: 1MB).
         * @param maxBufferSize Maximum inbound buffer size in bytes before parsing (default: 64MB).
         * @param maxOutboundQueueBytes Maximum outbound queue size in bytes (default: 10MB).
         * @param packetRetryIntervalSeconds Minimum seconds to wait before retrying a packet (default: 5).
         * @param packetRetryBackoffMultiplier Backoff multiplier for packet retry interval (default: 1.5).
         * @param maxPacketRetryIntervalSeconds Maximum seconds to wait before retrying a packet (default: 60).
         * @param socketConnectionTimeoutSeconds Timeout for socket connection attempts in seconds (default: 30).
         * @param keepAliveIntervalSeconds MQTT keep-alive interval in seconds (default: 60).
         * @param mqttConnectionTimeoutSeconds Timeout for MQTT connection handshake in seconds (default: 30).
         * @param initialRetryConnectionIntervalSeconds Initial retry interval for connection attempts in seconds (default: 5).
         * @param maxConnectionRetries Maximum number of connection retries before giving up (default: 5).
         * @param maxPacketRetries Maximum number of times to retry sending a packet (default: 3).
         * @param shouldVerifyServerCertificate Whether to verify the server certificate for secure connections (default: true).
         * @param sessionExpiryInterval Session expiry interval in seconds for MQTT 5 (default: 0 = session expires at disconnect).
         * @param autoReconnectEnabled Whether to automatically reconnect after unexpected disconnection (default: false).
         * @param autoReconnectInitialDelayMs Initial delay in milliseconds before first reconnect attempt (default: 1000ms = 1s).
         * @param autoReconnectMaxDelayMs Maximum delay in milliseconds between reconnect attempts (default: 60000ms = 60s).
         * @param autoReconnectMultiplier Multiplier for exponential backoff between reconnect attempts (default: 2.0).
         * @param strictMode Whether to use strict error handling (disconnect on protocol violations) vs lenient (warn/drop) (default:
         * false).
         * @param enforceMaxPacketSize Whether to enforce maxPacketSize limit on incoming packets (default: true).
         * @param maxInboundPacketsPerTick Maximum number of inbound packets to process per tick (default: 100).
         * @param maxPendingCommands Maximum number of pending acknowledgeable commands (QoS 1/2 publishes, subscribes, unsubscribes)
         * (default: 1000).
         * @param callbackExecutor Optional executor for marshalling callbacks to another thread (default: nullptr = immediate execution on
         * reactor thread).
         * @param sslVerifyCallback Optional custom SSL/TLS certificate verification callback (default: nullptr = use default verification).
         */
        ConnectionSettings(
            std::string host,
            const uint16_t port,
            const ConnectionProtocol protocol,
            std::shared_ptr<ICredentialsProvider> credentialsProvider,
            std::string path = std::string(),
            std::string clientId = std::string(),
            const uint32_t maxPacketSize = 1 * 1024 * 1024,
            const uint32_t maxBufferSize = 64 * 1024 * 1024,
            const uint32_t maxOutboundQueueBytes = 10 * 1024 * 1024,
            const uint16_t packetRetryIntervalSeconds = 5,
            const double packetRetryBackoffMultiplier = 1.5,
            const uint16_t maxPacketRetryIntervalSeconds = 60,
            const uint16_t socketConnectionTimeoutSeconds = 30,
            const uint16_t keepAliveIntervalSeconds = 60,
            const uint16_t mqttConnectionTimeoutSeconds = 30,
            const uint16_t initialRetryConnectionIntervalSeconds = 5,
            const uint8_t maxConnectionRetries = 5,
            const uint8_t maxPacketRetries = 3,
            const bool shouldVerifyServerCertificate = true,
            const uint32_t sessionExpiryInterval = 0,
            const bool autoReconnectEnabled = false,
            const uint32_t autoReconnectInitialDelayMs = 1000,
            const uint32_t autoReconnectMaxDelayMs = 60000,
            const double autoReconnectMultiplier = 2.0,
            const bool strictMode = false,
            const bool enforceMaxPacketSize = true,
            const uint32_t maxInboundPacketsPerTick = 100,
            const uint32_t maxPendingCommands = 1000,
            CallbackExecutor callbackExecutor = nullptr,
            SslVerifyCallback sslVerifyCallback = nullptr)
            : m_host(std::move(host))
            , m_port(port)
            , m_protocol(protocol)
            , m_credentialsProvider(std::move(credentialsProvider))
            , m_path(std::move(path))
            , m_clientId(std::move(clientId))
            , m_maxPacketSize(maxPacketSize)
            , m_maxBufferSize(maxBufferSize)
            , m_maxOutboundQueueBytes(maxOutboundQueueBytes)
            , m_packetRetryIntervalSeconds(packetRetryIntervalSeconds)
            , m_packetRetryBackoffMultiplier(packetRetryBackoffMultiplier)
            , m_maxPacketRetryIntervalSeconds(maxPacketRetryIntervalSeconds)
            , m_socketConnectionTimeoutSeconds(socketConnectionTimeoutSeconds)
            , m_keepAliveIntervalSeconds(keepAliveIntervalSeconds)
            , m_mqttConnectionTimeoutSeconds(mqttConnectionTimeoutSeconds)
            , m_initialRetryConnectionIntervalSeconds(initialRetryConnectionIntervalSeconds)
            , m_maxConnectionRetries(maxConnectionRetries)
            , m_maxPacketRetries(maxPacketRetries)
            , m_shouldVerifyServerCertificate(shouldVerifyServerCertificate)
            , m_sessionExpiryInterval(sessionExpiryInterval)
            , m_autoReconnectEnabled(autoReconnectEnabled)
            , m_autoReconnectInitialDelayMs(autoReconnectInitialDelayMs)
            , m_autoReconnectMaxDelayMs(autoReconnectMaxDelayMs)
            , m_autoReconnectMultiplier(autoReconnectMultiplier)
            , m_strictMode(strictMode)
            , m_enforceMaxPacketSize(enforceMaxPacketSize)
            , m_maxInboundPacketsPerTick(maxInboundPacketsPerTick)
            , m_maxPendingCommands(maxPendingCommands)
            , m_callbackExecutor(std::move(callbackExecutor))
            , m_sslVerifyCallback(std::move(sslVerifyCallback))
        {
        }

        /**
         * @brief Get the MQTT host name or IP address.
         * @return The host.
         */
        [[nodiscard]] const std::string& getHost() const
        {
            return m_host;
        }

        /**
         * @brief Get the port number for the connection.
         * @return The port.
         */
        [[nodiscard]] uint16_t getPort() const
        {
            return m_port;
        }

        /**
         * @brief Get the transport protocol used for the connection.
         * @return The protocol (Mqtt, Mqtts, Ws, Wss).
         */
        [[nodiscard]] ConnectionProtocol getProtocol() const
        {
            return m_protocol;
        }

        /**
         * @brief Get the credentials provider.
         * @return Shared pointer to the credentials provider.
         */
        [[nodiscard]] std::shared_ptr<ICredentialsProvider> getCredentialsProvider() const
        {
            return m_credentialsProvider;
        }

        /**
         * @brief Get the URI path for the connection (WebSocket protocols).
         * @return The path.
         */
        [[nodiscard]] const std::string& getPath() const
        {
            return m_path;
        }

        /**
         * @brief Get the unique client identifier.
         * @return The client ID.
         */
        [[nodiscard]] const std::string& getClientId() const
        {
            return m_clientId;
        }

        /**
         * @brief Get the maximum packet size in bytes.
         * @return The max packet size.
         */
        [[nodiscard]] uint32_t getMaxPacketSize() const
        {
            return m_maxPacketSize;
        }

        /**
         * @brief Get the maximum inbound buffer size in bytes.
         * This is a safety cap for accumulated inbound bytes prior to packet parsing.
         * Exceeding this cap results in a disconnect.
         * @return The max buffer size.
         */
        [[nodiscard]] uint32_t getMaxBufferSize() const
        {
            return m_maxBufferSize;
        }

        /**
         * @brief Get the maximum outbound queue size in bytes.
         * This is the limit for accumulated outbound data awaiting transmission.
         * Exceeding this limit results in backpressure (publish rejection).
         * @return The max outbound queue size.
         */
        [[nodiscard]] uint32_t getMaxOutboundQueueBytes() const
        {
            return m_maxOutboundQueueBytes;
        }

        /**
         * @brief Get the minimum packet retry interval in seconds.
         * @return The packet retry interval.
         */
        [[nodiscard]] uint16_t getPacketRetryIntervalSeconds() const
        {
            return m_packetRetryIntervalSeconds;
        }

        /**
         * @brief Get the packet retry backoff multiplier.
         * @return The backoff multiplier.
         */
        [[nodiscard]] double getPacketRetryBackoffMultiplier() const
        {
            return m_packetRetryBackoffMultiplier;
        }

        /**
         * @brief Get the maximum packet retry interval in seconds.
         * @return The max packet retry interval.
         */
        [[nodiscard]] uint16_t getMaxPacketRetryIntervalSeconds() const
        {
            return m_maxPacketRetryIntervalSeconds;
        }

        /**
         * @brief Get the socket connection timeout in seconds.
         * @return The socket connection timeout.
         */
        [[nodiscard]] uint16_t getSocketConnectionTimeoutSeconds() const
        {
            return m_socketConnectionTimeoutSeconds;
        }

        /**
         * @brief Get the MQTT keep-alive interval in seconds.
         * @return The keep-alive interval.
         */
        [[nodiscard]] uint16_t getKeepAliveIntervalSeconds() const
        {
            return m_keepAliveIntervalSeconds;
        }

        /**
         * @brief Get the MQTT connection timeout in seconds.
         * @return The MQTT connection timeout.
         */
        [[nodiscard]] uint16_t getMqttConnectionTimeoutSeconds() const
        {
            return m_mqttConnectionTimeoutSeconds;
        }

        /**
         * @brief Get the initial retry interval for connection attempts in seconds.
         * @return The initial retry interval.
         */
        [[nodiscard]] uint16_t getInitialRetryIntervalSeconds() const
        {
            return m_initialRetryConnectionIntervalSeconds;
        }

        /**
         * @brief Get the maximum number of connection retries.
         * @return The max connection retries.
         */
        [[nodiscard]] uint8_t getMaxConnectionRetries() const
        {
            return m_maxConnectionRetries;
        }

        /**
         * @brief Get the maximum number of packet retries.
         * @return The max packet retries.
         */
        [[nodiscard]] uint8_t getMaxPacketRetries() const
        {
            return m_maxPacketRetries;
        }

        /**
         * @brief Whether to verify the server certificate for secure connections.
         * @return True if certificate verification is enabled.
         */
        [[nodiscard]] bool shouldVerifyServerCertificate() const
        {
            return m_shouldVerifyServerCertificate;
        }

        /**
         * @brief Get the session expiry interval in seconds (MQTT 5).
         * @return The session expiry interval. 0 means session expires at disconnect.
         */
        [[nodiscard]] uint32_t getSessionExpiryInterval() const
        {
            return m_sessionExpiryInterval;
        }

        /**
         * @brief Check if automatic reconnection is enabled.
         * @return True if auto-reconnect is enabled after unexpected disconnection.
         */
        [[nodiscard]] bool isAutoReconnectEnabled() const
        {
            return m_autoReconnectEnabled;
        }

        /**
         * @brief Get the initial delay before first reconnect attempt in milliseconds.
         * @return The initial reconnect delay in milliseconds.
         */
        [[nodiscard]] uint32_t getAutoReconnectInitialDelayMs() const
        {
            return m_autoReconnectInitialDelayMs;
        }

        /**
         * @brief Get the maximum delay between reconnect attempts in milliseconds.
         * @return The maximum reconnect delay in milliseconds.
         */
        [[nodiscard]] uint32_t getAutoReconnectMaxDelayMs() const
        {
            return m_autoReconnectMaxDelayMs;
        }

        /**
         * @brief Get the multiplier for exponential backoff between reconnect attempts.
         * @return The reconnect backoff multiplier.
         */
        [[nodiscard]] double getAutoReconnectMultiplier() const
        {
            return m_autoReconnectMultiplier;
        }

        /**
         * @brief Check if strict error handling mode is enabled.
         * In strict mode, protocol violations cause immediate disconnect.
         * In lenient mode, violations are logged but processing continues.
         * @return True if strict mode is enabled.
         */
        [[nodiscard]] bool isStrictMode() const
        {
            return m_strictMode;
        }

        /**
         * @brief Check if maxPacketSize enforcement is enabled.
         * @return True if incoming packets exceeding maxPacketSize should be rejected.
         */
        [[nodiscard]] bool shouldEnforceMaxPacketSize() const
        {
            return m_enforceMaxPacketSize;
        }

        /**
         * @brief Get the maximum number of inbound packets to process per tick.
         * @return The max inbound packets per tick.
         */
        [[nodiscard]] uint32_t getMaxInboundPacketsPerTick() const
        {
            return m_maxInboundPacketsPerTick;
        }

        /**
         * @brief Get the maximum number of pending acknowledgeable commands.
         * @return The max pending commands limit.
         */
        [[nodiscard]] uint32_t getMaxPendingCommands() const
        {
            return m_maxPendingCommands;
        }

        /**
         * @brief Get the callback executor (if set).
         * If nullptr, callbacks execute immediately on reactor thread.
         * If set, callbacks are marshalled via the executor.
         * @return The executor function, or nullptr.
         */
        [[nodiscard]] const CallbackExecutor& getCallbackExecutor() const
        {
            return m_callbackExecutor;
        }

        /**
         * @brief Get the custom SSL/TLS certificate verification callback (if set).
         * If nullptr, the default verification callback is used.
         * @return The SSL verification callback function, or nullptr.
         */
        [[nodiscard]] const SslVerifyCallback& getSslVerifyCallback() const
        {
            return m_sslVerifyCallback;
        }

    private:
        std::string m_host;
        uint16_t m_port;
        ConnectionProtocol m_protocol;
        std::shared_ptr<ICredentialsProvider> m_credentialsProvider;
        std::string m_path;
        std::string m_clientId;
        uint32_t m_maxPacketSize;
        uint32_t m_maxBufferSize;
        uint32_t m_maxOutboundQueueBytes;
        uint16_t m_packetRetryIntervalSeconds;
        double m_packetRetryBackoffMultiplier;
        uint16_t m_maxPacketRetryIntervalSeconds;
        uint16_t m_socketConnectionTimeoutSeconds;
        uint16_t m_keepAliveIntervalSeconds;
        uint16_t m_mqttConnectionTimeoutSeconds;
        uint16_t m_initialRetryConnectionIntervalSeconds;
        uint8_t m_maxConnectionRetries;
        uint8_t m_maxPacketRetries;
        bool m_shouldVerifyServerCertificate;
        uint32_t m_sessionExpiryInterval;
        bool m_autoReconnectEnabled;
        uint32_t m_autoReconnectInitialDelayMs;
        uint32_t m_autoReconnectMaxDelayMs;
        double m_autoReconnectMultiplier;
        bool m_strictMode;
        bool m_enforceMaxPacketSize;
        uint32_t m_maxInboundPacketsPerTick;
        uint32_t m_maxPendingCommands;
        CallbackExecutor m_callbackExecutor;
        SslVerifyCallback m_sslVerifyCallback;
    };
} // namespace reactormq::mqtt
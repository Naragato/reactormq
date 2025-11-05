#pragma once

#include "socket/socket_base.h"
#include "reactormq/mqtt/connection_settings.h"

namespace reactormq::socket
{
    /**
     * @brief Wrapper class for native socket implementations.
     * 
     * Automatically selects the appropriate socket implementation (TCP or TLS)
     * based on the connection protocol specified in the settings. Delegates all
     * operations to the underlying socket instance.
     * 
     * Supports:
     * - ConnectionProtocol::Mqtt → NativeTcpSocket
     * - ConnectionProtocol::Mqtts → NativeTlsSocket (if REACTORMQ_WITH_OPENSSL is defined)
     */
    class NativeSocket final : public SocketBase
    {
    public:
        /**
         * @brief Construct a native socket with the given connection settings.
         * 
         * Automatically creates either a TCP or TLS socket based on the protocol
         * specified in the settings.
         * 
         * @param settings Shared connection settings containing host, port, protocol, etc.
         */
        explicit NativeSocket(const mqtt::ConnectionSettingsPtr& settings);

        /**
         * @brief Initiate connection to the remote host.
         */
        void connect() override;

        /**
         * @brief Gracefully disconnect from the remote host.
         */
        void disconnect() override;

        /**
         * @brief Close the socket connection.
         * @param code The close code (default: 1000 for normal closure).
         * @param reason The reason for closing the connection.
         */
        void close(int32_t code, const std::string&) override;

        /**
         * @brief Check if the socket is currently connected.
         * @return True if connected, false otherwise.
         */
        [[nodiscard]] bool isConnected() const override;

        /**
         * @brief Send raw data through the socket.
         * @param data Pointer to the data buffer to send.
         * @param size Size of the data in bytes.
         */
        void send(const uint8_t* data, uint32_t size) override;

        /**
         * @brief Process pending socket operations and receive data.
         * 
         * This method should be called regularly to drive non-blocking I/O.
         */
        void tick() override;

        [[nodiscard]] const char* getImplementationId() const override
        {
            return "NativeSocket";
        }

        /**
         * @brief Get the connection events delegate from the underlying socket.
         * @return Reference to the OnConnectCallback delegate.
         */
        OnConnectCallback& getOnConnectCallback() override
        {
            return m_socket->getOnConnectCallback();
        }

        /**
         * @brief Get the disconnection events delegate from the underlying socket.
         * @return Reference to the OnDisconnectCallback delegate.
         */
        OnDisconnectCallback& getOnDisconnectCallback() override
        {
            return m_socket->getOnDisconnectCallback();
        }

        /**
         * @brief Get the data received events delegate from the underlying socket.
         * @return Reference to the OnDataReceivedCallback delegate.
         */
        OnDataReceivedCallback& getOnDataReceivedCallback() override
        {
            return m_socket->getOnDataReceivedCallback();
        }

    private:
        /**
         * @brief The underlying socket implementation (TCP or TLS).
         */
        std::unique_ptr<SocketBase> m_socket;
    };
} // namespace reactormq::socket

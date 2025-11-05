#pragma once

#include <cstdint>

#include "reactormq/mqtt/connection_settings.h"
#include "socket/socket_delegate_mixin.h"

#ifdef REACTORMQ_PLATFORM_WINDOWS
#include <winsock2.h>
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocketHandle = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocketHandle = -1;
#endif // REACTORMQ_PLATFORM_WINDOWS

namespace reactormq::socket
{
    /**
     * @brief Native TCP socket implementation for MQTT connections.
     * 
     * Provides cross-platform TCP socket functionality using Winsock2 on Windows
     * and BSD sockets on POSIX systems. Supports non-blocking I/O and integrates
     * with the MQTT packet parser from SocketBase.
     * 
     * This class serves as the base for TLS sockets via inheritance.
     */
    class NativeTcpSocket : public SocketDelegateMixin<NativeTcpSocket>
    {
    public:
        /**
         * @brief Construct a new TCP socket with the given connection settings.
         * @param settings Shared connection settings containing host, port, and protocol.
         */
        explicit NativeTcpSocket(const mqtt::ConnectionSettingsPtr& settings);

        /**
         * @brief Virtual destructor to ensure proper cleanup in derived classes.
         */
        ~NativeTcpSocket() override;

        /**
         * @brief Check if this socket uses encryption.
         * @return false for base TCP socket, true for TLS-derived sockets.
         */
        virtual bool isEncrypted() const;

        /**
         * @brief Initiate connection to the remote host specified in settings.
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
        void close(int32_t code, const std::string& reason) override;

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
         * It receives available data, appends to the internal buffer, and
         * parses complete MQTT packets for dispatch.
         */
        void tick() override;

    protected:
        /**
         * @brief Send data through the socket (virtual for TLS override).
         * @param data Pointer to the data buffer to send.
         * @param size Size of the data in bytes.
         * @return Number of bytes sent, 0 if would block, < 0 on error.
         */
        virtual int32_t sendInternal(const uint8_t* data, uint32_t size) const;

        /**
         * @brief Receive data from the socket (virtual for TLS override).
         * @param buffer Pointer to the buffer to receive data into.
         * @param size Maximum size of data to receive in bytes.
         * @return Number of bytes received, 0 if would block, < 0 on error or disconnect.
         */
        virtual int32_t receiveInternal(uint8_t* buffer, uint32_t size) const;

        /**
         * @brief Create and configure the socket handle.
         * @return True if socket was created successfully, false otherwise.
         */
        bool createSocket();

        /**
         * @brief Close and invalidate the socket handle.
         */
        void closeSocket();

        /**
         * @brief Set the socket to non-blocking mode.
         * @return True if successful, false otherwise.
         */
        bool setNonBlocking() const;

        /**
         * @brief Disable Nagle's algorithm (set TCP_NODELAY).
         * @return True if successful, false otherwise.
         */
        bool setNoDelay() const;

        /**
         * @brief Check if connection is in progress (for non-blocking connect).
         * @return True if connection completed successfully, false if still in progress.
         */
        bool checkConnectionProgress() const;

        /**
         * @brief Platform-specific socket handle.
         */
        SocketHandle m_socketHandle;

        /**
         * @brief Flag indicating if connection is in progress.
         */
        bool m_isConnecting;
    };
} // namespace reactormq::socket

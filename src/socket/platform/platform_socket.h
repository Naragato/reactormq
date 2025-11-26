//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "socket/platform/platform.h"
#include "socket/socket_state.h"

#include <atomic>
#include <string>

namespace reactormq::socket
{
    enum class ReceiveFlags : std::uint32_t;
    enum class SocketError;

    /**
     * @brief Thin wrapper around a platform socket handle.
     * Provides a minimal API to connect, send, receive, and close a TCP connection.
     */
    class PlatformSocket
    {
    public:
        PlatformSocket() = default;

        virtual ~PlatformSocket()
        {
            PlatformSocket::close();
        }

        virtual bool createSocket();

        /**
         * @brief Initiate connection to the remote host.
         *
         * @param host The remote host address or hostname.
         * @param port The remote port number.
         * @return 0 on success, or a negative value on failure.
         */
        virtual int connect(const std::string& host, std::uint16_t port);

        /**
         * @brief Close the socket connection.
         *
         * Sends a close/shutdown sequence if appropriate for the underlying
         * protocol and then closes the underlying socket handle.
         *
         */
        virtual void close();

        [[nodiscard]] virtual bool isConnected() const;

        [[nodiscard]] virtual int getPendingData() const;

        /**
         * @brief Try to receive data without blocking indefinitely.
         *
         * @param outData Destination buffer for received bytes.
         * @param bufferSize Maximum bytes to read into outData.
         * @param bytesRead Set to the number of bytes actually read.
         * @return True if the call completed (including zero bytes read); false on error or closed connection.
         */
        virtual bool tryReceive(uint8_t* outData, int bufferSize, size_t& bytesRead) const;

        /**
         * @brief Try to receive data with receive flags.
         *
         * @param outData Destination buffer for received bytes.
         * @param bufferSize Maximum bytes to read into outData.
         * @param flags Receive flags controlling the operation.
         * @param bytesRead Set to the number of bytes actually read.
         * @return True if the call completed (including zero bytes read); false on error or closed connection.
         */
        virtual bool tryReceiveWithFlags(uint8_t* outData, int bufferSize, ReceiveFlags flags, size_t& bytesRead) const;

        /**
         * @brief Send raw data through the socket.
         *
         * @param data Pointer to the buffer to send.
         * @param size Number of bytes to attempt to send.
         * @param bytesSent Set to the number of bytes written to the socket.
         * @return True if the call completed (bytesSent may be less than size); false on error or closed connection.
         */
        virtual bool trySend(const uint8_t* data, uint32_t size, size_t& bytesSent) const;

        /**
         * @brief Get the last socket error in normalized form.
         *
         * Wraps the platform-specific error mechanism (for example,
         * @c errno / @c WSAGetLastError()) and converts it into
         * a @ref SocketError value.
         *
         * If this function returns SocketError::Unknown, the caller should consult
         * the raw platform error via @c getLastErrorCode() (or the platform-specific
         * equivalent) for more detailed information.
         *
         * @return The last socket error as a @ref SocketError value.
         */
        static SocketError getLastError();

        /**
         * @brief Get the last raw platform-specific socket error code.
         *
         * This returns the unmodified error value from the underlying platform
         * mechanism (for example, @c sce_net_errno on PS5, @c errno on POSIX
         * systems, or @c WSAGetLastError() on Windows).
         *
         * Use this when SocketError::Unknown is returned by getLastError(), or when
         * you need to log or inspect the exact platform error for debugging.
         *
         * @return The last socket error as a raw @c int32_t platform error code.
         */
        static int32_t getLastErrorCode();

        /**
         * @brief Get a human-readable description for a network error code.
         *
         * Converts a raw platform-specific error code (for example, the value
         * returned by getLastErrorCode()) into a descriptive, null-terminated
         * string suitable for logging or debugging.
         *
         * If the error code is not recognized, a generic "Unknown error" message
         * may be returned.
         *
         * @param errorCode Raw platform network error code.
         * @return A pointer to a static, null-terminated string describing the error.
         */
        static const char* getNetworkErrorDescription(int32_t errorCode);

        /**
         * @brief Initialize the platform-specific socket subsystem.
         *
         * This function must be called before creating any sockets. It performs
         * platform-specific initialization:
         * - Windows: Calls WSAStartup() to initialize Winsock
         * - POSIX/Linux/macOS: No operation (returns true)
         * - Sony PS5: No operation or minimal initialization if needed
         *
         * This function is safe to call multiple times. Subsequent calls after
         * successful initialization will return true without performing additional work.
         *
         * @return true if initialization succeeded or was already initialized, false on error.
         */
        static bool initializeNetworking();

        /**
         * @brief Clean up the platform-specific socket subsystem.
         *
         * This function should be called when socket operations are no longer needed,
         * typically at application shutdown or after all tests complete. It performs
         * platform-specific cleanup:
         * - Windows: Calls WSACleanup() to release Winsock resources
         * - POSIX/Linux/macOS: No operation
         * - Sony PS5: No operation or minimal cleanup if needed
         *
         * This function is safe to call multiple times.
         */
        static void shutdownNetworking();

    protected:
        /**
         * @brief Get the underlying socket handle.
         *
         * This method is protected to allow derived classes (e.g., PlatformSecureSocket)
         * direct access to the socket descriptor for TLS operations.
         *
         * @return The platform-specific socket handle.
         */
        [[nodiscard]] SocketHandle getSocketDescriptor() const
        {
            return m_socket;
        }

    private:
        mutable std::atomic<SocketState> m_state = SocketState::Disconnected;

        SocketHandle m_socket{ kInvalidSocketHandle };

        /**
         * @brief Convert a platform error code to a normalized SocketError.
         * @param platformCode Platform-specific error code.
         * @return Normalized socket error.
         */
        static SocketError getErrorFromPlatformCode(uint32_t platformCode);

        /**
         * @brief Check whether the underlying socket handle is valid.
         * @return True if the handle is not the platform's invalid sentinel.
         */
        [[nodiscard]] bool isHandleValid() const;
    };
} // namespace reactormq::socket
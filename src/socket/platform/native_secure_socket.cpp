//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_SECURE_SOCKET_WITH_TLS

#include "socket/platform/platform_secure_socket.h"
#include "util/logging/logging.h"

#include <openssl/ssl.h>

namespace reactormq::socket
{
    int PlatformSecureSocket::connect(const std::string& host, const std::uint16_t port)
    {
        if (m_state.load(std::memory_order_acquire) != SocketState::Disconnected)
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSecureSocket::connect() called but already connecting/connected");
            return -1;
        }

        m_state.store(SocketState::Connecting, std::memory_order_release);

        if (!createSocket())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: failed to create socket");
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return -1;
        }

        if (const int tcpResult = PlatformSocket::connect(host, port); tcpResult != 0)
        {
            const int32_t error = getLastErrorCode();
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "PlatformSecureSocket: TCP connect failed (%d: %s)",
                error,
                PlatformSocket::getNetworkErrorDescription(error));
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return tcpResult;
        }

        if (!initializeSslCommon())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: initializeSsl() failed");
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return -1;
        }

        if (const SocketHandle socketDescriptor = getSocketDescriptor(); SSL_set_fd(m_ssl, static_cast<int>(socketDescriptor)) != 1)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: SSL_set_fd() failed: %s", getLastSslErrorString().c_str());
            cleanupSsl();
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return -1;
        }

        SSL_set_connect_state(m_ssl);
        m_state.store(SocketState::SslConnecting, std::memory_order_release);

        return 0;
    }

    bool PlatformSecureSocket::hasSslHandshakeComplete() const
    {
        if (nullptr == m_ssl)
        {
            return false;
        }

        return SSL_is_init_finished(m_ssl) != 0;
    }
} // namespace reactormq::socket

#endif // REACTORMQ_SECURE_SOCKET_WITH_TLS
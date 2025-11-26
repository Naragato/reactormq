//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS

#include "socket/platform/platform_secure_socket.h"
#include "socket/platform/socket_error.h"
#include "util/logging/logging.h"

#if REACTORMQ_WITH_UE5
#if WITH_SSL
UE_PUSH_MACRO("UI")
#define UI UI_ST
#include <openssl/bio.h>
#include <openssl/ssl.h>
#undef UI
UE_POP_MACRO("UI")
#endif // WITH_SSL
#else // REACTORMQ_WITH_UE5
#include <openssl/bio.h>
#include <openssl/ssl.h>
#endif // REACTORMQ_WITH_UE5

namespace reactormq::socket
{
    static int socketBioWrite(BIO* bio, const char* buffer, int length);

    static int socketBioRead(BIO* bio, char* buffer, int length);

    static long socketBioCtrl(BIO* bio, int cmd, long num, void* ptr);

    static int socketBioCreate(BIO* bio);

    static int socketBioDestroy(BIO* bio);

    static BIO_METHOD* getSocketBioMethod()
    {
        static BIO_METHOD* method = nullptr;
        if (nullptr == method)
        {
            const int bioId = BIO_get_new_index() | BIO_TYPE_SOURCE_SINK;
            method = BIO_meth_new(bioId, "ReactorMQ PlatformSocket BIO");
            BIO_meth_set_write(method, socketBioWrite);
            BIO_meth_set_read(method, socketBioRead);
            BIO_meth_set_ctrl(method, socketBioCtrl);
            BIO_meth_set_create(method, socketBioCreate);
            BIO_meth_set_destroy(method, socketBioDestroy);
        }
        return method;
    }

    static int socketBioWrite(BIO* bio, const char* buffer, const int length)
    {
        BIO_clear_retry_flags(bio);

        const auto* self = static_cast<PlatformSecureSocket*>(BIO_get_data(bio));
        if (nullptr == self || nullptr == buffer || length <= 0)
        {
            return 0;
        }

        size_t bytesSent = 0;

        if (const bool ok
            = self->PlatformSocket::trySend(reinterpret_cast<const std::uint8_t*>(buffer), static_cast<std::uint32_t>(length), bytesSent);
            ok && bytesSent > 0)
        {
            return static_cast<int>(bytesSent);
        }

        if (const SocketError err = PlatformSocket::getLastError(); err == SocketError::WouldBlock)
        {
            BIO_set_retry_write(bio);
            return -1;
        }

        return -1;
    }

    static int socketBioRead(BIO* bio, char* buffer, const int length)
    {
        BIO_clear_retry_flags(bio);

        if (nullptr == buffer || length <= 0)
        {
            return -1;
        }

        const auto* self = static_cast<PlatformSecureSocket*>(BIO_get_data(bio));
        if (nullptr == self)
        {
            return -1;
        }

        size_t bytesRead = 0;
        const bool ok = self->PlatformSocket::tryReceive(reinterpret_cast<std::uint8_t*>(buffer), length, bytesRead);

        if (ok && bytesRead > 0)
        {
            return static_cast<int>(bytesRead);
        }

        if (const SocketError err = PlatformSocket::getLastError(); (ok && bytesRead == 0) || err == SocketError::WouldBlock)
        {
            BIO_set_retry_read(bio);
            return -1;
        }

        if (!ok)
        {
            return 0;
        }

        return -1;
    }

    static long socketBioCtrl(BIO* bio, const int cmd, const long num, void* ptr)
    {
        (void)bio;
        (void)num;
        (void)ptr;

        if (cmd == BIO_CTRL_FLUSH)
        {
            return 1;
        }

        return 0;
    }

    static int socketBioCreate(BIO* bio)
    {
        BIO_set_shutdown(bio, 0);
        BIO_set_init(bio, 1);
        BIO_set_data(bio, nullptr);
        return 1;
    }

    static int socketBioDestroy(BIO* bio)
    {
        if (nullptr == bio)
        {
            return 0;
        }

        BIO_set_init(bio, 0);
        BIO_set_data(bio, nullptr);
        return 1;
    }

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

        m_bio = BIO_new(getSocketBioMethod());
        if (nullptr == m_bio)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: BIO_new failed");
            cleanupSsl();
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            return -1;
        }

        BIO_set_data(m_bio, this);
        SSL_set_bio(m_ssl, m_bio, m_bio);
        SSL_set_connect_state(m_ssl);

        m_state.store(SocketState::SslConnecting, std::memory_order_release);

        return 0;
    }

    [[nodiscard]] bool PlatformSecureSocket::hasSslHandshakeComplete() const
    {
        if (nullptr == m_ssl)
        {
            return false;
        }

        return SSL_is_init_finished(m_ssl) != 0;
    }
} // namespace reactormq::socket

#endif // REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS
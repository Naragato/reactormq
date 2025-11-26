//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#if REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS

#include "reactormq/mqtt/connection_settings.h"
#include "socket/platform/platform_socket.h"
#include "socket/platform/trust_anchor_set.h"
#include "util/logging/logging.h"

#include <algorithm>

#if REACTORMQ_WITH_UE5
#if WITH_SSL
UE_PUSH_MACRO("UI")
#define UI UI_ST
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#undef UI
UE_POP_MACRO("UI")
#endif // WITH_SSL
#else // REACTORMQ_WITH_UE5
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#endif // REACTORMQ_WITH_UE5

#include <string>

namespace reactormq::socket
{
    /**
     * @brief TLS/SSL layer on top of PlatformSocket using OpenSSL.
     * Provides encrypted TCP using either a native SSL_set_fd setup or a BIO-based
     * wrapper for platforms without raw descriptors. Handshake and I/O are driven
     * through the same non-blocking send/receive methods.
     */
    class PlatformSecureSocket final : public PlatformSocket
    {
    public:
        /**
         * @brief Construct a secure socket with connection settings.
         *
         * @param settings Shared pointer to connection settings containing certificate verification
         *        preferences and optional custom SSL verification callback. If the settings specify
         *        certificate verification, the server's certificate chain will be validated against
         *        the system's trusted CA store.
         */
        explicit PlatformSecureSocket(mqtt::ConnectionSettingsPtr settings)
            : m_settings(std::move(settings))
        {
        }

        ~PlatformSecureSocket() override
        {
            PlatformSecureSocket::close();
        }

        PlatformSecureSocket(const PlatformSecureSocket&) = delete;

        PlatformSecureSocket& operator=(const PlatformSecureSocket&) = delete;

        PlatformSecureSocket(PlatformSecureSocket&&) = delete;

        PlatformSecureSocket& operator=(PlatformSecureSocket&&) = delete;

        /**
         * @brief Connect TCP, initialize TLS, and begin the handshake.
         *
         * Certificate verification follows the supplied ConnectionSettings.
         * @param host Remote host.
         * @param port Remote port.
         * @return 0 on success (TCP connected and TLS initialized), negative on failure.
         */
        int connect(const std::string& host, std::uint16_t port) override;

        /**
         * @brief Close the secure connection and free SSL resources.
         *
         * Performs SSL shutdown, frees SSL context and objects, then closes
         * the underlying TCP socket.
         */
        void close() override
        {
            m_state.store(SocketState::Disconnected, std::memory_order_release);
            cleanupSsl();
            PlatformSocket::close();
        }

        /**
         * @brief Check if the secure connection is fully established.
         *
         * Returns true only if both the TCP connection is active and the TLS handshake
         * has completed successfully.
         *
         * @return true if connected and handshake complete, false otherwise.
         */
        [[nodiscard]] bool isConnected() const override
        {
            const SocketState state = m_state.load(std::memory_order_acquire);

            if (state == SocketState::SslConnecting)
            {
                if (!PlatformSocket::isConnected())
                {
                    return false;
                }

                if (hasSslHandshakeComplete())
                {
                    m_state.store(SocketState::Connected, std::memory_order_release);
                    return true;
                }

                return false;
            }

            return state == SocketState::Connected && PlatformSocket::isConnected();
        }

        /**
         * @brief Try to receive decrypted data from the secure connection.
         *
         * Reads encrypted data from the underlying socket, decrypts it using SSL,
         * and writes the plaintext into the output buffer. This method handles
         * SSL-specific errors including handshake requirements.
         *
         * @param outData Pointer to the buffer to fill with decrypted data.
         * @param bufferSize Maximum number of bytes to read.
         * @param bytesRead Reference that will be set to the number of bytes actually received.
         * @return true if successful (may read 0 bytes if no data available), false on error.
         */
        bool tryReceive(std::uint8_t* outData, const int bufferSize, size_t& bytesRead) const override
        {
            if (nullptr == m_ssl || nullptr == outData || bufferSize <= 0)
            {
                bytesRead = 0;
                return false;
            }

            const int result = SSL_read(m_ssl, outData, static_cast<int>(bufferSize));
            if (result > 0)
            {
                bytesRead = result;
                return true;
            }

            const int sslError = SSL_get_error(m_ssl, result);
            if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
            {
                bytesRead = 0;
                return true;
            }

            if (sslError == SSL_ERROR_ZERO_RETURN)
            {
                bytesRead = 0;
                return false;
            }

            REACTORMQ_LOG(
                logging::LogLevel::Trace,
                "PlatformSecureSocket: SSL_read failed: %s, %d",
                getLastSslErrorString().c_str(),
                sslError);

            bytesRead = 0;
            return false;
        }

        /**
         * @brief Try to receive decrypted data with receive flags.
         *
         * For SSL/TLS connections, platform-specific receive flags (MSG_PEEK, etc.)
         * are not supported since SSL_read operates on a decrypted stream.
         * This method delegates to tryReceive() and ignores the flags parameter.
         *
         * @param outData Pointer to the buffer to fill with decrypted data.
         * @param bufferSize Maximum number of bytes to read.
         * @param flags Receive flags (ignored for SSL connections).
         * @param bytesRead Reference that will be set to the number of bytes actually received.
         * @return true if successful (may read 0 bytes if no data available), false on error.
         */
        bool tryReceiveWithFlags(std::uint8_t* outData, const int bufferSize, const ReceiveFlags flags, size_t& bytesRead) const override
        {
            (void)flags;
            return tryReceive(outData, bufferSize, bytesRead);
        }

        /**
         * @brief Send plaintext data through the secure connection.
         *
         * Encrypts the provided plaintext data using SSL and sends it through
         * the underlying socket. Handles SSL-specific errors including handshake
         * requirements and WANT_READ/WANT_WRITE conditions.
         *
         * @param data Pointer to the plaintext data to encrypt and send.
         * @param size Size of the data in bytes.
         * @param bytesSent Reference that will be set to the number of bytes actually sent.
         * @return true if successful (may send 0 bytes if you would block), false on error.
         */
        bool trySend(const std::uint8_t* data, const std::uint32_t size, size_t& bytesSent) const override
        {
            if (nullptr == m_ssl || nullptr == data || size == 0)
            {
                bytesSent = 0;
                return false;
            }

            const int result = SSL_write(m_ssl, data, static_cast<int>(size));
            if (result > 0)
            {
                bytesSent = result;
                return true;
            }

            if (const int sslError = SSL_get_error(m_ssl, result); sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
            {
                bytesSent = 0;
                return true;
            }

            REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: SSL_write failed: %s", getLastSslErrorString().c_str());
            bytesSent = 0;
            return false;
        }

        /**
         * @brief Get the number of bytes available to read without blocking.
         *
         * For secure connections, this returns the maximum of:
         * - Bytes available in the underlying TCP socket buffer
         * - Bytes already decrypted and buffered by OpenSSL (via SSL_pending)
         *
         * This ensures that already-decrypted data in SSL's internal buffers
         * is not overlooked when checking for available data.
         *
         * @return Number of bytes that can be read immediately, or 0 if none available.
         */
        [[nodiscard]] int getPendingData() const override
        {
            size_t pendingData = PlatformSocket::getPendingData();

            if (nullptr != m_ssl)
            {
                const int sslPending = SSL_pending(m_ssl);
                if (sslPending > 0)
                {
                    pendingData = std::max(pendingData, static_cast<size_t>(sslPending));
                }
            }

            return static_cast<int>(pendingData);
        }

    private:
        mutable std::atomic<SocketState> m_state = SocketState::Disconnected;

        /**
         * @brief OpenSSL certificate verification callback.
         *
         * Called during SSL/TLS handshake to validate each certificate in the chain.
         * OpenSSL performs the actual cryptographic verification; this callback allows
         * us to inspect the results and make policy decisions.
         *
         * If a custom verification callback is provided in the ConnectionSettings,
         * it will be invoked. Otherwise, the default verification logic is used.
         *
         * @param preverifyOk Result of OpenSSL's built-in verification (1 = passed, 0 = failed)
         * @param ctx X.509 store context containing the certificate being verified
         * @return 1 to accept the certificate, 0 to reject it
         */
        static int32_t verifyCertificateCallback(const int32_t preverifyOk, X509_STORE_CTX* ctx)
        {
            int32_t result = preverifyOk;

            // Basic context info
            const int32_t depth = X509_STORE_CTX_get_error_depth(ctx);
            int32_t err = X509_STORE_CTX_get_error(ctx);

            const X509* cert = X509_STORE_CTX_get_current_cert(ctx);
            std::string subjectName;
            subjectName.assign(512, '\0');
            if (cert != nullptr)
            {
                X509_NAME_oneline(X509_get_subject_name(cert), subjectName.data(), static_cast<int>(subjectName.length()));
            }

            const SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));

            if (ssl == nullptr)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "verifyCertificateCallback: no SSL* (depth %d, subject %s, err %d: %s)",
                    depth,
                    subjectName.c_str(),
                    err,
                    X509_verify_cert_error_string(err));
                return result;
            }

            const SSL_CTX* sslCtx = SSL_get_SSL_CTX(ssl);
            if (sslCtx == nullptr)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "verifyCertificateCallback: no SSL_CTX (depth %d, subject %s, err %d: %s)",
                    depth,
                    subjectName.c_str(),
                    err,
                    X509_verify_cert_error_string(err));
                return result;
            }

            const auto* optsPtr = static_cast<mqtt::ConnectionSettingsPtr*>(SSL_CTX_get_ex_data(sslCtx, s_settingsExDataIndex));

            if (optsPtr == nullptr || nullptr == *optsPtr)
            {
                if (result == kPreverifySuccess)
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Info,
                        "Certificate verification succeeded (no opts) at depth %d: %s",
                        depth,
                        subjectName.c_str());
                }
                else
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Warn,
                        "Certificate verification failed (no opts) at depth %d: %s (error %d: %s)",
                        depth,
                        subjectName.c_str(),
                        err,
                        X509_verify_cert_error_string(err));
                }
                return result;
            }

            const mqtt::ConnectionSettingsPtr& settings = *optsPtr;

            if (!settings->shouldVerifyServerCertificate())
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "Certificate verification disabled by settings; accepting cert at depth %d: %s",
                    depth,
                    subjectName.c_str());
                return kPreverifySuccess;
            }

            if (result != kPreverifySuccess)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "OpenSSL verification failed at depth %d: %s (error %d: %s)",
                    depth,
                    subjectName.c_str(),
                    err,
                    X509_verify_cert_error_string(err));
            }

            if (const mqtt::SslVerifyCallback& customCallback = settings->getSslVerifyCallback(); nullptr != customCallback)
            {
                const int32_t userResult = customCallback(preverifyOk, ctx);

                if (preverifyOk == kPreverifySuccess && userResult == kPreverifyFailed)
                {
                    X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_UNTRUSTED);
                }

                if (preverifyOk == kPreverifyFailed && userResult == kPreverifySuccess)
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Info,
                        "User verify callback overriding OpenSSL failure at depth %d: %s (err %d: %s)",
                        depth,
                        subjectName.c_str(),
                        err,
                        X509_verify_cert_error_string(err));
                }

                result = userResult;
                (void)X509_STORE_CTX_get_error(ctx);
            }

            if (result == kPreverifySuccess)
            {
                REACTORMQ_LOG(logging::LogLevel::Info, "Certificate verification succeeded at depth %d: %s", depth, subjectName.c_str());
            }
            else
            {
                const int32_t finalErr = X509_STORE_CTX_get_error(ctx);
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "PlatformSecureSocket: Certificate verification failed at depth %d: %s "
                    "(error %d: %s) for %s",
                    depth,
                    X509_verify_cert_error_string(finalErr),
                    finalErr,
                    X509_verify_cert_error_string(finalErr),
                    subjectName.c_str());
            }

            return result;
        }

        bool initializeSslCommon()
        {
            OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);

            m_sslCtx = SSL_CTX_new(TLS_client_method());
            if (nullptr == m_sslCtx)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: SSL_CTX_new failed: %s", getLastSslErrorString().c_str());
                return false;
            }

            SSL_CTX_set_mode(m_sslCtx, SSL_MODE_AUTO_RETRY);
            SSL_CTX_set_min_proto_version(m_sslCtx, TLS1_2_VERSION);

            if (m_settings && m_settings->shouldVerifyServerCertificate())
            {
                if (s_settingsExDataIndex < 0)
                {
                    s_settingsExDataIndex = SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
                }

                if (s_settingsExDataIndex >= 0)
                {
                    SSL_CTX_set_ex_data(m_sslCtx, s_settingsExDataIndex, &m_settings);
                }

                REACTORMQ_LOG(logging::LogLevel::Info, "PlatformSecureSocket: should verify certs");

                if (const TrustAnchorSet trustAnchors; !trustAnchors.empty())
                {
                    if (!trustAnchors.attachTo(m_sslCtx))
                    {
                        REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSecureSocket: failed to attach trust anchors to SSL_CTX");
                    }
                }
                else
                {
                    if (SSL_CTX_set_default_verify_paths(m_sslCtx) != 1)
                    {
                        REACTORMQ_LOG(logging::LogLevel::Warn, "PlatformSecureSocket: failed to set default verify paths");
                    }
                }
                SSL_CTX_set_verify(m_sslCtx, SSL_VERIFY_PEER, verifyCertificateCallback);
            }
            // If verification flags are not modified explicitly by SSL_CTX_set_verify()
            // or SSL_set_verify(), the default value will be SSL_VERIFY_NONE
            // else
            // {
            //     SSL_CTX_set_verify(m_sslCtx, SSL_VERIFY_NONE, nullptr);
            // }

            const auto cipherList
                = "HIGH:"
                  "!aNULL:"
                  "!eNULL:"
                  "!EXPORT:"
                  "!DES:"
                  "!RC4:"
                  "!MD5:"
                  "!PSK:"
                  "!SRP:"
                  "!CAMELLIA:"
                  "!SEED:"
                  "!IDEA:"
                  "!3DES";

            SSL_CTX_set_cipher_list(m_sslCtx, cipherList);

            const auto cipherSuites
                = "TLS_AES_256_GCM_SHA384:"
                  "TLS_CHACHA20_POLY1305_SHA256:"
                  "TLS_AES_128_GCM_SHA256";

            SSL_CTX_set_ciphersuites(m_sslCtx, cipherSuites);

            m_ssl = SSL_new(m_sslCtx);
            if (nullptr == m_ssl)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "PlatformSecureSocket: SSL_new failed: %s", getLastSslErrorString().c_str());
                cleanupSsl();
                return false;
            }

            return true;
        }

        void cleanupSsl()
        {
            if (nullptr != m_ssl)
            {
                SSL_shutdown(m_ssl);
                SSL_free(m_ssl);
                m_ssl = nullptr;
                m_bio = nullptr;
            }

            if (nullptr != m_sslCtx)
            {
                SSL_CTX_free(m_sslCtx);
                m_sslCtx = nullptr;
            }
        }

        static std::string getLastSslErrorString() noexcept
        {
            const unsigned long err = ERR_get_error();
            if (err == 0)
            {
                return "No OpenSSL error (queue empty)";
            }

            std::string buf;
            buf.assign(256, '\0');
            ERR_error_string_n(err, buf.data(), sizeof(buf.size()));
            return buf;
        }

        [[nodiscard]] bool hasSslHandshakeComplete() const;

        static constexpr int32_t kPreverifyFailed{ 0 };
        static constexpr int32_t kPreverifySuccess{ 1 };
        mqtt::ConnectionSettingsPtr m_settings;
        SSL_CTX* m_sslCtx{ nullptr };
        SSL* m_ssl{ nullptr };
#if defined(REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)
        BIO* m_bio{ nullptr };
#endif

        static inline int32_t s_settingsExDataIndex{ -1 };
    };
} // namespace reactormq::socket

#endif // REACTORMQ_WITH_TLS
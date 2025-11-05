#pragma once

#include "native_tcp_socket.h"

class NativeTlsSocket;

#ifdef REACTORMQ_WITH_OPENSSL

struct ssl_ctx_st;
struct ssl_st;
using SSL_CTX = ssl_ctx_st;
using SSL = ssl_st;

namespace reactormq::socket
{
    /**
     * @brief Native TLS socket implementation extending TCP socket with encryption.
     * 
     * Provides OpenSSL-based TLS encryption on top of the base TCP socket functionality.
     * Configures OpenSSL with secure cipher suites, curves, and minimum TLS version 1.2.
     * Supports certificate verification based on connection settings.
     * 
     * Only available when REACTORMQ_WITH_OPENSSL is defined.
     */
    class NativeTlsSocket final : public NativeTcpSocket
    {
    public:
        /**
         * @brief Construct a new TLS socket with the given connection settings.
         * @param settings Shared connection settings containing host, port, protocol, and verification options.
         */
        explicit NativeTlsSocket(const mqtt::ConnectionSettingsPtr& settings);

        /**
         * @brief Destructor that cleans up OpenSSL resources.
         */
        ~NativeTlsSocket() override;

        /**
         * @brief Check if this socket uses encryption.
         * @return true for TLS socket.
         */
        bool isEncrypted() const override;

        /**
         * @brief Initiate TLS connection to the remote host.
         * 
         * Creates the underlying TCP socket, establishes TCP connection,
         * initializes OpenSSL context and SSL handle, and sets connect state.
         * The TLS handshake occurs implicitly during the first send/receive.
         */
        void connect() override;

        /**
         * @brief Close the TLS connection and cleanup OpenSSL resources.
         * @param code The close code (default: 1000 for normal closure).
         * @param reason The reason for closing the connection.
         */
        void close(int32_t code, const std::string& reason) override;

    protected:
        /**
         * @brief Send data through the encrypted TLS connection.
         * @param data Pointer to the data buffer to send.
         * @param size Size of the data in bytes.
         * @return Number of bytes sent, 0 if would block, < 0 on error.
         */
        int32_t sendInternal(const uint8_t* data, uint32_t size) const override;

        /**
         * @brief Receive data from the encrypted TLS connection.
         * @param buffer Pointer to the buffer to receive data into.
         * @param size Maximum size of data to receive in bytes.
         * @return Number of bytes received, 0 if would block, < 0 on error or disconnect.
         */
        int32_t receiveInternal(uint8_t* buffer, uint32_t size) const override;

    private:
        /**
         * @brief Initialize the OpenSSL context and SSL handle.
         * 
         * Configures:
         * - Minimum TLS version 1.2
         * - TLS 1.2 ciphers: HIGH:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!SRP:!CAMELLIA:!SEED:!IDEA:!3DES
         * - TLS 1.3 suites: TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256
         * - Curves: P-521:P-384:P-256
         * - Disables: SSLv2, SSLv3, TLSv1.0, TLSv1.1
         * 
         * @return True if initialization succeeded, false otherwise.
         */
        bool initializeTls();

        /**
         * @brief Clean up OpenSSL resources (SSL and SSL_CTX).
         */
        void cleanupTls();

        /**
         * @brief OpenSSL context handle.
         */
        SSL_CTX* m_sslContext;

        /**
         * @brief OpenSSL SSL handle.
         */
        SSL* m_sslSocket;
    };
} // namespace reactormq::socket

#endif // REACTORMQ_WITH_OPENSSL

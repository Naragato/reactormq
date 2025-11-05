#include "native_tls_socket.h"

#ifdef REACTORMQ_WITH_OPENSSL

#include <cstring>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#ifdef REACTORMQ_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#endif // REACTORMQ_PLATFORM_WINDOWS

namespace reactormq::socket
{
    namespace
    {
        int32_t verifyCertificateCallback(const int32_t preverifyOk, X509_STORE_CTX* ctx)
        {
            if (preverifyOk == 0)
            {
                const int32_t err = X509_STORE_CTX_get_error(ctx);
                const int32_t depth = X509_STORE_CTX_get_error_depth(ctx);

                if (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT ||
                    err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
                {
                    return 0;
                }

                if (depth > 0)
                {
                    return 1;
                }
            }

            return preverifyOk;
        }

        bool isSslWouldBlock(const int32_t sslError)
        {
            return sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE;
        }

#ifdef REACTORMQ_PLATFORM_WINDOWS
        int32_t getLastError()
        {
            return WSAGetLastError();
        }

        bool isInProgress(const int32_t error)
        {
            return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
        }
#else
        int32_t getLastError()
        {
            return errno;
        }

        bool isInProgress(int error)
        {
            return error == EINPROGRESS;
        }
#endif // REACTORMQ_PLATFORM_WINDOWS
    }

    NativeTlsSocket::NativeTlsSocket(const mqtt::ConnectionSettingsPtr& settings)
        : NativeTcpSocket(settings), m_sslContext(nullptr), m_sslSocket(nullptr)
    {
    }

    NativeTlsSocket::~NativeTlsSocket()
    {
        cleanupTls();
    }

    bool NativeTlsSocket::isEncrypted() const
    {
        return true;
    }

    void NativeTlsSocket::connect()
    {
        std::scoped_lock lock(m_socketMutex);

        cleanupTls();

        if (m_socketHandle != kInvalidSocketHandle)
        {
            closeSocket();
        }

        if (!createSocket())
        {
            invokeOnConnect(false);
            return;
        }

        if (!setNonBlocking() || !setNoDelay())
        {
            closeSocket();
            invokeOnConnect(false);
            return;
        }

        const std::string& host = m_settings->getHost();
        const uint16_t port = m_settings->getPort();

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        const int32_t getaddrinfoResult = getaddrinfo(host.c_str(), nullptr, &hints, &result);

        if (getaddrinfoResult != 0 || result == nullptr)
        {
            closeSocket();
            invokeOnConnect(false);
            return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        std::memcpy(&addr.sin_addr, &reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr, sizeof(addr.sin_addr));

        freeaddrinfo(result);

        const int32_t connectResult = ::connect(m_socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        if (connectResult == 0)
        {
            m_isConnecting = false;
        }
        else
        {
            const int32_t error = getLastError();
            if (isInProgress(error))
            {
                m_isConnecting = true;
            }
            else
            {
                closeSocket();
                invokeOnConnect(false);
                return;
            }
        }

        if (!initializeTls())
        {
            closeSocket();
            invokeOnConnect(false);
            return;
        }

        if (!m_isConnecting)
        {
            invokeOnConnect(true);
        }
    }

    void NativeTlsSocket::close(int32_t code, const std::string& reason)
    {
        std::scoped_lock lock(m_socketMutex);
        cleanupTls();
        closeSocket();
        m_isConnecting = false;
        invokeOnDisconnect();
    }

    int32_t NativeTlsSocket::sendInternal(const uint8_t* data, const uint32_t size) const
    {
        if (m_sslSocket == nullptr)
        {
            return -1;
        }

        const int32_t sent = SSL_write(m_sslSocket, data, static_cast<int>(size));

        if (sent < 0)
        {
            const int32_t sslError = SSL_get_error(m_sslSocket, sent);
            if (isSslWouldBlock(sslError))
            {
                return 0;
            }
            return -1;
        }

        return sent;
    }

    int32_t NativeTlsSocket::receiveInternal(uint8_t* buffer, const uint32_t size) const
    {
        if (m_sslSocket == nullptr)
        {
            return -1;
        }

        const int32_t received = SSL_read(m_sslSocket, buffer, static_cast<int>(size));

        if (received < 0)
        {
            const int32_t sslError = SSL_get_error(m_sslSocket, received);
            if (isSslWouldBlock(sslError))
            {
                return 0;
            }
            return -1;
        }

        if (received == 0)
        {
            return -1;
        }

        return received;
    }

    bool NativeTlsSocket::initializeTls()
    {
        m_sslContext = SSL_CTX_new(TLS_client_method());

        if (m_sslContext == nullptr)
        {
            return false;
        }

        SSL_CTX_set_min_proto_version(m_sslContext, TLS1_2_VERSION);

        constexpr unsigned long kSslOptions = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 |
            SSL_OP_NO_TLSv1_1;
        SSL_CTX_set_options(m_sslContext, kSslOptions);

        const char* cipherList = "HIGH:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!SRP:!CAMELLIA:!SEED:!IDEA:!3DES";
        if (SSL_CTX_set_cipher_list(m_sslContext, cipherList) != 1)
        {
            cleanupTls();
            return false;
        }

        const char* cipherSuites = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256";
        if (SSL_CTX_set_ciphersuites(m_sslContext, cipherSuites) != 1)
        {
            cleanupTls();
            return false;
        }

        const char* curves = "P-521:P-384:P-256";
        if (SSL_CTX_set1_curves_list(m_sslContext, curves) != 1)
        {
            cleanupTls();
            return false;
        }

        if (m_settings->shouldVerifyServerCertificate())
        {
            SSL_CTX_set_verify(m_sslContext, SSL_VERIFY_PEER, verifyCertificateCallback);

            if (SSL_CTX_set_default_verify_paths(m_sslContext) != 1)
            {
                cleanupTls();
                return false;
            }
        }
        else
        {
            SSL_CTX_set_verify(m_sslContext, SSL_VERIFY_NONE, nullptr);
        }

        m_sslSocket = SSL_new(m_sslContext);

        if (m_sslSocket == nullptr)
        {
            cleanupTls();
            return false;
        }

        if (SSL_set_fd(m_sslSocket, static_cast<int>(m_socketHandle)) != 1)
        {
            cleanupTls();
            return false;
        }

        SSL_set_connect_state(m_sslSocket);

        const std::string& host = m_settings->getHost();
        SSL_set_tlsext_host_name(m_sslSocket, host.c_str());

        return true;
    }

    void NativeTlsSocket::cleanupTls()
    {
        if (m_sslSocket != nullptr)
        {
            SSL_free(m_sslSocket);
            m_sslSocket = nullptr;
        }

        if (m_sslContext != nullptr)
        {
            SSL_CTX_free(m_sslContext);
            m_sslContext = nullptr;
        }
    }
} // namespace reactormq::socket

#endif // REACTORMQ_WITH_OPENSSL
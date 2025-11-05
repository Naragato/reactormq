#ifdef REACTORMQ_WITH_FSOCKET_UE5

#include "ue5_socket.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "SslModule.h"
#include "Interfaces/ISslCertificateManager.h"

#if WITH_SSL
#define UI UI_ST
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#undef UI
#endif // WITH_SSL

namespace reactormq::socket
{
    Ue5Socket::Ue5Socket(const mqtt::ConnectionSettingsPtr& settings)
        : SocketDelegateMixin<Ue5Socket>(settings)
        , m_socket(nullptr, [](FSocket* s)
        {
            if (s)
            {
                s->Close();
                ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(s);
            }
        })
        , m_currentState(Ue5SocketState::Disconnected)
        , m_useSsl(settings->getProtocol() == mqtt::ConnectionProtocol::Mqtts)
#if WITH_SSL
        , m_sslCtx(nullptr)
        , m_ssl(nullptr)
        , m_bio(nullptr)
#endif // WITH_SSL
    {
    }

    Ue5Socket::~Ue5Socket()
    {
        disconnectInternal();
    }

    void Ue5Socket::connect()
    {
        Ue5SocketState expected = Ue5SocketState::Disconnected;
        if (!m_currentState.compare_exchange_strong(
            expected,
            Ue5SocketState::Connecting,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
        {
            return;
        }

        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);
            initializeSocket();
            if (m_socket)
            {
                ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
                FAddressInfoResult addressInfoResult = socketSubsystem->GetAddressInfo(
                    *FString(m_settings->getHost().c_str()),
                    nullptr,
                    EAddressInfoFlags::Default,
                    NAME_None,
                    SOCKTYPE_Streaming);

                if (addressInfoResult.ReturnCode == SE_NO_ERROR && addressInfoResult.Results.Num() > 0)
                {
                    const TSharedRef<FInternetAddr> addr = addressInfoResult.Results[0].Address;
                    if (addr->IsValid())
                    {
                        addr->SetPort(m_settings->getPort());
                        if (m_socket->Connect(*addr))
                        {
#if WITH_SSL
                            if (m_useSsl)
                            {
                                if (initializeSsl())
                                {
                                    m_currentState.store(Ue5SocketState::SslConnecting, std::memory_order_release);
                                }
                            }
                            else
#endif // WITH_SSL
                            {
                                m_currentState.store(Ue5SocketState::Connected, std::memory_order_release);
                            }
                        }
                    }
                }
            }
        }

        if (m_currentState.load(std::memory_order_acquire) == Ue5SocketState::Connected)
        {
            invokeOnConnect(true);
        }
        else if (m_currentState.load(std::memory_order_acquire) != Ue5SocketState::SslConnecting)
        {
            invokeOnConnect(false);
            expected = Ue5SocketState::Connecting;
            if (m_currentState.compare_exchange_strong(expected, Ue5SocketState::Disconnected))
            {
                disconnectInternal();
            }
        }
    }

    void Ue5Socket::disconnect()
    {
        while (m_currentState.load(std::memory_order_acquire) == Ue5SocketState::Connecting)
        {
            FPlatformProcess::YieldThread();
        }

        Ue5SocketState expected = m_currentState.load(std::memory_order_acquire);
        while (expected == Ue5SocketState::Connected || expected == Ue5SocketState::SslConnecting)
        {
            if (m_currentState.compare_exchange_weak(expected, Ue5SocketState::Disconnected))
            {
                disconnectInternal();
                invokeOnDisconnect();
                break;
            }
        }
    }

    void Ue5Socket::close(int32_t code, const std::string& reason)
    {
        disconnect();
    }

    void Ue5Socket::send(const uint8_t* data, uint32_t size)
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
        if (!isConnected())
        {
            return;
        }

#if WITH_SSL
        if (m_useSsl)
        {
            const int32_t ret = SSL_write(m_ssl, data, size);
            if (ret < 0)
            {
                const int32_t errCode = SSL_get_error(m_ssl, ret);
                if (errCode != SSL_ERROR_WANT_READ && errCode != SSL_ERROR_WANT_WRITE)
                {
                    disconnect();
                }
            }
        }
        else
#endif // WITH_SSL
        {
            size_t totalBytesSent = 0;
            while (totalBytesSent < size)
            {
                int32_t bytesSent = 0;
                if (!m_socket->Send(data + totalBytesSent, size - totalBytesSent, bytesSent) || bytesSent == 0)
                {
                    disconnect();
                    return;
                }
                totalBytesSent += bytesSent;
            }
        }
    }

    void Ue5Socket::tick()
    {
        bool shouldDisconnect = false;

        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);

            const Ue5SocketState state = m_currentState.load(std::memory_order_acquire);
            if (state == Ue5SocketState::Disconnected)
            {
                return;
            }

#if WITH_SSL
            if (m_useSsl)
            {
                if (state == Ue5SocketState::SslConnecting)
                {
                    shouldDisconnect = !performSslHandshake();
                }
                else if (isConnected())
                {
                    shouldDisconnect = !readAvailableData(
                        [this](int32_t want, std::vector<uint8_t>& temp, size_t& bytesRead)
                        {
                            return receiveFromSsl(want, temp, bytesRead);
                        });
                }
            }
            else
#endif // WITH_SSL
            {
                if (isConnected())
                {
                    shouldDisconnect = !readAvailableData(
                        [this](int32_t want, std::vector<uint8_t>& temp, size_t& bytesRead)
                        {
                            return receiveFromSocket(want, temp, bytesRead);
                        });
                }
            }
        }

        if (shouldDisconnect)
        {
            disconnect();
        }
    }

    bool Ue5Socket::isConnected() const
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
        return (m_currentState.load(std::memory_order_acquire) == Ue5SocketState::Connected)
            && m_socket
            && m_socket->GetConnectionState() == ESocketConnectionState::SCS_Connected;
    }

    void Ue5Socket::disconnectInternal()
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
#if WITH_SSL
        if (m_useSsl)
        {
            cleanupSsl();
        }
#endif // WITH_SSL
        if (m_socket)
        {
            m_socket->Close();
            m_socket.reset();
        }
    }

    void Ue5Socket::initializeSocket()
    {
        ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        FSocket* rawSocket = socketSubsystem->CreateSocket(NAME_Stream, TEXT("ReactorMQ Connection"), false);

        if (!rawSocket)
        {
            return;
        }

        const bool success = rawSocket->SetReuseAddr(true)
            && rawSocket->SetNonBlocking(true)
            && rawSocket->SetNoDelay(true)
            && rawSocket->SetLinger(false, 0)
            && rawSocket->SetRecvErr();

        if (!success)
        {
            rawSocket->Close();
            socketSubsystem->DestroySocket(rawSocket);
            return;
        }

        int32_t bufferSize = 0;
        rawSocket->SetReceiveBufferSize(kBufferSize, bufferSize);
        rawSocket->SetSendBufferSize(kBufferSize, bufferSize);

        m_socket.reset(rawSocket);
    }

    bool Ue5Socket::isSocketReadyForWrite() const
    {
        if (m_socket->GetConnectionState() == ESocketConnectionState::SCS_ConnectionError)
        {
            return false;
        }

        int32_t bytesRead = 0;
        uint8_t dummy = 0;
        if (!m_socket->Recv(&dummy, 1, bytesRead, ESocketReceiveFlags::Peek))
        {
            return false;
        }
        return true;
    }

    bool Ue5Socket::receiveFromSocket(int32_t want, std::vector<uint8_t>& temp, size_t& outBytesRead) const
    {
        outBytesRead = 0;
        int32_t bytesRead = 0;
        if (!m_socket->Recv(temp.data(), want, bytesRead))
        {
            return false;
        }

        outBytesRead = bytesRead;
        return true;
    }

    bool Ue5Socket::readAvailableData(
        const std::function<bool(int32_t want, std::vector<uint8_t>& temp, size_t& outBytesRead)>& reader)
    {
        uint32_t pendingData = 0;
        m_socket->HasPendingData(pendingData);

#if WITH_SSL
        if (m_useSsl && m_ssl)
        {
            pendingData = FMath::Max<int32_t>(pendingData, SSL_pending(m_ssl));
        }
#endif // WITH_SSL

        if (pendingData == 0)
        {
            return true;
        }

        const int32_t want = FMath::Min<int32_t>(kMaxChunkSize, static_cast<int32_t>(pendingData));

        std::vector<uint8_t> temp(want);

        size_t bytesRead = 0;
        if (!reader(want, temp, bytesRead))
        {
            return false;
        }

        if (bytesRead > 0)
        {
#if WITH_SSL
            if (m_useSsl)
            {
                appendAndProcess(temp.data(), bytesRead);
            }
            else
#endif // WITH_SSL
            {
                std::scoped_lock<std::mutex> lock(m_socketMutex);
                m_dataBuffer.insert(m_dataBuffer.end(), temp.data(), temp.data() + bytesRead);
            }
        }

        return true;
    }

#if WITH_SSL
    bool Ue5Socket::initializeSsl()
    {
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
        m_sslCtx = SSL_CTX_new(TLS_client_method());
        if (!m_sslCtx)
        {
            return false;
        }

        SSL_CTX_set_min_proto_version(m_sslCtx, TLS1_2_VERSION);

        if (m_settings->shouldVerifyServerCertificate())
        {
            SSL_CTX_set_verify(m_sslCtx, SSL_VERIFY_PEER, sslCertVerify);
        }
        else
        {
            SSL_CTX_set_verify(m_sslCtx, SSL_VERIFY_NONE, nullptr);
        }

        FSslModule::Get().GetCertificateManager().AddCertificatesToSslContext(m_sslCtx);

        m_ssl = SSL_new(m_sslCtx);
        if (!m_ssl)
        {
            SSL_CTX_free(m_sslCtx);
            m_sslCtx = nullptr;
            return false;
        }

        SSL_set_min_proto_version(m_ssl, TLS1_2_VERSION);
        SSL_set_hostflags(m_ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
        SSL_set1_host(m_ssl, m_settings->getHost().c_str());

        const char* cipherList =
            "HIGH:"
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
        SSL_set_cipher_list(m_ssl, cipherList);

        const char* cipherSuites =
            "TLS_AES_256_GCM_SHA384:"
            "TLS_CHACHA20_POLY1305_SHA256:"
            "TLS_AES_128_GCM_SHA256";

        SSL_CTX_set_ciphersuites(m_sslCtx, cipherSuites);
        SSL_set_ciphersuites(m_ssl, cipherSuites);

        SSL_CTX_set1_curves_list(m_sslCtx, "P-521:P-384:P-256");
        SSL_set1_curves_list(m_ssl, "P-521:P-384:P-256");
        SSL_CTX_set_options(m_sslCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

        m_bio = BIO_new(getSocketBioMethod());
        if (!m_bio)
        {
            cleanupSsl();
            return false;
        }

        BIO_set_data(m_bio, this);
        SSL_set_bio(m_ssl, m_bio, m_bio);
        SSL_set_connect_state(m_ssl);
        SSL_set_app_data(m_ssl, this);
        return true;
    }

    void Ue5Socket::cleanupSsl()
    {
        if (m_ssl)
        {
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
            m_ssl = nullptr;
        }
        if (m_bio)
        {
            m_bio = nullptr;
        }
        if (m_sslCtx)
        {
            SSL_CTX_free(m_sslCtx);
            m_sslCtx = nullptr;
        }
    }

    bool Ue5Socket::performSslHandshake()
    {
        const int ret = SSL_connect(m_ssl);
        switch (const int sslError = SSL_get_error(m_ssl, ret))
        {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return true;
        case SSL_ERROR_NONE:
            if (m_settings->shouldVerifyServerCertificate())
            {
                if (const long verifyResult = SSL_get_verify_result(m_ssl); verifyResult != X509_V_OK)
                {
                    return false;
                }
            }
            m_currentState.store(Ue5SocketState::Connected, std::memory_order_release);
            invokeOnConnect(true);
            return true;
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SYSCALL:
        default:
            return false;
        }
    }

    bool Ue5Socket::receiveFromSsl(int32_t want, std::vector<uint8_t>& temp, size_t& bytesRead) const
    {
        const int32_t ret = SSL_read_ex(m_ssl, temp.data(), want, &bytesRead);

        if (bytesRead > 0 && ret == 1)
        {
            return true;
        }

        switch (const int err = SSL_get_error(m_ssl, ret))
        {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return true;
        default:
            return false;
        }
    }

    void Ue5Socket::appendAndProcess(const uint8_t* data, size_t length)
    {
        bool shouldDisconnect = false;
        size_t currentSize;
        uint32_t capBytes;
        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);
            m_dataBuffer.insert(m_dataBuffer.end(), data, data + length);
            currentSize = m_dataBuffer.size();
            capBytes = m_settings->getMaxBufferSize();
            if (currentSize > capBytes)
            {
                shouldDisconnect = true;
            }
        }
        if (shouldDisconnect)
        {
            disconnect();
            return;
        }
        readPacketsFromBuffer();
    }

    std::string Ue5Socket::getLastSslErrorString(bool consume) noexcept
    {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        const unsigned long err = consume ? ERR_get_error() : ERR_peek_last_error();
#else
        const unsigned long err = ERR_get_error();
#endif // OPENSSL_VERSION_NUMBER >= 0x10100000L

        if (err == 0)
        {
            return "No OpenSSL error (queue empty)";
        }

        char buf[256] = {};
        ERR_error_string_n(err, buf, sizeof(buf));

        const char* libStr = ERR_lib_error_string(err);
        const char* reasonStr = ERR_reason_error_string(err);

        if (libStr || reasonStr)
        {
            return std::string(buf) + " | lib=" + (libStr ? libStr : "?") + " | reason=" + (reasonStr ? reasonStr : "?");
        }

        return std::string(buf);
    }

    int32_t Ue5Socket::sslCertVerify(int32_t preverifyOk, X509_STORE_CTX* context)
    {
        const SSL* handle = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(
            context,
            SSL_get_ex_data_X509_STORE_CTX_idx()));
        if (!handle)
        {
            return 0;
        }
        const Ue5Socket* socket = static_cast<Ue5Socket*>(SSL_get_app_data(handle));
        if (!socket)
        {
            return 0;
        }

        if (!socket->m_settings->shouldVerifyServerCertificate())
        {
            return 1;
        }

        if (preverifyOk == 1)
        {
            if (!FSslModule::Get().GetCertificateManager().VerifySslCertificates(
                context,
                socket->m_settings->getHost().c_str()))
            {
                preverifyOk = 0;
            }
        }
        return preverifyOk;
    }

    BIO_METHOD* Ue5Socket::getSocketBioMethod()
    {
        static BIO_METHOD* method = nullptr;
        if (!method)
        {
            const int32_t bioId = BIO_get_new_index() | BIO_TYPE_SOURCE_SINK;
            method = BIO_meth_new(bioId, "ReactorMQ Socket BIO");
            BIO_meth_set_write(method, socketBioWrite);
            BIO_meth_set_read(method, socketBioRead);
            BIO_meth_set_ctrl(method, socketBioCtrl);
            BIO_meth_set_create(method, socketBioCreate);
            BIO_meth_set_destroy(method, socketBioDestroy);
        }
        return method;
    }

    int Ue5Socket::socketBioWrite(BIO* bio, const char* buffer, const int32_t bufferSize)
    {
        BIO_clear_retry_flags(bio);

        const Ue5Socket* self = static_cast<const Ue5Socket*>(BIO_get_data(bio));
        if (!self || !self->m_socket || bufferSize <= 0)
        {
            return 0;
        }

        int32_t bytesSent = 0;
        const bool ok = self->m_socket->Send(reinterpret_cast<const uint8_t*>(buffer), bufferSize, bytesSent);

        if (ok && bytesSent > 0)
        {
            return bytesSent;
        }

        const ESocketErrors err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
        if (err == SE_EWOULDBLOCK)
        {
            BIO_set_retry_write(bio);
            return -1;
        }

        return -1;
    }

    int Ue5Socket::socketBioRead(BIO* bio, char* outBuffer, const int32_t bufferSize)
    {
        BIO_clear_retry_flags(bio);

        if (!outBuffer)
        {
            return -1;
        }

        const Ue5Socket* self = static_cast<const Ue5Socket*>(BIO_get_data(bio));
        if (!self || !self->m_socket)
        {
            return -1;
        }

        uint32_t size = bufferSize;
        if (!self->m_socket->HasPendingData(size))
        {
            BIO_set_retry_read(bio);
            return 0;
        }

        int32_t bytesRead = 0;
        const bool ok = self->m_socket->Recv(reinterpret_cast<uint8_t*>(outBuffer), bufferSize, bytesRead);
        if (ok && bytesRead > 0)
        {
            return bytesRead;
        }

        const ESocketErrors err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();

        if ((ok && bytesRead == 0) || err == SE_EWOULDBLOCK)
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

    long Ue5Socket::socketBioCtrl(BIO* bio, int cmd, long num, void* ptr)
    {
        switch (cmd)
        {
        case BIO_CTRL_FLUSH:
            return 1;
        default:
            return 0;
        }
    }

    int Ue5Socket::socketBioCreate(BIO* bio)
    {
        BIO_set_shutdown(bio, 0);
        BIO_set_init(bio, 1);
        BIO_set_data(bio, nullptr);
        return 1;
    }

    int Ue5Socket::socketBioDestroy(BIO* bio)
    {
        if (!bio)
        {
            return 0;
        }

        BIO_set_init(bio, 0);
        BIO_set_data(bio, nullptr);
        return 1;
    }
#endif // WITH_SSL

} // namespace reactormq::socket

#endif // REACTORMQ_WITH_FSOCKET_UE5

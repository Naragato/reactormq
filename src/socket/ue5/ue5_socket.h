#pragma once

#ifdef REACTORMQ_WITH_FSOCKET_UE5

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "socket/socket_delegate_mixin.h"
#include "reactormq/mqtt/connection_settings.h"

class FSocket;
class ISocketSubsystem;

#if WITH_SSL
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct bio_st BIO;
typedef struct bio_method_st BIO_METHOD;
typedef struct x509_store_ctx_st X509_STORE_CTX;
#endif // WITH_SSL

namespace reactormq::socket
{
    /**
     * @brief Socket state for the UE5 FSocket adapter.
     */
    enum class Ue5SocketState : uint8_t
    {
        Disconnected,
        Connecting,
        SslConnecting,
        Connected
    };

    /**
     * @brief UE5 FSocket adapter for ReactorMQ.
     * 
     * Supports both plain TCP (mqtt://) and TLS (mqtts://) connections using UE5's FSocket API.
     * Uses non-blocking I/O driven by tick().
     * Feeds received data to the base class shared buffer for MQTT framing via readPacketsFromBuffer().
     */
    class Ue5Socket final : public SocketDelegateMixin<Ue5Socket>
    {
    public:
        /**
         * @brief Construct a UE5 socket with the given connection settings.
         * @param settings Connection settings containing host, port, protocol, and TLS options.
         */
        explicit Ue5Socket(const mqtt::ConnectionSettingsPtr& settings);

        /**
         * @brief Destructor.
         */
        ~Ue5Socket() override;

        void connect() override;
        void disconnect() override;
        void close(int32_t code, const std::string& reason) override;
        [[nodiscard]] bool isConnected() const override;
        void send(const uint8_t* data, uint32_t size) override;
        void tick() override;

        [[nodiscard]] const char* getImplementationId() const override
        {
            return "Ue5Socket";
        }

    private:
        static constexpr uint32_t kBufferSize = 2 * 1024 * 1024;
        static constexpr uint32_t kMaxChunkSize = 16 * 1024;

        std::unique_ptr<FSocket, std::function<void(FSocket*)>> m_socket;
        std::atomic<Ue5SocketState> m_currentState;
        bool m_useSsl;

#if WITH_SSL
        SSL_CTX* m_sslCtx;
        SSL* m_ssl;
        BIO* m_bio;
#endif // WITH_SSL

        void disconnectInternal();
        void initializeSocket();
        bool isSocketReadyForWrite() const;
        
        bool receiveFromSocket(int32_t want, std::vector<uint8_t>& temp, size_t& outBytesRead) const;
        
        bool readAvailableData(
            const std::function<bool(int32_t want, std::vector<uint8_t>& temp, size_t& outBytesRead)>& reader);

#if WITH_SSL
        bool initializeSsl();
        void cleanupSsl();
        bool performSslHandshake();
        bool receiveFromSsl(int32_t want, std::vector<uint8_t>& temp, size_t& bytesRead) const;
        void appendAndProcess(const uint8_t* data, size_t length);
        
        static std::string getLastSslErrorString(bool consume = false) noexcept;
        static int32_t sslCertVerify(int32_t preverifyOk, X509_STORE_CTX* context);
        static BIO_METHOD* getSocketBioMethod();
        static int socketBioWrite(BIO* bio, const char* buffer, int32_t bufferSize);
        static int socketBioRead(BIO* bio, char* outBuffer, int32_t bufferSize);
        static long socketBioCtrl(BIO* bio, int cmd, long num, void* ptr);
        static int socketBioCreate(BIO* bio);
        static int socketBioDestroy(BIO* bio);
#endif // WITH_SSL
    };

} // namespace reactormq::socket

#endif // REACTORMQ_WITH_FSOCKET_UE5

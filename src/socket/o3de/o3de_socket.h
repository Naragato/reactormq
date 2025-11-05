#pragma once

#ifdef REACTORMQ_WITH_O3DE_SOCKET

#include <atomic>
#include <memory>

#include "reactormq/mqtt/connection_settings.h"
#include "socket/socket_delegate_mixin.h"

namespace AzNetworking
{
    class TcpSocket;
    class IpAddress;
    enum class TrustZone : uint8_t;
}

namespace reactormq::socket
{
    /// @brief Enum representing the state of an O3DE socket connection.
    enum class O3deSocketState : uint8_t
    {
        Disconnected,
        Connecting,
        Connected
    };

    /// @brief O3DE adapter that implements the Socket interface.
    class O3deSocket final : public SocketDelegateMixin<O3deSocket>
    {
    public:
        /**
         * @brief Construct an O3DE socket with the given connection settings.
         * @param settings Connection settings containing host, port, protocol, and TLS options.
         */
        explicit O3deSocket(const mqtt::ConnectionSettingsPtr& settings);

        ~O3deSocket() override;

        void connect() override;
        void disconnect() override;
        void close(int32_t code, const std::string& reason) override;
        [[nodiscard]] bool isConnected() const override;
        void send(const uint8_t* data, uint32_t size) override;
        void tick() override;

        [[nodiscard]] const char* getImplementationId() const override
        {
            return "O3deSocket";
        }

    private:
        static constexpr uint32_t kMaxChunkSize = 16 * 1024;

        std::unique_ptr<AzNetworking::TcpSocket> m_socket;
        std::atomic<O3deSocketState> m_currentState;
        bool m_useTls;

        void disconnectInternal();
        bool receiveData();
        std::unique_ptr<AzNetworking::IpAddress> createIpAddress(const std::string& host, uint16_t port);
    };

} // namespace reactormq::socket

#endif // REACTORMQ_WITH_O3DE_SOCKET

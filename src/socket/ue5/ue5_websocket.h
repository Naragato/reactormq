#pragma once

#ifdef REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "socket/socket_delegate_mixin.h"
#include "reactormq/mqtt/connection_settings.h"

class IWebSocket;

namespace reactormq::socket
{
    /// @brief WebSocket state for the UE5 WebSocket adapter.
    enum class Ue5WebSocketState : uint8_t
    {
        Disconnected,
        Connecting,
        Connected
    };

    /**
     * @brief UE5 adapter that implements the Socket interface on top of IWebSocket.
     *
     * Supports ws and wss; MQTT bytes are sent as binary frames.
     */
    class Ue5WebSocket final : public SocketDelegateMixin<Ue5WebSocket>
    {
    public:
        /**
         * @brief Construct a UE5 WebSocket with the given connection settings.
         * @param settings Connection settings containing host, port, protocol, path, and TLS options.
         */
        explicit Ue5WebSocket(const mqtt::ConnectionSettingsPtr& settings);

        ~Ue5WebSocket() override;

        void connect() override;
        void disconnect() override;
        void close(int32_t code, const std::string& reason) override;
        [[nodiscard]] bool isConnected() const override;
        void send(const uint8_t* data, uint32_t size) override;
        void tick() override;

        [[nodiscard]] const char* getImplementationId() const override
        {
            return "Ue5WebSocket";
        }

    private:
        std::shared_ptr<IWebSocket> m_webSocket;
        std::atomic<Ue5WebSocketState> m_currentState;
        bool m_useTls;

        void onConnected();
        void onConnectionError(const std::string& error);
        void onClosed(int32_t statusCode, const std::string& reason, bool wasClean);
        void onMessage(const void* data, size_t size, size_t bytesRemaining);
        void onBinaryMessage(const void* data, size_t size, bool isLastFragment);

        std::string buildWebSocketUrl() const;
    };

} // namespace reactormq::socket

#endif // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5

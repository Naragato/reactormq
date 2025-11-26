//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "socket/platform/platform_socket.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

namespace reactormq::socket
{
    /**
     * @brief Socket that selects TCP or TLS based on connection settings.
     * Delegates work to PlatformSocket (TCP) or PlatformSecureSocket (TLS).
     */
    class SecureSocket final
        : public Socket
        , public std::enable_shared_from_this<SecureSocket>
    {
    public:
        explicit SecureSocket(mqtt::ConnectionSettingsPtr settings);

        ~SecureSocket() override;

        const char* getImplementationId() const override
        {
            return "SecureSocket";
        }

        OnConnectCallback& getOnConnectCallback() override
        {
            return m_onConnect;
        }

        OnDisconnectCallback& getOnDisconnectCallback() override
        {
            return m_onDisconnect;
        }

        OnDataReceivedCallback& getOnDataReceivedCallback() override
        {
            return m_onDataReceived;
        }

    private:
        void connect() override;

        void disconnect() override;

        void close(int32_t code, const std::string& reason) override;

        [[nodiscard]] bool isConnected() const override;

        void send(const uint8_t* data, uint32_t size) override;

        void tick() override;

        bool readAvailableData();

        static constexpr int kMaxChunkSize = 64 * 1024;

        std::unique_ptr<PlatformSocket> m_socketPtr;
        std::atomic<bool> m_connectCallbackInvoked{ false };

        mutable std::recursive_mutex m_resourceMutex;

        OnConnectCallback m_onConnect;
        OnDisconnectCallback m_onDisconnect;
        OnDataReceivedCallback m_onDataReceived;
    };

    inline SocketPtr CreateSecureSocket(const mqtt::ConnectionSettingsPtr& settings)
    {
        return std::make_shared<SecureSocket>(settings);
    }
} // namespace reactormq::socket
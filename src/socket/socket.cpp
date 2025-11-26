//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "socket.h"

#include "config/platform_config.h"
#include "secure_socket.h"
#include "util/logging/logging.h"

using SelectedSocket = reactormq::socket::SecureSocket;

#if REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
#include "socket/ue5_websocket.h"
using SelectedWebSocket = reactormq::socket::Ue5WebSocket;
#else // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
// TODO: replace with real native websocket if we add Libwebsockets or similar
using SelectedWebSocket = reactormq::socket::SecureSocket;
#endif // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5

namespace reactormq::socket
{
    SocketPtr CreateSocket(const mqtt::ConnectionSettingsPtr& settings)
    {
        static_assert(!(config::kIsO3DESocket && config::kIsSocketUE5), "Only one socket backend may be enabled.");

        if (!settings)
        {
            REACTORMQ_LOG(logging::LogLevel::Critical, "settings is null");
            return nullptr;
        }

        switch (settings->getProtocol())
        {
            using enum mqtt::ConnectionProtocol;
        case Tcp:
        case Tls:
            return std::make_shared<SelectedSocket>(settings);

        case Ws:
        case Wss:
            return std::make_shared<SelectedWebSocket>(settings);

        default:
            REACTORMQ_LOG(logging::LogLevel::Error, "Unknown protocol");
            return nullptr;
        }
    }

    bool Socket::processPacketData(const uint8_t* data, const size_t size)
    {
        if (size == 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "SecureSocket::appendAndProcess() called with size=0");
            return true;
        }
        const mqtt::ConnectionSettingsPtr settings = getSettings();

        bool shouldDisconnect = false;
        size_t curSize = 0;
        uint32_t capBytes = 0;

        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "SecureSocket::appendAndProcess() appending size=%zd", size);

            m_dataBuffer.insert(m_dataBuffer.end(), data, data + size);
            curSize = m_dataBuffer.size();

            constexpr uint32_t defaultCap = 4 * 1024 * 1024;
            capBytes = settings ? settings->getMaxBufferSize() : defaultCap;

            if (curSize > static_cast<size_t>(capBytes))
            {
                shouldDisconnect = true;
            }
        }

        if (shouldDisconnect)
        {
            const auto curSizeAsSizeT = curSize;
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "SecureSocket::appendAndProcess() inbound buffer exceeded cap; disconnecting (size=%zu, cap=%u)",
                curSizeAsSizeT,
                capBytes);
            return false;
        }

        if (!readPacketsFromBuffer())
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "SecureSocket::appendAndProcess() readPacketsFromBuffer() reported oversized/invalid packet; disconnecting");
            return false;
        }

        return true;
    }
} // namespace reactormq::socket
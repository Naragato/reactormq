#include "socket_base.h"

#include "config/platform_config.h"
#include "util/logging/logging.h"

#ifdef REACTORMQ_WITH_O3DE_SOCKET
#include "socket/o3de/o3de_socket.h"
using SelectedSocket = reactormq::socket::O3deSocket;
#endif // REACTORMQ_WITH_O3DE_SOCKET
#ifdef REACTORMQ_WITH_FSOCKET_UE5
#include "socket/ue5/ue5_socket.h"
using SelectedSocket = reactormq::socket::Ue5Socket;
#endif // REACTORMQ_WITH_FSOCKET_UE5

#if !defined(REACTORMQ_WITH_O3DE_SOCKET) && !defined(REACTORMQ_WITH_FSOCKET_UE5)
#include "native/native_socket.h"
using SelectedSocket = reactormq::socket::NativeSocket;
#endif // #if !defined(REACTORMQ_WITH_O3DE_SOCKET) && !defined(REACTORMQ_WITH_FSOCKET_UE5)

#ifdef REACTORMQ_WITH_WEBSOCKET_UE5
#include "socket/ue5/ue5_websocket.h"
using SelectedWebSocket = reactormq::socket::Ue5WebSocket;
#else // REACTORMQ_WITH_WEBSOCKET_UE5
// TODO: replace with real native websocket if we add Libwebsockets or similar
using SelectedWebSocket = reactormq::socket::NativeSocket;
#endif // REACTORMQ_WITH_WEBSOCKET_UE5

namespace reactormq::socket
{
    SocketPtr CreateSocket(const mqtt::ConnectionSettingsPtr& settings)
    {
        static_assert(
            !(config::kIsO3DESocket && config::kIsSocketUE5),
            "Only one socket backend may be enabled.");

        if (!settings)
        {
            REACTORMQ_LOG(logging::LogLevel::Critical, "settings is null");
            return nullptr;
        }

        switch (settings->getProtocol())
        {
        case mqtt::ConnectionProtocol::Tcp:
        case mqtt::ConnectionProtocol::Tls:
            return std::make_shared<SelectedSocket>(settings);

        case mqtt::ConnectionProtocol::Ws:
        case mqtt::ConnectionProtocol::Wss:
            return std::make_shared<SelectedWebSocket>(settings);

        default:
            REACTORMQ_LOG(logging::LogLevel::Error, "Unknown protocol");
            return nullptr;
        }
    }
}
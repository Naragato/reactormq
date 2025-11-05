#pragma once

#include <cstdint>
#include <limits>

namespace reactormq::mqtt
{
    /// @brief Enumerates supported MQTT connection protocols.
    enum class ConnectionProtocol : uint8_t
    {

        /// @brief MQTT over tcp
        Tcp,
        /// @brief MQTT over tls/ssl
        Tls,
        /// @brief MQTT over WebSocket
        Ws,
        /// @brief MQTT over secure WebSocket (TLS)
        Wss,
        /// @brief Unknown Protocol
        Unknown = std::numeric_limits<uint8_t>::max(),
    };
}

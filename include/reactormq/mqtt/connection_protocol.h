//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>
#include <limits>

namespace reactormq::mqtt
{
    /**
     * @brief Supported transport protocols for establishing the MQTT connection.
     *
     * These are transport options (TCP/TLS/WebSocket), not the MQTT wire version.
     */
    enum class ConnectionProtocol : uint8_t
    {
        Tcp, ///< MQTT over TCP.
        Tls, ///< MQTT over TLS/SSL.
        Ws, ///< MQTT over WebSocket.
        Wss, ///< MQTT over secure WebSocket (TLS).
        Unknown = std::numeric_limits<uint8_t>::max(), ///< Sentinel for an unknown or unsupported protocol.
    };
} // namespace reactormq::mqtt
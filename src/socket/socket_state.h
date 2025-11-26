//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>
#include <string>

namespace reactormq::socket
{
    /// @brief Enum representing the state of a socket connection.
    enum class SocketState : uint8_t
    {
        Disconnected,
        Connecting,
        SslConnecting,
        Connected,
        Disconnecting
    };

    /**
     * @brief Convert a SocketState enum to a string representation.
     * @param state The socket state to convert.
     * @return String representation of the socket state.
     */
    inline std::string socketStateToString(const SocketState state)
    {
        switch (state)
        {
            using enum SocketState;
        case Disconnected:
            return "Disconnected";
        case Connecting:
            return "Connecting";
        case SslConnecting:
            return "SslConnecting";
        case Connected:
            return "Connected";
        case Disconnecting:
            return "Disconnecting";
        default:
            return "Unknown";
        }
    }
} // namespace reactormq::socket
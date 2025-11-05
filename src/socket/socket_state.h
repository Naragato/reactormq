#pragma once

#include <cstdint>
#include <string>

namespace reactormq::socket
{
    /**
     * @brief Enum representing the state of a socket connection.
     */
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
            case SocketState::Disconnected:
                return "Disconnected";
            case SocketState::Connecting:
                return "Connecting";
            case SocketState::SslConnecting:
                return "SslConnecting";
            case SocketState::Connected:
                return "Connected";
            case SocketState::Disconnecting:
                return "Disconnecting";
            default:
                return "Unknown";
        }
    }
} // namespace reactormq::socket

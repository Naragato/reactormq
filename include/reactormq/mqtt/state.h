#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief The state of the MQTT client.
     * 
     * Represents the current connection state of the client.
     */
    enum class State : uint8_t
    {
        /// @brief The client is disconnected and will only reconnect when triggered manually.
        Disconnected,

        /// @brief The client is connecting. If connection fails, it will retry based on the reconnect policy.
        Connecting,

        /// @brief The client is connected.
        Connected,

        /// @brief The client is disconnecting. Only used for graceful disconnects.
        Disconnecting
    };

    /**
     * @brief Convert State enum to string representation.
     * @param state The state to convert.
     * @return String representation of the state.
     */
    inline const char* stateToString(const State state)
    {
        switch (state)
        {
            case State::Disconnected:
                return "Disconnected";
            case State::Connecting:
                return "Connecting";
            case State::Connected:
                return "Connected";
            case State::Disconnecting:
                return "Disconnecting";
            default:
                return "Unknown";
        }
    }
} // namespace reactormq::mqtt

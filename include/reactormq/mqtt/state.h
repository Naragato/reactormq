//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT client connection state.
     * Describes the current lifecycle stage of the client.
     */
    enum class State : uint8_t
    {
        Disconnected, ///< No active connection; reconnect only when triggered.
        Connecting, ///< Attempting to connect; retries may occur per policy.
        Connected, ///< Connected to the broker.
        Disconnecting ///< Performing a graceful disconnect.
    };

    /**
     * @brief Convert a State value to a human-readable string.
     * @param state The state to convert.
     * @return A constant C-string describing the state.
     */
    inline const char* stateToString(const State state)
    {
        switch (state)
        {
            using enum State;
        case Disconnected:
            return "Disconnected";
        case Connecting:
            return "Connecting";
        case Connected:
            return "Connected";
        case Disconnecting:
            return "Disconnecting";
        default:
            return "Unknown";
        }
    }
} // namespace reactormq::mqtt
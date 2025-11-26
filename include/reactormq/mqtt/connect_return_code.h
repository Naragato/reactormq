//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 3.1.1 CONNACK return codes.
     * Maps the second byte of CONNACK to a simple enum.
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349257
     */
    enum class ConnectReturnCode : uint8_t
    {
        Accepted = 0, ///< Accepted.
        RefusedProtocolVersion = 1, ///< Unacceptable protocol version.
        RefusedIdentifierRejected = 2, ///< Client identifier rejected.
        RefusedServerUnavailable = 3, ///< Server unavailable.
        RefusedBadUserNameOrPassword = 4, ///< Bad user name or password.
        RefusedNotAuthorized = 5, ///< Not authorized.
        Cancelled = std::numeric_limits<uint8_t>::max(), ///< Cancelled by local code.
    };
} // namespace reactormq::mqtt
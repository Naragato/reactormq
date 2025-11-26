//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief MQTT protocol versions by spec level.
     * Values match the protocol level numbers used on the wire.
     */
    enum class ProtocolVersion : uint8_t
    {
        V311 = 4,
        V5 = 5
    };
} // namespace reactormq::mqtt::packets
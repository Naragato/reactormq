#pragma once

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     *
     * @brief Public API: Protocol version for MQTT
     * Values correspond to the MQTT specification protocol level numbers.
     * - 3.1.1 uses level 4
     * - 5.0   uses level 5
     */
    enum class ProtocolVersion : uint8_t
    {
        V311 = 4,
        V5 = 5
    };
} // namespace reactormq::mqtt::packets

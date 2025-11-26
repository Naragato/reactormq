//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT Quality of Service levels.
     */
    enum class QualityOfService : uint8_t
    {
        AtMostOnce = 0, ///< QoS 0 - At most once (no acknowledgement)
        AtLeastOnce = 1, ///< QoS 1 - At least once (acknowledged)
        ExactlyOnce = 2 ///< QoS 2 - Exactly once (acknowledged)
    };

    /**
     * @brief Convert QoS enum to a human-readable string.
     * @param qos QoS level to convert.
     * @return String view of the QoS level.
     */
    inline const char* qualityOfServiceToString(const QualityOfService qos)
    {
        switch (qos)
        {
            using enum QualityOfService;
        case AtMostOnce:
            return "QoS 0 - At most once";
        case AtLeastOnce:
            return "QoS 1 - At least once";
        case ExactlyOnce:
            return "QoS 2 - Exactly once";
        default:
            return "Invalid Quality of Service";
        }
    }
} // namespace reactormq::mqtt
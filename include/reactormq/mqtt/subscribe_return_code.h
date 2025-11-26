//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 3.1.1 SUBACK return codes.
     * Reports the outcome for each topic filter in a SUBSCRIBE.
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718068
     */
    enum class SubscribeReturnCode : uint8_t
    {
        SuccessQualityOfService0 = 0, ///< Success with QoS 0.
        SuccessQualityOfService1 = 1, ///< Success with QoS 1.
        SuccessQualityOfService2 = 2, ///< Success with QoS 2.
        Failure = 128 ///< Subscription failed.
    };
} // namespace reactormq::mqtt
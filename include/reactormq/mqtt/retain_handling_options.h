//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 5 retain handling options.
     * Controls delivery of retained messages at subscribe time.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169
     */
    enum class RetainHandlingOptions : uint8_t
    {
        /// @brief Send retained messages at subscribe time.
        SendRetainedMessagesAtSubscribeTime = 0,

        /// @brief Send retained messages only if the subscription does not already exist.
        SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist = 1,

        /// @brief Do not send retained messages at subscribe time.
        DontSendRetainedMessagesAtSubscribeTime = 2,

        /// @brief Maximum enum value (for validation).
        Max = 3
    };
} // namespace reactormq::mqtt
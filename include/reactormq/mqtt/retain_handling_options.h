#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 5 retain handling options.
     * 
     * Specifies whether retained messages are sent when the subscription is established.
     * This does not affect the sending of retained messages at any point after the subscribe.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169
     */
    enum class RetainHandlingOptions : uint8_t
    {
        /**
         * @brief Send retained messages at the time of the subscribe.
         */
        SendRetainedMessagesAtSubscribeTime = 0,

        /**
         * @brief Send retained messages at subscribe time if the subscription does not currently exist.
         */
        SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist = 1,

        /**
         * @brief Do not send retained messages at the time of the subscribe.
         */
        DontSendRetainedMessagesAtSubscribeTime = 2,

        /**
         * @brief Maximum value (for validation).
         */
        Max = 3
    };

} // namespace reactormq::mqtt

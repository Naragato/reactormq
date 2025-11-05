#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 3.1.1 subscription return codes.
     *
     * Used in SUBACK packets to indicate the result of each subscription request.
     * 
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718068
     */
    enum class SubscribeReturnCode : uint8_t
    {
        /**
         * @brief Success with Quality of Service level 0.
         * Indicates a successful subscription acknowledgment with QoS level 0.
         */
        SuccessQualityOfService0 = 0,

        /**
         * @brief Success with Quality of Service level 1.
         * Indicates a successful subscription acknowledgment with QoS level 1.
         */
        SuccessQualityOfService1 = 1,

        /**
         * @brief Success with Quality of Service level 2.
         * Indicates a successful subscription acknowledgment with QoS level 2.
         */
        SuccessQualityOfService2 = 2,

        /**
         * @brief Failure.
         * Indicates a failure in subscription acknowledgment.
         */
        Failure = 128
    };
} // namespace reactormq::mqtt

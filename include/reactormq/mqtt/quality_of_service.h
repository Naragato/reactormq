#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT Quality of Service levels.
     *
     */
    enum class QualityOfService : uint8_t
    {
        /// @brief QoS 0: At most once (not guaranteed)
        /// The message is delivered at most once, and delivery is not confirmed.
        AtMostOnce = 0,

        /// @brief QoS 1: At least once (guaranteed)
        /// The message is delivered at least once, and delivery is confirmed.
        AtLeastOnce = 1,

        /// @brief QoS 2: Exactly once (guaranteed)
        /// The message is delivered exactly once by using a four-step handshake.
        ExactlyOnce = 2
    };

    /**
     * @brief Convert QualityOfService enum to string representation.
     * @param qos The QoS level to convert.
     * @return String representation of the QoS level.
     */
    inline const char* qualityOfServiceToString(const QualityOfService qos)
    {
        switch (qos)
        {
            case QualityOfService::AtMostOnce:
                return "QoS 0 - At most once";
            case QualityOfService::AtLeastOnce:
                return "QoS 1 - At least once";
            case QualityOfService::ExactlyOnce:
                return "QoS 2 - Exactly once";
            default:
                return "Invalid Quality of Service";
        }
    }
}

#pragma once

#include <cstdint>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT 3.1.1 Connect Return Codes (CONNACK byte 2).
     * Spec: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349257
     */
    enum class ConnectReturnCode : uint8_t
    {
        /** The Connection is accepted by the Server. */
        Accepted = 0,
        /** The Server does not support the requested protocol level. */
        RefusedProtocolVersion = 1,
        /** The Client identifier is correct UTF-8 but not allowed by the Server. */
        RefusedIdentifierRejected = 2,
        /** The Network Connection has been made but the MQTT service is unavailable. */
        RefusedServerUnavailable = 3,
        /** The data in the user name or password is malformed. */
        RefusedBadUserNameOrPassword = 4,
        /** The Client is not authorized to connect. */
        RefusedNotAuthorized = 5,
        /** Non-spec convenience value used by higher layers to signal local cancellation. */
        Cancelled = 255
    };
}

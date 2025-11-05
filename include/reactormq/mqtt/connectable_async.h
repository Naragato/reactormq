#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/result.h"

#include <future>



namespace reactormq::mqtt
{
    using ConnectFuture = std::future<Result<void>>;

    /**
     * @brief Interface for a client that can connect to an MQTT broker.
     */
    class REACTORMQ_API IConnectableAsync
    {
    public:
        virtual ~IConnectableAsync() = default;

        /**
         * @brief Connect to the MQTT broker.
         * @param cleanSession Whether to start a clean session (true) or attempt to resume (false).
         * @return A future that resolves to the result of the connection attempt.
         */
        virtual ConnectFuture connectAsync(bool cleanSession) = 0;
    };
}

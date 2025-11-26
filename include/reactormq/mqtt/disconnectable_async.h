//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/result.h"

#include <future>

namespace reactormq::mqtt
{
    using DisconnectFuture = std::future<Result<void>>;

    /**
     * @brief Interface for a client that can disconnect from an MQTT broker.
     */
    class REACTORMQ_API IDisconnectableAsync
    {
    public:
        virtual ~IDisconnectableAsync() = default;

        /**
         * @brief Disconnect from the MQTT broker.
         * @return A future that resolves to the result of the disconnection.
         */
        virtual DisconnectFuture disconnectAsync() = 0;
    };
} // namespace reactormq::mqtt
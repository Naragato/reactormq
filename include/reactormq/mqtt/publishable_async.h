//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/result.h"

#include <future>

namespace reactormq::mqtt
{
    using PublishFuture = std::future<Result<void>>;

    /**
     * @brief Interface for a client that can publish messages.
     */
    class REACTORMQ_API IPublishableAsync
    {
    public:
        virtual ~IPublishableAsync() = default;

        /**
         * @brief Publish a message to the connected MQTT broker.
         * @param message The message to publish (moved).
         * @return A future that resolves to a Result<void> indicating success.
         */
        virtual PublishFuture publishAsync(Message&& message) = 0;
    };
} // namespace reactormq::mqtt
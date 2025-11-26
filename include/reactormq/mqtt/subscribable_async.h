//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/result.h"
#include "reactormq/mqtt/subscribe_result.h"
#include "reactormq/mqtt/topic_filter.h"

#include <future>
#include <string>
#include <vector>

namespace reactormq::mqtt
{
    using SubscribesFuture = std::future<Result<std::vector<SubscribeResult>>>;
    using SubscribeFuture = std::future<Result<SubscribeResult>>;

    /**
     * @brief Interface for a client that can subscribe to topics.
     */
    class REACTORMQ_API ISubscribableAsync
    {
    public:
        virtual ~ISubscribableAsync() = default;

        /**
         * @brief Subscribe to multiple topic filters.
         * @param topicFilters The set of topic filters to subscribe to.
         * @return A future resolving to per-topic results for the subscribe request.
         */
        virtual SubscribesFuture subscribeAsync(const std::vector<TopicFilter>& topicFilters) = 0;

        /**
         * @brief Subscribe to a single topic filter.
         * @param topicFilter The topic filter to subscribe to (moved).
         * @return A future resolving to the result for the single subscription.
         */
        virtual SubscribeFuture subscribeAsync(TopicFilter&& topicFilter) = 0;

        /**
         * @brief Convenience overload: subscribe using a single filter string.
         * @param topicFilter The topic filter string (e.g., "sensors/+/temp").
         * @return A future resolving to the result for the single subscription.
         */
        virtual SubscribeFuture subscribeAsync(const std::string& topicFilter) = 0;
    };
} // namespace reactormq::mqtt
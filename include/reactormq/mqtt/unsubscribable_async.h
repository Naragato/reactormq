//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/result.h"
#include "reactormq/mqtt/unsubscribe_result.h"

#include <future>
#include <string>
#include <vector>

namespace reactormq::mqtt
{
    using UnsubscribesFuture = std::future<Result<std::vector<UnsubscribeResult>>>;

    /**
     * @brief Interface for a client that can unsubscribe from topics.
     */
    class REACTORMQ_API IUnsubscribableAsync
    {
    public:
        virtual ~IUnsubscribableAsync() = default;

        /**
         * @brief Unsubscribe from the provided topic filters.
         * @param topicFilters The topic filters to unsubscribe from (order preserved for result mapping).
         * @return A future resolving to the per-topic unsubscribe results.
         */
        virtual UnsubscribesFuture unsubscribeAsync(const std::vector<std::string>& topicFilters) = 0;
    };
} // namespace reactormq::mqtt
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <utility>

#include "reactormq/export.h"
#include "reactormq/mqtt/topic_filter.h"

namespace reactormq::mqtt
{
    /**
     * @brief Result of an unsubscribe attempt.
     */
    struct REACTORMQ_API UnsubscribeResult final
    {
    public:
        /**
         * @brief Construct an unsubscribe result.
         * @param filter The topic filter that was requested to be unsubscribed.
         * @param wasSuccessful Whether the unsubscribe succeeded for this filter.
         */
        explicit UnsubscribeResult(const TopicFilter& filter, const bool wasSuccessful)
            : m_filter(filter)
            , m_wasSuccessful(wasSuccessful)
        {
        }

        /**
         * @brief Construct an unsubscribe result (rvalue overload).
         * @param filter The topic filter that was requested to be unsubscribed (moved).
         * @param wasSuccessful Whether the unsubscribe succeeded for this filter.
         */
        explicit UnsubscribeResult(TopicFilter&& filter, const bool wasSuccessful)
            : m_filter(std::move(filter))
            , m_wasSuccessful(wasSuccessful)
        {
        }

        ~UnsubscribeResult() = default;

        UnsubscribeResult(UnsubscribeResult&&) = default;

        UnsubscribeResult& operator=(UnsubscribeResult&&) = default;

        /**
         * @brief Get the topic filter associated with this result.
         * @return The topic filter.
         */
        [[nodiscard]] const TopicFilter& getFilter() const
        {
            return m_filter;
        }

        /**
         * @brief Whether the unsubscribe operation succeeded.
         * @return True if the unsubscribe was successful.
         */
        [[nodiscard]] bool wasSuccessful() const
        {
            return m_wasSuccessful;
        }

    private:
        TopicFilter m_filter;
        bool m_wasSuccessful{ false };
    };
} // namespace reactormq::mqtt
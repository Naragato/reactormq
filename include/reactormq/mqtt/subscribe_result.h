//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/delegates.h"
#include "reactormq/mqtt/topic_filter.h"

#include <memory>
#include <utility>

namespace reactormq::mqtt
{
    /// @brief Result of a subscription attempt.
    struct REACTORMQ_API SubscribeResult final
    {
    public:
        /**
         * @brief Construct a subscription result object.
         * @param filter The topic filter for which the result applies.
         * @param wasSuccessful Whether the subscribe request for this filter succeeded.
         * @param onMessage Optional per-filter message callback assigned on success.
         */
        SubscribeResult(TopicFilter filter, const bool wasSuccessful, const std::shared_ptr<OnMessage>& onMessage = nullptr)
            : m_filter(std::move(filter))
            , m_wasSuccessful(wasSuccessful)
            , m_onMessage(onMessage)
        {
        }

        ~SubscribeResult() = default;

        SubscribeResult(SubscribeResult&&) = default;

        SubscribeResult& operator=(SubscribeResult&&) = default;

        /**
         * @brief Get the topic filter for this result.
         * @return The topic filter.
         */
        [[nodiscard]] const TopicFilter& getFilter() const
        {
            return m_filter;
        }

        /**
         * @brief Whether the subscribe operation succeeded for this filter.
         * @return True if the subscribe was successful.
         */
        [[nodiscard]] bool wasSuccessful() const
        {
            return m_wasSuccessful;
        }

        /**
         * @brief Get the per-filter OnMessage callback if set.
         * @return Shared pointer to the callback or null if not set.
         */
        [[nodiscard]] std::shared_ptr<OnMessage> getOnMessage() const
        {
            return m_onMessage;
        }

    private:
        TopicFilter m_filter;
        bool m_wasSuccessful{ false };
        std::shared_ptr<OnMessage> m_onMessage;
    };
} // namespace reactormq::mqtt
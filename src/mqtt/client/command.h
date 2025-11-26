//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/message.h"
#include "reactormq/mqtt/result.h"
#include "reactormq/mqtt/subscribe_result.h"
#include "reactormq/mqtt/topic_filter.h"
#include "reactormq/mqtt/unsubscribe_result.h"

#include <future>
#include <variant>
#include <vector>

namespace reactormq::mqtt::client
{
    /**
     * @brief Command to initiate connection to broker.
     */
    struct ConnectCommand
    {
        bool cleanSession;
        std::promise<Result<void>> promise;
    };

    /**
     * @brief Command to publish a message.
     */
    struct PublishCommand
    {
        Message message;
        std::promise<Result<void>> promise;
    };

    /**
     * @brief Command to subscribe to multiple topic filters.
     */
    struct SubscribesCommand
    {
        std::vector<TopicFilter> topicFilters;
        std::promise<Result<std::vector<SubscribeResult>>> promise;
    };

    /**
     * @brief Command to subscribe to a single topic filter.
     */
    struct SubscribeCommand
    {
        TopicFilter topicFilter;
        std::promise<Result<SubscribeResult>> promise;
    };

    /**
     * @brief Command to unsubscribe from multiple topics.
     */
    struct UnsubscribesCommand
    {
        std::vector<std::string> topics;
        std::promise<Result<std::vector<UnsubscribeResult>>> promise;
    };

    /**
     * @brief Command to unsubscribe from a single topic.
     */
    struct UnsubscribeCommand
    {
        std::string topic;
        std::promise<Result<UnsubscribeResult>> promise;
    };

    /**
     * @brief Command to disconnect from broker.
     */
    struct DisconnectCommand
    {
        std::promise<Result<void>> promise;
    };

    /**
     * @brief Command to close the socket.
     */
    struct CloseSocketCommand
    {
        std::int32_t code;
        std::string reason;
    };

    /**
     * @brief Variant holding all possible command types.
     */
    using Command = std::variant<
        ConnectCommand,
        PublishCommand,
        SubscribesCommand,
        SubscribeCommand,
        UnsubscribesCommand,
        UnsubscribeCommand,
        DisconnectCommand,
        CloseSocketCommand>;
} // namespace reactormq::mqtt::client
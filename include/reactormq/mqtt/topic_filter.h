//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "reactormq/export.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/retain_handling_options.h"

namespace reactormq::mqtt
{
    /**
     * @brief Topic filter plus subscribe options.
     * Captures the filter string and per-subscription flags for MQTT 3.1.1 and 5.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901241
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718106
     */
    class REACTORMQ_API TopicFilter
    {
    public:
        TopicFilter()
            : m_qualityOfService(QualityOfService::AtMostOnce)
            , m_noLocal(true)
            , m_retainAsPublished(true)
            , m_retainHandlingOptions(RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
        {
        }

        /**
         * @brief Create a topic filter with optional subscribe options.
         * @param filter Topic filter string.
         * @param qos Maximum QoS accepted for incoming messages.
         * @param noLocal If true, do not receive messages published by this client (MQTT 5).
         * @param retainAsPublished If true, keep the message's original RETAIN flag (MQTT 5).
         * @param retainHandlingOptions Delivery of retained messages at subscribe time (MQTT 5).
         */
        explicit TopicFilter(
            const std::string_view filter,
            const QualityOfService qos = QualityOfService::AtMostOnce,
            const bool noLocal = true,
            const bool retainAsPublished = true,
            const RetainHandlingOptions retainHandlingOptions = RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
            : m_filter(filter)
            , m_qualityOfService(qos)
            , m_noLocal(noLocal)
            , m_retainAsPublished(retainAsPublished)
            , m_retainHandlingOptions(retainHandlingOptions)
        {
        }

        /**
         * @brief Create a topic filter with optional subscribe options.
         * Parameter semantics match the other constructor.
         */
        explicit TopicFilter(
            const char* filter,
            const QualityOfService qos = QualityOfService::AtMostOnce,
            const bool noLocal = true,
            const bool retainAsPublished = true,
            const RetainHandlingOptions retainHandlingOptions = RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
            : TopicFilter(std::string_view{ filter }, qos, noLocal, retainAsPublished, retainHandlingOptions)
        {
        }

        /**
         * @brief Create a topic filter with optional subscribe options.
         * Parameter semantics match the other constructor.
         */
        explicit TopicFilter(
            std::string&& filter,
            const QualityOfService qos = QualityOfService::AtMostOnce,
            const bool noLocal = true,
            const bool retainAsPublished = true,
            const RetainHandlingOptions retainHandlingOptions = RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
            : m_filter(std::move(filter))
            , m_qualityOfService(qos)
            , m_noLocal(noLocal)
            , m_retainAsPublished(retainAsPublished)
            , m_retainHandlingOptions(retainHandlingOptions)
        {
        }

        bool operator==(const TopicFilter& other) const = default;

        bool operator==(const std::string_view other) const
        {
            return m_filter == other;
        }

        [[nodiscard]] const std::string& getFilter() const
        {
            return m_filter;
        }

        [[nodiscard]] QualityOfService getQualityOfService() const
        {
            return m_qualityOfService;
        }

        [[nodiscard]] bool getIsNoLocal() const
        {
            return m_noLocal;
        }

        [[nodiscard]] bool getIsRetainAsPublished() const
        {
            return m_retainAsPublished;
        }

        [[nodiscard]] RetainHandlingOptions getRetainHandlingOptions() const
        {
            return m_retainHandlingOptions;
        }

    private:
        std::string m_filter;
        QualityOfService m_qualityOfService;
        bool m_noLocal;
        bool m_retainAsPublished;
        RetainHandlingOptions m_retainHandlingOptions;
    };
} // namespace reactormq::mqtt
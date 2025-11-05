#pragma once

#include <string>
#include <utility>

#include "reactormq/export.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/retain_handling_options.h"

namespace reactormq::mqtt
{
    /**
     * @brief Represents an MQTT topic filter for subscribe operations.
     *
     * Contains the topic filter string and subscription options for MQTT 3.1.1 and 5.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901241
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718106
     */
    class REACTORMQ_API TopicFilter
    {
    public:
        /**
         * @brief Default constructor.
         */
        TopicFilter()
            : m_qualityOfService(QualityOfService::AtMostOnce)
              , m_noLocal(true)
              , m_retainAsPublished(true)
              , m_retainHandlingOptions(RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
        {
        }

        /**
         * @brief Constructor with filter and options.
         * @param filter Topic filter string.
         * @param qos Maximum QoS level at which the Server can send messages to the Client.
         * @param noLocal If true, Application Messages forwarded using this subscription do not get sent back to the Client that published them (MQTT 5 only).
         * @param retainAsPublished If true, Application Messages forwarded using this subscription keep the RETAIN flag they were published with (MQTT 5 only).
         * @param retainHandlingOptions Specifies whether retained messages are sent when the subscription is established (MQTT 5 only).
         */
        explicit TopicFilter(
            const std::string& filter,
            const QualityOfService qos = QualityOfService::AtMostOnce,
            const bool noLocal = true,
            const bool retainAsPublished = true,
            const RetainHandlingOptions retainHandlingOptions =
                    RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
            : m_filter(filter)
              , m_qualityOfService(qos)
              , m_noLocal(noLocal)
              , m_retainAsPublished(retainAsPublished)
              , m_retainHandlingOptions(retainHandlingOptions)
        {
        }

        /**
         * @brief Constructor with filter and options.
         * @param filter Topic filter string.
         * @param qos Maximum QoS level at which the Server can send messages to the Client.
         * @param noLocal If true, Application Messages forwarded using this subscription do not get sent back to the Client that published them (MQTT 5 only).
         * @param retainAsPublished If true, Application Messages forwarded using this subscription keep the RETAIN flag they were published with (MQTT 5 only).
         * @param retainHandlingOptions Specifies whether retained messages are sent when the subscription is established (MQTT 5 only).
         */
        explicit TopicFilter(
            std::string&& filter,
            const QualityOfService qos = QualityOfService::AtMostOnce,
            const bool noLocal = true,
            const bool retainAsPublished = true,
            const RetainHandlingOptions retainHandlingOptions =
                    RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
            : m_filter(std::move(filter))
              , m_qualityOfService(qos)
              , m_noLocal(noLocal)
              , m_retainAsPublished(retainAsPublished)
              , m_retainHandlingOptions(retainHandlingOptions)
        {
        }

        /**
         * @brief Equality operator.
         * @param other TopicFilter to compare with.
         * @return True if equal, false otherwise.
         */
        bool operator==(const TopicFilter& other) const
        {
            return m_filter == other.m_filter &&
                   m_qualityOfService == other.m_qualityOfService &&
                   m_noLocal == other.m_noLocal &&
                   m_retainAsPublished == other.m_retainAsPublished &&
                   m_retainHandlingOptions == other.m_retainHandlingOptions;
        }

        /**
         * @brief Equality operator with string.
         * @param other String to compare filter with.
         * @return True if filter matches string, false otherwise.
         */
        bool operator==(const std::string& other) const
        {
            return m_filter == other;
        }

        /**
         * @brief Inequality operator.
         * @param other TopicFilter to compare with.
         * @return True if not equal, false otherwise.
         */
        bool operator!=(const TopicFilter& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief Inequality operator with string.
         * @param other String to compare filter with.
         * @return True if filter doesn't match string, false otherwise.
         */
        bool operator!=(const std::string& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief Get the topic filter string.
         * @return The topic filter string.
         */
        [[nodiscard]] const std::string& getFilter() const
        {
            return m_filter;
        }

        /**
         * @brief Get the quality of service.
         * @return The maximum QoS level at which the Server can send messages.
         */
        [[nodiscard]] QualityOfService getQualityOfService() const
        {
            return m_qualityOfService;
        }

        /**
         * @brief Get the no local flag.
         * @return True if Application Messages published by this Client should not be sent back to it.
         */
        [[nodiscard]] bool getIsNoLocal() const
        {
            return m_noLocal;
        }

        /**
         * @brief Get the retain as published flag.
         * @return True if forwarded messages should keep their original RETAIN flag.
         */
        [[nodiscard]] bool getIsRetainAsPublished() const
        {
            return m_retainAsPublished;
        }

        /**
         * @brief Get the retain handling options.
         * @return The retain handling options for this subscription.
         */
        [[nodiscard]] RetainHandlingOptions getRetainHandlingOptions() const
        {
            return m_retainHandlingOptions;
        }

    private:
        /// @brief Topic filter string (can contain wildcards + and #).
        std::string m_filter;

        /// @brief Maximum QoS level at which the Server can send Application Messages to the Client.
        QualityOfService m_qualityOfService;

        /// @brief No Local flag (MQTT 5 only).
        bool m_noLocal;

        /// @brief Retain as Published flag (MQTT 5 only).
        bool m_retainAsPublished;

        /// @brief Retain Handling Options (MQTT 5 only).
        RetainHandlingOptions m_retainHandlingOptions;
    };
} // namespace reactormq::mqtt

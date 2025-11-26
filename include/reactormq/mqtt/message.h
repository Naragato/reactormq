//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/quality_of_service.h"

#include <chrono>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace reactormq::mqtt
{
    /// @brief Immutable MQTT message with topic, payload, retain flag, and QoS.
    struct REACTORMQ_API Message final
    {
    public:
        using Clock = std::chrono::system_clock;
        using Payload = std::vector<std::uint8_t>;

        Message() = default;

        /**
         * @brief Construct a message from movable topic and payload.
         * @param topic Topic to publish to.
         * @param payload Message payload bytes (moved).
         * @param shouldRetain Whether the broker should retain the message.
         * @param qualityOfService The QoS level for delivery.
         */
        Message(std::string&& topic, Payload&& payload, const bool shouldRetain, const QualityOfService qualityOfService) noexcept
            : m_topic{ std::move(topic) }
            , m_payload{ std::move(payload) }
            , m_shouldRetain{ shouldRetain }
            , m_qualityOfService{ qualityOfService }
        {
        }

        /**
         * @brief Construct a message from copied topic and payload.
         * @param topic Topic to publish to.
         * @param payload Message payload bytes (copied).
         * @param shouldRetain Whether the broker should retain the message.
         * @param qualityOfService The QoS level for delivery.
         */
        Message(const std::string& topic, const Payload& payload, const bool shouldRetain, const QualityOfService qualityOfService) noexcept
            : m_topic{ topic }
            , m_payload{ payload }
            , m_shouldRetain{ shouldRetain }
            , m_qualityOfService{ qualityOfService }
        {
        }

        /**
         * @brief Construct a message from movable topic and a span view of payload.
         * @param topic Topic to publish to.
         * @param payload Message payload bytes (copied from view).
         * @param shouldRetain Whether the broker should retain the message.
         * @param qualityOfService The QoS level for delivery.
         */
        Message(
            std::string&& topic,
            std::span<const std::uint8_t> payload,
            const bool shouldRetain,
            const QualityOfService qualityOfService) noexcept
            : m_topic{ std::move(topic) }
            , m_payload{ payload.begin(), payload.end() }
            , m_shouldRetain{ shouldRetain }
            , m_qualityOfService{ qualityOfService }
        {
        }

        Message(const Message&) = default;

        Message(Message&&) noexcept = default;

        Message& operator=(const Message&) = delete;

        Message& operator=(Message&&) = delete;

        /**
         * @brief Get the message creation timestamp in UTC.
         * @return Timestamp when the message object was constructed.
         */
        [[nodiscard]] Clock::time_point getTimestampUtc() const noexcept
        {
            return m_timestampUtc;
        }

        /**
         * @brief Get the message topic.
         * @return Topic string the message is published to.
         */
        [[nodiscard]] const std::string& getTopic() const noexcept
        {
            return m_topic;
        }

        /**
         * @brief Get the message payload bytes.
         * @return Reference to the payload buffer.
         */
        [[nodiscard]] const Payload& getPayload() const noexcept
        {
            return m_payload;
        }

        /**
         * @brief Get a non-owning view over the payload bytes.
         * @return Span view of the payload buffer.
         */
        [[nodiscard]] std::span<const std::uint8_t> getPayloadView() const noexcept
        {
            return { m_payload.data(), m_payload.size() };
        }

        /**
         * @brief Whether the broker should retain this message.
         * @return True if the retain flag is set.
         */
        [[nodiscard]] bool shouldRetain() const noexcept
        {
            return m_shouldRetain;
        }

        /**
         * @brief Get the Quality of Service level for this message.
         * @return The QoS level requested for delivery.
         */
        [[nodiscard]] QualityOfService getQualityOfService() const noexcept
        {
            return m_qualityOfService;
        }

    private:
        const Clock::time_point m_timestampUtc{ Clock::now() };
        const std::string m_topic{};
        const Payload m_payload{};
        const bool m_shouldRetain{ false };
        const QualityOfService m_qualityOfService{ QualityOfService::AtMostOnce };
    };
} // namespace reactormq::mqtt
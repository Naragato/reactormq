//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <chrono>
#include <random>

namespace reactormq::mqtt::client
{
    /**
     * @brief Calculates exponential backoff delays with jitter for reconnection attempts.
     *
     * Implements exponential backoff: delay = min(initialDelay * (multiplier ^ attempt), maxDelay)
     * with ±10% random jitter to prevent thundering herd problem.
     *
     */
    class BackoffCalculator
    {
    public:
        /**
         * @brief Constructor.
         * @param initialDelayMs Initial delay in milliseconds before first retry.
         * @param maxDelayMs Maximum delay in milliseconds between retries.
         * @param multiplier Exponential backoff multiplier (e.g., 2.0 for doubling).
         */
        BackoffCalculator(std::uint32_t initialDelayMs, std::uint32_t maxDelayMs, double multiplier);

        /**
         * @brief Calculate the next delay with exponential backoff and jitter.
         *
         * Each call increments the attempt counter and calculates:
         * baseDelay = min(initialDelay * (multiplier ^ attempt), maxDelay)
         * actualDelay = baseDelay * randomFactor where randomFactor is in [0.9, 1.1]
         *
         * @return The delay in milliseconds before the next retry attempt.
         */
        std::chrono::milliseconds calculateNextDelay();

        /**
         * @brief Reset the backoff state (attempt counter).
         *
         * Called when connection succeeds to reset backoff for future reconnection attempts.
         */
        void reset();

        /**
         * @brief Get the current attempt count.
         * @return Number of retry attempts made since last reset.
         */
        [[nodiscard]] std::uint32_t getAttemptCount() const
        {
            return m_attemptCount;
        }

    private:
        /**
         * @brief Apply jitter to a delay value (±10%).
         * @param baseDelayMs Base delay in milliseconds.
         * @return Jittered delay in milliseconds.
         */
        std::uint32_t applyJitter(std::uint32_t baseDelayMs);

        /// @brief Initial delay in milliseconds.
        std::uint32_t m_initialDelayMs;

        /// @brief Maximum delay in milliseconds (cap for exponential growth).
        std::uint32_t m_maxDelayMs;

        /// @brief Exponential backoff multiplier.
        double m_multiplier;

        /// @brief Current attempt count (incremented on each calculateNextDelay call).
        std::uint32_t m_attemptCount;

        /// @brief Random number generator for jitter (seeded with std::random_device).
        std::mt19937 m_rng;

        /// @brief Uniform distribution for jitter factor [0.9, 1.1].
        std::uniform_real_distribution<> m_jitterDistribution;
    };
} // namespace reactormq::mqtt::client
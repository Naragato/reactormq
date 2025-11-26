//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "backoff.h"

#include <algorithm>

namespace reactormq::mqtt::client
{
    BackoffCalculator::BackoffCalculator(const std::uint32_t initialDelayMs, const std::uint32_t maxDelayMs, const double multiplier)
        : m_initialDelayMs(initialDelayMs)
        , m_maxDelayMs(maxDelayMs)
        , m_multiplier(multiplier)
        , m_attemptCount(0)
        , m_rng(std::random_device{}())
        , m_jitterDistribution(0.9, 1.1)
    {
    }

    std::chrono::milliseconds BackoffCalculator::calculateNextDelay()
    {
        double baseDelay = m_initialDelayMs;

        if (m_attemptCount > 0)
        {
            baseDelay *= std::pow(m_multiplier, static_cast<double>(m_attemptCount));
        }

        baseDelay = std::min(baseDelay, static_cast<double>(m_maxDelayMs));

        const auto baseDelayMs = static_cast<std::uint32_t>(baseDelay);

        const std::uint32_t jitteredDelayMs = applyJitter(baseDelayMs);

        ++m_attemptCount;

        return std::chrono::milliseconds(jitteredDelayMs);
    }

    void BackoffCalculator::reset()
    {
        m_attemptCount = 0;
    }

    std::uint32_t BackoffCalculator::applyJitter(const std::uint32_t baseDelayMs)
    {
        const double jitterFactor = m_jitterDistribution(m_rng);

        const double raw = static_cast<double>(baseDelayMs) * jitterFactor;
        auto value = static_cast<std::uint32_t>(std::llround(raw));

        const auto minMs = static_cast<std::uint32_t>(std::floor(static_cast<double>(baseDelayMs) * 0.9));
        const auto maxMs = static_cast<std::uint32_t>(std::ceil(static_cast<double>(baseDelayMs) * 1.1));
        if (value < minMs)
        {
            value = minMs;
        }
        else if (value > maxMs)
        {
            value = maxMs;
        }

        return std::max(value, 1u);
    }
} // namespace reactormq::mqtt::client
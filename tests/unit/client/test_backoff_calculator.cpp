//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "mqtt/client/backoff.h"

#include <algorithm>

using reactormq::mqtt::client::BackoffCalculator;

namespace
{
    void expectWithinJitter(const uint32_t actualMs, const uint32_t baseMs)
    {
        const auto minMs = static_cast<uint32_t>(baseMs * 0.9);
        const auto maxMs = static_cast<uint32_t>(baseMs * 1.1);
        EXPECT_GE(actualMs, minMs);
        EXPECT_LE(actualMs, maxMs);
    }
} // namespace

TEST(BackoffCalculatorTest, InitialAttemptIsZeroBeforeAnyCall)
{
    const BackoffCalculator calc(100, 10'000, 2.0);
    EXPECT_EQ(calc.getAttemptCount(), 0u);
}

TEST(BackoffCalculatorTest, FirstCallUsesInitialDelayWithJitterRange)
{
    BackoffCalculator calc(200, 10'000, 2.0);
    const auto delay = calc.calculateNextDelay();
    expectWithinJitter(static_cast<uint32_t>(delay.count()), 200u);
    EXPECT_EQ(calc.getAttemptCount(), 1u);
}

TEST(BackoffCalculatorTest, DelayGrowsExponentiallyUntilMax)
{
    constexpr uint32_t initial = 100;
    constexpr double mult = 2.0;
    BackoffCalculator calc(initial, 5000, mult);

    const auto d0 = calc.calculateNextDelay();
    expectWithinJitter(static_cast<uint32_t>(d0.count()), initial);

    const auto d1 = calc.calculateNextDelay();
    expectWithinJitter(static_cast<uint32_t>(d1.count()), static_cast<uint32_t>(initial * mult));

    const auto d2 = calc.calculateNextDelay();
    expectWithinJitter(static_cast<uint32_t>(d2.count()), static_cast<uint32_t>(initial * mult * mult));
}

TEST(BackoffCalculatorTest, DelayIsCappedAtMaxDelay)
{
    constexpr uint32_t initial = 500;
    constexpr uint32_t maxDelay = 3000;
    BackoffCalculator calc(initial, maxDelay, 3.0);

    (void)calc.calculateNextDelay();
    (void)calc.calculateNextDelay();

    for (int i = 0; i < 10; ++i)
    {
        auto delay = calc.calculateNextDelay();
        EXPECT_LE(delay.count(), static_cast<long long>(static_cast<uint32_t>(maxDelay * 1.1)));
        EXPECT_GE(delay.count(), static_cast<long long>(static_cast<uint32_t>(maxDelay * 0.9)));
    }
}

TEST(BackoffCalculatorTest, JitterIsWithinMinus10ToPlus10Percent)
{
    BackoffCalculator calc(1000, 1000, 2.0);
    for (int i = 0; i < 5; ++i)
    {
        auto delay = calc.calculateNextDelay();
        expectWithinJitter(static_cast<uint32_t>(delay.count()), 1000u);
    }
}

TEST(BackoffCalculatorTest, ResetSetsAttemptCountBackToZero)
{
    BackoffCalculator calc(150, 10'000, 2.0);
    (void)calc.calculateNextDelay();
    EXPECT_EQ(calc.getAttemptCount(), 1u);
    calc.reset();
    EXPECT_EQ(calc.getAttemptCount(), 0u);
}

TEST(BackoffCalculatorTest, ResetAffectsNextDelayAsFirstAttemptAgain)
{
    BackoffCalculator calc(250, 10'000, 2.0);
    (void)calc.calculateNextDelay();
    (void)calc.calculateNextDelay();
    calc.reset();
    const auto d = calc.calculateNextDelay();
    expectWithinJitter(static_cast<uint32_t>(d.count()), 250u);
}
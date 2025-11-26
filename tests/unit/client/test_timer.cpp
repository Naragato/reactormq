//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "mqtt/client/timer.h"

#include <chrono>
#include <thread>

using namespace reactormq::mqtt::client;

TEST(TimerTest, ShouldFireTrueWhenNowAfterFireTimeAndActive)
{
    Timer t;
    t.isActive = true;
    t.fireTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
    EXPECT_TRUE(t.shouldFire(std::chrono::steady_clock::now()));
}

TEST(TimerTest, ShouldFireFalseWhenInactive)
{
    Timer t;
    t.isActive = false;
    t.fireTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
    EXPECT_FALSE(t.shouldFire(std::chrono::steady_clock::now()));
}

TEST(TimerTest, ReschedulePeriodicTimerMovesFireTimeForward)
{
    Timer t;
    t.isActive = true;
    t.isPeriodic = true;
    t.interval = std::chrono::milliseconds(5);
    t.fireTime = std::chrono::steady_clock::now();

    const auto before = t.fireTime;
    t.reschedule();
    // fireTime should move forward by approximately interval
    EXPECT_GT(t.fireTime, before);
}

TEST(TimerTest, RescheduleOneShotDeactivatesTimer)
{
    Timer t;
    t.isActive = true;
    t.isPeriodic = false;
    t.interval = std::chrono::milliseconds(0);
    t.fireTime = std::chrono::steady_clock::now();

    t.reschedule();
    EXPECT_FALSE(t.isActive);
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <chrono>
#include <functional>

namespace reactormq::mqtt::client
{
    using TimerCallback = std::function<void()>;
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::milliseconds;

    /**
     * @brief Simple timer for scheduling callbacks.
     *
     * Timers are checked and fired during the reactor's tick() cycle.
     * Supports one-shot and periodic timers.
     * n.b we can potentially delete this, we stuck with global timer / timestamp approach as this started getting messy,
     * and harder to reason about.
     */
    struct Timer
    {
        std::uint32_t id = 0;

        TimePoint fireTime;

        Duration interval{ 0 };

        TimerCallback callback;

        bool isPeriodic = false;

        bool isActive = true;

        /**
         * @brief Check if this timer should fire now.
         * @param now Current time point.
         * @return True if the timer should fire.
         */
        [[nodiscard]] bool shouldFire(const TimePoint now) const
        {
            return isActive && now >= fireTime;
        }

        /**
         * @brief Reschedule this timer for the next interval (for periodic timers).
         */
        void reschedule()
        {
            if (isPeriodic && interval.count() > 0)
            {
                fireTime = std::chrono::steady_clock::now() + interval;
            }
            else
            {
                isActive = false;
            }
        }
    };
} // namespace reactormq::mqtt::client
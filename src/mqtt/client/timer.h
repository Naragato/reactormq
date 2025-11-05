#pragma once

#include <chrono>
#include <cstdint>
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
     */
    struct Timer
    {
        /**
         * @brief Unique identifier for this timer.
         */
        std::uint32_t id = 0;

        /**
         * @brief Time point when the timer should fire.
         */
        TimePoint fireTime;

        /**
         * @brief Interval for periodic timers (0 for one-shot).
         */
        Duration interval{0};

        /**
         * @brief Callback to invoke when timer fires.
         */
        TimerCallback callback;

        /**
         * @brief Whether this timer is periodic (reschedules after firing).
         */
        bool isPeriodic = false;

        /**
         * @brief Whether this timer is active.
         */
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

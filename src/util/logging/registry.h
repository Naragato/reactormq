//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "util/logging/log_level.h"
#include "util/logging/sink.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <vector>

#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK

#include "CoreMinimal.h"
#include "LogReactorMQ.h"

#endif // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK

namespace reactormq::logging
{
    // Forward declarations for template methods
    namespace detail
    {
        template<class... Args>
        class LazyFormat;
    }

    /**
     * @brief Global registry that dispatches log messages to sinks.
     * Holds the active sinks and the global log level. Thread-safe singleton
     * used by the logging macros.
     */
    class Registry
    {
    public:
        /**
         * @brief Access the singleton instance.
         * @return Reference to the global registry.
         */
        static Registry& instance();

        /**
         * @brief Add a sink to receive future log messages that pass the level filter.
         * @param sink Sink to add.
         */
        void addSink(std::shared_ptr<Sink> sink);

        /**
         * @brief Remove all registered sinks.
         */
        void removeAllSinks();

        /**
         * @brief Set the global minimum log level.
         * @param level New minimum log level.
         */
        void setLevel(LogLevel level);

        /**
         * @brief Get the current global minimum log level.
         * @return Currently configured level.
         */
        LogLevel level() const;

        /**
         * @brief Check whether messages at a level are enabled.
         * @param level Level to test.
         * @return True if enabled.
         */
        bool shouldLog(LogLevel level) const;

        /**
         * @brief Dispatch a formatted log message to all sinks.
         * @param level Severity level.
         * @param location location of the call
         * @param text Formatted message text
         */
        void log(LogLevel level, const std::source_location& location, std::string_view text) const;

        /**
         * @brief Dispatch a lazy-formatted log message to all sinks.
         * Formats into SmallLogString (inline buffer first, heap fallback), then dispatches.
         * @tparam Args Argument types captured by LazyFormat
         * @param level Severity level.
         * @param location location of the call
         * @param lazyMsg Lazy formatter that defers formatting until after level check
         */
        template<class... Args>
        void log(LogLevel level, const std::source_location& location, const detail::LazyFormat<Args...>& lazyMsg) const;

    private:
        /** @brief Private constructor for singleton pattern. */
        Registry() = default;

        /**
         * @brief Dispatch a formatted log message to all sinks.
         * @param level Severity level.
         * @param location location of the call
         * @param text Formatted message text
         */
        [[nodiscard]] static LogMessage makeMessage(LogLevel level, const std::source_location& location, std::string_view text);

        /**
         * @brief sends the formatted message to the registered sinks
         * @param msg Formatted message text
         */
        void logToSinks(const LogMessage& msg) const;

        mutable std::mutex mtx_;
        std::vector<std::shared_ptr<Sink>> sinks_;
        std::atomic<LogLevel> level_{ LogLevel::Trace };
    };
} // namespace reactormq::logging
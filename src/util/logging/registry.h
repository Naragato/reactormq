#pragma once
#include "util/logging/log_level.h"
#include "util/logging/sink.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <vector>

namespace reactormq::logging
{
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
         * @param text Formatted message text (moved).
         */
        void log(
            LogLevel level, const std::source_location& location,
            std::string text) const;

    private:
        /** @brief Private constructor for singleton pattern. */
        Registry() = default;

        mutable std::mutex mtx_;
        std::vector<std::shared_ptr<Sink>> sinks_;
        std::atomic<LogLevel> level_{ LogLevel::Trace };
    };
} // namespace reactormq::logging
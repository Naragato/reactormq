#pragma once

#include "logging.h"
#include <mutex>
#include <string>

namespace reactormq
{
    namespace logging
    {
        /**
         * @brief Sink that writes log messages to the console (stdout/stderr).
         * Provides optional ANSI color output on supported terminals.
         */
        class ConsoleSink : public Sink
        {
        public:
            /**
             * @brief Construct a ConsoleSink.
             * @param enable_colors Whether to enable ANSI color output when supported.
             */
            ConsoleSink(bool enable_colors = true);

            /**
             * @brief Write a log message to the console.
             * @param msg The log message to be written.
             */
            void log(const LogMessage &msg) override;

            /**
             * @brief Format a time point as ISO-8601 with millisecond precision.
             * @param tp The time point to format.
             * @return Formatted timestamp string.
             */
            static std::string to_iso8601_ms(const std::chrono::system_clock::time_point &tp);

            /**
             * @brief Get a human-readable thread identifier string.
             * @param id The thread id.
             * @return String representation of the thread id.
             */
            static std::string thread_id_str(const std::thread::id &id);

        private:
            bool colors_ = true;
            std::mutex mtx_;

            /**
             * @brief Format the non-message prefix (timestamp, level, thread) for a log line.
             * @param msg The message to format prefix for.
             * @return Formatted prefix string.
             */
            static std::string format_prefix(const LogMessage &msg);

            /**
             * @brief Map a log level to its ANSI color escape sequence (or empty if disabled).
             * @param lvl The log level.
             * @return Null-terminated color code string.
             */
            static const char *color_for(LogLevel lvl);

            /**
             * @brief Choose the output stream for a given level.
             * @param lvl The log level.
             * @return "out" or "err".
             */
            static const char *stream_for(LogLevel lvl);
            /**
             * @brief Ensure Windows VT processing is enabled once for colored output.
             * No-op on non-Windows platforms.
             */
            void ensure_vt_enabled_once();

            bool vt_enabled_checked_ = false;
        };
    } // namespace logging
} // namespace reactormq
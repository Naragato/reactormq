//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#if REACTORMQ_WITH_CONSOLE_SINK

#include "logging.h"
#include "util/logging/sink.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

namespace reactormq::logging
{
    /**
     * @brief Sink that writes log messages to the console (stdout/stderr).
     * Provides optional ANSI color output on supported terminals.
     */
    class ConsoleSink final : public Sink
    {
    public:
        /**
         * @brief Construct a ConsoleSink.
         *
         * @param enable_colors Whether to enable ANSI color output when supported.
         */
        explicit ConsoleSink(bool enable_colors = true);

        /**
         * @brief Destructor.
         *
         * Flushes any pending duplicate messages.
         */
        ~ConsoleSink() override;

        /**
         * @brief Write a log message to the console.
         *
         * The message is formatted, optionally colorized depending on the log
         * level, and written to either stdout or stderr.
         *
         * @param msg The log message to be written.
         */
        void log(const LogMessage& msg) override;

    private:
        /**
         * @brief Whether ANSI color output is enabled for this sink.
         *
         * When @c true and the terminal supports ANSI escape codes, log
         * messages are colorized according to their log level.
         */
        bool colors_ = true;

        /**
         * @brief Mutex protecting concurrent access to console output.
         *
         * All write operations acquire this mutex to prevent interleaving of
         * log lines from multiple threads.
         */
        std::mutex mtx_;

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        /**
         * @brief Indicates whether VT processing initialization has already been attempted.
         *
         * Used to ensure @ref ensureVtEnabledOnce is performed at most once.
         */
        bool vt_enabled_checked_ = false;
#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY
        /**
         * @brief Information about the last message received for duplicate detection.
         */
        struct LastMessageInfo
        {
            std::string text;
            std::string function;
            std::string file;
            std::uint_least32_t line;
            LogLevel level;
            std::jthread::id threadId;
        };

        /**
         * @brief Last message identity for duplicate detection.
         */
        std::optional<LastMessageInfo> m_lastMessage;

        /**
         * @brief Count of consecutive duplicate messages.
         */
        size_t m_duplicateCount = 0;

        /**
         * @brief Timestamp of the first occurrence in the duplicate sequence.
         */
        std::chrono::system_clock::time_point m_firstTimestamp;

        /**
         * @brief Timestamp of the last occurrence in the duplicate sequence.
         */
        std::chrono::system_clock::time_point m_lastTimestamp;

        /**
         * @brief Format the non-message prefix for a log line.
         *
         * The prefix typically contains timestamp, log level, thread id and
         * source location information.
         *
         * @param msg The message to format prefix for.
         * @return Formatted prefix string.
         */
        static std::string formatPrefix(const LogMessage& msg);

        /**
         * @brief Map a log level to its ANSI color escape sequence.
         *
         * If colors are disabled or the terminal is not capable of color, this
         * may return an empty string.
         *
         * @param lvl The log level.
         * @return Null-terminated color code string (may be empty).
         */
        static const char* colorFor(LogLevel lvl);

        /**
         * @brief Choose the output stream for a given level.
         *
         * Typically informational messages go to stdout, while warnings and
         * errors go to stderr.
         *
         * @param lvl The log level.
         * @return "out" if the message should go to stdout, "err" for stderr.
         */
        static const char* streamFor(LogLevel lvl);

        /**
         * @brief Ensure Windows VT processing is enabled once for colored output.
         *
         * On Windows consoles, VT (virtual terminal) processing is required for
         * ANSI escape sequences to work. This function performs a one-time
         * initialization on supported platforms and is a no-op elsewhere.
         */
        void ensureVtEnabledOnce();

        /**
         * @brief Flush any pending duplicate message to output.
         */
        void flushPendingDuplicate();

        /**
         * @brief Write a single log message to output.
         *
         * @param msg The message to write.
         */
        void writeMessage(const LogMessage& msg) const;

        /**
         * @brief Write a duplicate message summary to output.
         *
         * @param msg The message to write.
         * @param count Number of times the message was repeated.
         * @param firstTime Timestamp of the first occurrence.
         * @param lastTime Timestamp of the last occurrence.
         */
        void writeDuplicateSummary(
            const LogMessage& msg,
            size_t count,
            const std::chrono::system_clock::time_point& firstTime,
            const std::chrono::system_clock::time_point& lastTime) const;

        /**
         * @brief Check if two messages are duplicates.
         *
         * @param msg The message to check.
         * @param last The last message info.
         * @return True if messages are duplicates.
         */
        static bool isDuplicate(const LogMessage& msg, const LastMessageInfo& last);
    };
} // namespace reactormq::logging
#endif // REACTORMQ_WITH_CONSOLE_SINK
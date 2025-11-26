//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#if REACTORMQ_WITH_FILE_SINK

#include "logging.h"
#include "util/logging/sink.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>

namespace reactormq::logging
{
    /**
     * @brief Sink that writes log messages to a file.
     * The file is opened in append mode and all messages are written in a
     * thread-safe manner.
     */
    class FileSink final : public Sink
    {
    public:
        /**
         * @brief Construct a FileSink that writes to the given file.
         *
         * If the file does not exist, it will be created. If it exists, new
         * log entries are appended to the end.
         *
         * @param path Path to the log file. Defaults to @c "reactormq.log".
         */
        explicit FileSink(const std::string& path = "reactormq.log");

        /**
         * @brief Destructor.
         *
         * Flushes and closes the underlying file stream.
         */
        ~FileSink() override;

        /**
         * @brief Write a log message to the file.
         *
         * @param msg The log message to be written.
         */
        void log(const LogMessage& msg) override;

    private:
        /**
         * @brief Output file stream used to write log data.
         */
        std::ofstream out_;

        /**
         * @brief Mutex protecting concurrent file write operations.
         */
        std::mutex mtx_;

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
         * @brief Flush any pending duplicate message to output.
         */
        void flushPendingDuplicate();

        /**
         * @brief Write a single log message to output.
         *
         * @param msg The message to write.
         */
        void writeMessage(const LogMessage& msg);

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
            const std::chrono::system_clock::time_point& lastTime);

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
#endif // REACTORMQ_WITH_FILE_SINK
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "file_sink.h"
#if REACTORMQ_WITH_FILE_SINK

#include "log_message.h"
#include "logging.h"
#include "util/logging/console_sink.h"
#include "util/logging/log_message.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>

namespace reactormq::logging
{
    FileSink::FileSink(const std::string& path)
    {
        out_.open(path, std::ios::out | std::ios::app);
    }

    FileSink::~FileSink()
    {
        std::scoped_lock lock(mtx_);
#if REACTORMQ_WITH_EXCEPTIONS
        try
#endif

        {
            flushPendingDuplicate();
            if (out_.is_open())
            {
                out_.flush();
                out_.close();
            }
        }
#if REACTORMQ_WITH_EXCEPTIONS
        catch (...)
#endif
        {
            // intentionally ignored
        }
    }

    void FileSink::log(const LogMessage& msg)
    {
        std::scoped_lock lock(mtx_);
        if (!out_.is_open())
        {
            return;
        }

        if (m_lastMessage.has_value() && isDuplicate(msg, *m_lastMessage))
        {
            ++m_duplicateCount;
            m_lastTimestamp = msg.timestamp;
        }
        else
        {
            flushPendingDuplicate();

            m_lastMessage = LastMessageInfo{ msg.text, msg.function, msg.file, msg.line, msg.level, msg.thread_id };
            m_duplicateCount = 1;
            m_firstTimestamp = msg.timestamp;
            m_lastTimestamp = msg.timestamp;
        }
    }

    void FileSink::flushPendingDuplicate()
    {
        if (!m_lastMessage.has_value() || m_duplicateCount == 0)
        {
            return;
        }

        LogMessage msg;
        msg.text = m_lastMessage->text;
        msg.function = m_lastMessage->function;
        msg.file = m_lastMessage->file;
        msg.line = m_lastMessage->line;
        msg.level = m_lastMessage->level;
        msg.thread_id = m_lastMessage->threadId;
        msg.timestamp = m_firstTimestamp;

        if (m_duplicateCount == 1)
        {
            writeMessage(msg);
        }
        else
        {
            writeDuplicateSummary(msg, m_duplicateCount, m_firstTimestamp, m_lastTimestamp);
        }

        m_lastMessage.reset();
        m_duplicateCount = 0;
    }

    void FileSink::writeMessage(const LogMessage& msg)
    {
        const auto ts = toIso8601ms(msg.timestamp);
        const auto tid = threadIdStr(msg.thread_id);

        out_ << ts << " | " << tid << " | " << levelToString(msg.level) << " | " << msg.function << " | " << msg.file << ':' << msg.line
             << " | " << msg.text << '\n';
        out_.flush();
    }

    void FileSink::writeDuplicateSummary(
        const LogMessage& msg,
        const size_t count,
        const std::chrono::system_clock::time_point& firstTime,
        const std::chrono::system_clock::time_point& lastTime)
    {
        const auto firstTs = toIso8601ms(firstTime);
        const auto lastTs = toIso8601ms(lastTime);
        const auto tid = threadIdStr(msg.thread_id);

        out_ << firstTs << " to " << lastTs << " | " << tid << " | " << levelToString(msg.level) << " | " << msg.function << " | "
             << msg.file << ':' << msg.line << " | " << msg.text << " (repeated " << count << " times)\n";
        out_.flush();
    }

    bool FileSink::isDuplicate(const LogMessage& msg, const LastMessageInfo& last)
    {
        return msg.text == last.text && msg.function == last.function && msg.file == last.file && msg.line == last.line;
    }
} // namespace reactormq::logging
#endif // REACTORMQ_WITH_FILE_SINK
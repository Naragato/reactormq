//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "console_sink.h"
#if REACTORMQ_WITH_CONSOLE_SINK
#include "util/logging/log_message.h"

#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

namespace reactormq::logging
{
    static std::string basename(const std::string& path)
    {
        const size_t pos = path.find_last_of("/\\");
        return pos == std::string::npos ? path : path.substr(pos + 1);
    }

    ConsoleSink::ConsoleSink(const bool enable_colors)
        : colors_(enable_colors)
    {
    }

    ConsoleSink::~ConsoleSink()
    {
#if REACTORMQ_WITH_EXCEPTIONS
        try
#endif

        {
            std::scoped_lock lock(mtx_);
            flushPendingDuplicate();
        }
#if REACTORMQ_WITH_EXCEPTIONS
        catch (...)
#endif
        {
            // intentionally ignored
        }
    }

    const char* ConsoleSink::colorFor(const LogLevel lvl)
    {
        switch (lvl)
        {
            using enum LogLevel;
        case Trace:
        case Debug:
            return "\x1b[2m";
        case Info:
            return "\x1b[32m";
        case Warn:
            return "\x1b[33m";
        case Error:
            return "\x1b[31m";
        case Critical:
            return "\x1b[35m";
        case Off:
            return "";
        }
        return "";
    }

    const char* ConsoleSink::streamFor(const LogLevel lvl)
    {
        return lvl >= LogLevel::Warn ? "err" : "out";
    }

    std::string ConsoleSink::formatPrefix(const LogMessage& msg)
    {
        std::ostringstream oss;

        oss << toIso8601ms(msg.timestamp) << " | " << threadIdStr(msg.thread_id) << " | " << levelToString(msg.level) << " | "
            << msg.function << " | " << basename(msg.file) << ':' << msg.line << " | ";
        return oss.str();
    }

    void ConsoleSink::ensureVtEnabledOnce()
    {
#ifdef _WIN32
        if (vt_enabled_checked_)
        {
            return;
        }
        vt_enabled_checked_ = true;
        const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
        {
            return;
        }
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode) == 0)
        {
            return;
        }
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        if (const HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE); hErr != INVALID_HANDLE_VALUE && GetConsoleMode(hErr, &dwMode) != 0)
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hErr, dwMode);
        }
#endif // _WIN32
    }

    void ConsoleSink::log(const LogMessage& msg)
    {
        std::scoped_lock lock(mtx_);
        ensureVtEnabledOnce();

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

    void ConsoleSink::flushPendingDuplicate()
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

    void ConsoleSink::writeMessage(const LogMessage& msg) const
    {
        const auto prefix = formatPrefix(msg);
        const bool use_err = streamFor(msg.level) == std::string("err");

        if (colors_)
        {
            const char* color = colorFor(msg.level);
            const auto reset = "\x1b[0m";
            if (use_err)
            {
                std::cerr << color << prefix << msg.text << reset << '\n';
            }
            else
            {
                std::cout << color << prefix << msg.text << reset << '\n';
            }
        }
        else
        {
            if (use_err)
            {
                std::cerr << prefix << msg.text << '\n';
            }
            else
            {
                std::cout << prefix << msg.text << '\n';
            }
        }
    }

    void ConsoleSink::writeDuplicateSummary(
        const LogMessage& msg,
        const size_t count,
        const std::chrono::system_clock::time_point& firstTime,
        const std::chrono::system_clock::time_point& lastTime) const
    {
        const auto firstTs = toIso8601ms(firstTime);
        const auto lastTs = toIso8601ms(lastTime);
        const auto tid = threadIdStr(msg.thread_id);
        const bool use_err = streamFor(msg.level) == std::string("err");

        std::ostringstream oss;
        oss << firstTs << " to " << lastTs << " | " << tid << " | " << levelToString(msg.level) << " | " << msg.function << " | "
            << basename(msg.file) << ':' << msg.line << " | " << msg.text << " (repeated " << count << " times)";

        const std::string output = oss.str();

        if (colors_)
        {
            const char* color = colorFor(msg.level);
            const auto reset = "\x1b[0m";
            if (use_err)
            {
                std::cerr << color << output << reset << '\n';
            }
            else
            {
                std::cout << color << output << reset << '\n';
            }
        }
        else
        {
            if (use_err)
            {
                std::cerr << output << '\n';
            }
            else
            {
                std::cout << output << '\n';
            }
        }
    }

    bool ConsoleSink::isDuplicate(const LogMessage& msg, const LastMessageInfo& last)
    {
        return msg.text == last.text && msg.function == last.function && msg.file == last.file && msg.line == last.line;
    }
} // namespace reactormq::logging
#endif // REACTORMQ_WITH_CONSOLE_SINK
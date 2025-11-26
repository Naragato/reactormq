//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "registry.h"

#include "console_sink.h"
#include "file_sink.h"
#include "ue_log_sink.h"
#include "util/logging/log_message.h"

#include <format>
#include <mutex>

namespace reactormq::logging
{
    Registry& Registry::instance()
    {
        static Registry inst;
        static std::once_flag initFlag;
        std::call_once(
            initFlag,
            []()
            {
                using namespace std::chrono;
                const auto now = floor<seconds>(system_clock::now());
#if REACTORMQ_WITH_FILE_SINK
                inst.addSink(std::make_shared<FileSink>(std::format("reactormq-{}.log", toIso8601ms(now))));
#endif // REACTORMQ_WITH_FILE_SINK
#if REACTORMQ_WITH_CONSOLE_SINK
                inst.addSink(std::make_shared<ConsoleSink>());
#endif // REACTORMQ_WITH_CONSOLE_SINK
#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
                inst.addSink(std::make_shared<UeLogSink>());
#endif // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
            });

        return inst;
    }

    void Registry::addSink(std::shared_ptr<Sink> sink)
    {
        std::scoped_lock lock(mtx_);
        sinks_.push_back(std::move(sink));
    }

    void Registry::removeAllSinks()
    {
        std::scoped_lock lock(mtx_);
        sinks_.clear();
    }

    void Registry::setLevel(const LogLevel level)
    {
        level_.store(level, std::memory_order_relaxed);
    }

    LogLevel Registry::level() const
    {
        return level_.load(std::memory_order_relaxed);
    }

    bool Registry::shouldLog(LogLevel level) const
    {
#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
        return ShouldLogToUeCategory(level);
#else // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
        return static_cast<int>(level) >= static_cast<int>(level_.load(std::memory_order_relaxed)) && level != LogLevel::Off;
#endif // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
    }

    void Registry::log(const LogLevel level, const std::source_location& location, const std::string_view text) const
    {
        LogMessage msg;
        msg.timestamp = std::chrono::system_clock::now();
        msg.thread_id = std::this_thread::get_id();
        msg.level = level;
        msg.function = location.function_name();
        msg.file = location.file_name();
        msg.line = location.line();
        msg.text = text;

        std::vector<std::shared_ptr<Sink>> sinks_copy;
        {
            std::scoped_lock lock(mtx_);
            sinks_copy = sinks_; // copy to avoid holding a lock during I/O
        }
        for (auto const& s : sinks_copy)
        {
            if (s)
            {
                s->log(msg);
            }
        }
    }

    LogMessage Registry::makeMessage(const LogLevel level, const std::source_location& location, const std::string_view text)
    {
        LogMessage msg{};
        msg.timestamp = std::chrono::system_clock::now();
        msg.thread_id = std::this_thread::get_id();
        msg.level = level;
        msg.function = location.function_name();
        msg.file = location.file_name();
        msg.line = location.line();
        msg.text = text;
        return msg;
    }

    void Registry::logToSinks(const LogMessage& msg) const
    {
        std::vector<std::shared_ptr<Sink>> sinks_copy;
        {
            std::scoped_lock lock(mtx_);
            sinks_copy = sinks_; // copy to avoid holding a lock during I/O
        }

        for (auto const& s : sinks_copy)
        {
            if (s)
            {
                s->log(msg);
            }
        }
    }
} // namespace reactormq::logging
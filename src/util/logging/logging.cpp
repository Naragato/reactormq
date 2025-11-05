#include "util/logging/logging.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

namespace reactormq::logging
{
    

    std::string toHex(const void* data, const size_t length)
    {
        const auto* bytes = static_cast<const unsigned char*>(data);
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < length; ++i)
        {
            oss << std::setw(2) << static_cast<unsigned int>(bytes[i]) << ' ';
        }
        return oss.str();
    }

    const char* level_to_string(const LogLevel lvl)
    {
        switch (lvl)
        {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Critical:
            return "CRITICAL";
        case LogLevel::Off:
            return "OFF";
        }
        return "?";
    }

    Registry& Registry::instance()
    {
        static Registry inst;
        return inst;
    }

    void Registry::add_sink(std::shared_ptr<Sink> sink)
    {
        std::scoped_lock lock(mtx_);
        sinks_.push_back(std::move(sink));
    }

    void Registry::remove_all_sinks()
    {
        std::scoped_lock lock(mtx_);
        sinks_.clear();
    }

    void Registry::set_level(const LogLevel level)
    {
        level_.store(level, std::memory_order_relaxed);
    }

    LogLevel Registry::level() const
    {
        return level_.load(std::memory_order_relaxed);
    }

    bool Registry::should_log(LogLevel level) const
    {
        return static_cast<int>(level) >= static_cast<int>(level_.load(std::memory_order_relaxed))
            && level != LogLevel::Off;
    }

    void Registry::log(
        const LogLevel level,
        const char* function,
        const char* file,
        const int32_t line,
        std::string text) const
    {
        LogMessage msg;
        msg.timestamp = std::chrono::system_clock::now();
        msg.thread_id = std::this_thread::get_id();
        msg.level = level;
        msg.function = function ? function : "";
        msg.file = file ? file : "";
        msg.line = line;
        msg.text = std::move(text);

        std::vector<std::shared_ptr<Sink>> sinks_copy;
        {
            std::scoped_lock lock(mtx_);
            sinks_copy = sinks_; // copy to avoid holding lock during I/O
        }
        for (auto& s : sinks_copy)
        {
            if (s)
                s->log(msg);
        }
    }

    std::string vformat_snprintf(const char* fmt, va_list args)
    {
        if (!fmt)
            return {};

        va_list args_copy;
        va_copy(args_copy, args);
#ifdef _MSC_VER
        int32_t size = _vscprintf(fmt, args_copy);
        va_end(args_copy);
        if (size < 0)
            return {};
        std::string result;
        result.resize(static_cast<size_t>(size));
        (void) vsnprintf(result.data(), result.size() + 1, fmt, args);
        return result;
#else
        char small[256];
        const int n = vsnprintf(small, sizeof(small), fmt, args_copy);
        va_end(args_copy);
        if (n >= 0 && static_cast<size_t>(n) < sizeof(small))
        {
            return { small, static_cast<size_t>(n) };
        }
        const size_t size_needed = n > 0 ? static_cast<size_t>(n) + 1 : 1024;
        std::string buf;
        buf.resize(size_needed);
        const int32_t n2 = vsnprintf(buf.data(), buf.size(), fmt, args);
        if (n2 < 0)
            return {};
        return { buf.data(), static_cast<size_t>(n2) };
#endif // _MSC_VER
    }

    std::string format_snprintf(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string s = vformat_snprintf(fmt, args);
        va_end(args);
        return s;
    }
} // namespace reactormq::logging
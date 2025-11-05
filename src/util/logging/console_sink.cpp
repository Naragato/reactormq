#include "console_sink.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
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

    std::string ConsoleSink::to_iso8601_ms(const std::chrono::system_clock::time_point& tp)
    {
        using namespace std::chrono;
        auto t_c = system_clock::to_time_t(tp);
        std::tm tm_buf{};
#ifdef _WIN32
        (void) localtime_s(&tm_buf, &t_c);
#else
        (void) localtime_r(&t_c, &tm_buf);
#endif // _WIN32
        const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    std::string ConsoleSink::thread_id_str(const std::thread::id& id)
    {
        std::ostringstream oss;
        oss << id;
        return oss.str();
    }

    const char* ConsoleSink::color_for(const LogLevel lvl)
    {
        switch (lvl)
        {
        case LogLevel::Trace:
        case LogLevel::Debug:
            return "\x1b[2m";
        case LogLevel::Info:
            return "\x1b[32m";
        case LogLevel::Warn:
            return "\x1b[33m";
        case LogLevel::Error:
            return "\x1b[31m";
        case LogLevel::Critical:
            return "\x1b[35m";
        case LogLevel::Off:
            return "";
        }
        return "";
    }

    const char* ConsoleSink::stream_for(const LogLevel lvl)
    {
        return lvl >= LogLevel::Warn ? "err" : "out";
    }

    std::string ConsoleSink::format_prefix(const LogMessage& msg)
    {
        std::ostringstream oss;
        oss << to_iso8601_ms(msg.timestamp)
            << " | " << thread_id_str(msg.thread_id)
            << " | " << level_to_string(msg.level)
            << " | " << msg.function
            << " | " << basename(msg.file) << ':' << msg.line
            << " | ";
        return oss.str();
    }

    void ConsoleSink::ensure_vt_enabled_once()
    {
#ifdef _WIN32
        if (vt_enabled_checked_)
            return;
        vt_enabled_checked_ = true;
        const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
            return;
        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode))
            return;
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        if (const HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE); hErr != INVALID_HANDLE_VALUE && GetConsoleMode(hErr, &dwMode))
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hErr, dwMode);
        }
#endif // _WIN32
    }

    void ConsoleSink::log(const LogMessage& msg)
    {
        std::scoped_lock lock(mtx_);
        ensure_vt_enabled_once();

        const auto prefix = format_prefix(msg);
        const bool use_err = (stream_for(msg.level) == std::string("err"));

        if (colors_)
        {
            const char* color = color_for(msg.level);
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
} // namespace reactormq::logging
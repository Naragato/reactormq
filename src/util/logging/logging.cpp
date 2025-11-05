#include "util/logging/logging.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

namespace reactormq::logging
{
    std::string toIso8601ms(const std::chrono::system_clock::time_point& tp)
    {
        using namespace std::chrono;
        auto t_c = system_clock::to_time_t(tp);
        std::tm tm_buf{};
#ifdef _WIN32
        (void)localtime_s(&tm_buf, &t_c);
#elif REACTORMQ_PLATFORM_PROSPERO
        (void)localtime_s(&t_c, &tm_buf);
#else
        (void)localtime_r(&t_c, &tm_buf);
#endif // _WIN32
        const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    std::string threadIdStr(const std::thread::id& id)
    {
        std::ostringstream oss;
        oss << id;
        return oss.str();
    }

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

    const char* levelToString(const LogLevel lvl)
    {
        switch (lvl)
        {
            using enum LogLevel;
        case Trace:
            return "TRACE";
        case Debug:
            return "DEBUG";
        case Info:
            return "INFO";
        case Warn:
            return "WARN";
        case Error:
            return "ERROR";
        case Critical:
            return "CRITICAL";
        case Off:
            return "OFF";
        }
        return "?";
    }

    std::string vFormatSnprintf(const char* fmt, va_list args)
    {
        if (nullptr == fmt)
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
        (void)vsnprintf(result.data(), result.size() + 1, fmt, args);
        return result;
#else
        char small[256]; const int n = vsnprintf(small, sizeof(small), fmt, args_copy); va_end(args_copy); if (n >= 0 && static_cast<size_t>
            (n) < sizeof(small))
        {
            return { small, static_cast<size_t>(n) };
        } const size_t size_needed = n > 0 ? static_cast<size_t>(n) + 1 : 1024; std::string buf; buf.resize(size_needed); const int32_t n2 =
            vsnprintf(buf.data(), buf.size(), fmt, args); if (n2 < 0)
            return {}; return { buf.data(), static_cast<size_t>(n2) };
#endif // _MSC_VER
    }

    std::string FormatSnprintf(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string s = vFormatSnprintf(fmt, args);
        va_end(args);
        return s;
    }
} // namespace reactormq::logging
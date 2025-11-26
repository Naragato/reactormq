//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "util/logging/logging.h"

#include <fstream>
#include <iomanip>
#include <sstream>

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

    std::string toHex(const std::span<const std::uint8_t> data)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (const std::uint8_t b : data)
        {
            oss << std::setw(2) << static_cast<unsigned>(b) << ' ';
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
} // namespace reactormq::logging
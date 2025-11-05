#pragma once

#include "log_level.h"
#include "util/logging/registry.h"

#include <chrono>
#include <cstdarg>
#include <string>
#include <thread>

namespace reactormq::logging
{
    /**
     * @brief Format a time point as ISO-8601 with millisecond precision.
     * @param tp Time point to format.
     * @return Formatted timestamp string.
     */
    std::string toIso8601ms(const std::chrono::system_clock::time_point& tp);

    /**
     * @brief Get a human-readable thread identifier string.
     * @param id Thread id.
     * @return String form of the thread id.
     */
    std::string threadIdStr(const std::thread::id& id);

    /**
     * @brief Convert raw bytes to a hexadecimal string.
     * @param data Pointer to input buffer.
     * @param length Number of bytes to convert.
     * @return Hex representation of the buffer.
     */
    std::string toHex(const void* data, size_t length);

    /**
     * @brief Convert a log level to its textual representation.
     * @param lvl Log level to convert.
     * @return C-string such as "info" or "error".
     */
    const char* levelToString(LogLevel lvl);

    /**
     * @brief Format a printf-style string using a va_list.
     * @param fmt Format string.
     * @param args Arguments list.
     * @return Formatted string.
     */
    std::string vFormatSnprintf(const char* fmt, va_list args);

    /**
     * @brief Format a printf-style string using variadic arguments.
     * @param fmt Format string.
     * @param ... Arguments corresponding to format specifiers in fmt.
     * @return Formatted string.
     */
    std::string FormatSnprintf(const char* fmt, ...);

    /**
     * @def REACTORMQ_LOG
     * @brief Log a formatted message if enabled at the given level.
     * @param level LogLevel to log at.
     * @param fmt printf-style format string.
     * @param ... Arguments matching the format string.
     */
#define REACTORMQ_LOG(level, fmt, ...) \
    do { \
        using namespace reactormq::logging; \
        if (Registry::instance().shouldLog(level)) { \
            std::string _msg = reactormq::logging::FormatSnprintf(fmt, ##__VA_ARGS__); \
            Registry::instance().log(level, std::source_location::current(), std::move(_msg)); \
        } \
    } while (0)

    /**
     * @def REACTORMQ_LOG_HEX
     * @brief Log a formatted message plus a hex dump of binary data.
     * @param level LogLevel to log at.
     * @param data Pointer to the binary buffer.
     * @param length Number of bytes in data.
     * @param fmt printf-style format string.
     * @param ... Arguments matching the format string.
     */
#define REACTORMQ_LOG_HEX(level, data, length, fmt, ...) \
    do { \
        using namespace reactormq::logging; \
        if (Registry::instance().shouldLog(level)) { \
            std::string _msg = reactormq::logging::FormatSnprintf(fmt, ##__VA_ARGS__); \
            _msg += " | "; \
            _msg += reactormq::logging::toHex(data, length); \
            Registry::instance().log(level, std::source_location::current(), std::move(_msg)); \
        } \
    } while (0)
} // namespace reactormq::logging

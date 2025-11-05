#pragma once

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


namespace reactormq::logging
{
    enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical, Off };

    struct LogMessage
    {
        std::chrono::system_clock::time_point timestamp;
        std::thread::id thread_id;
        LogLevel level = LogLevel::Info;
        std::string function;
        std::string file;
        int32_t line = 0;
        std::string text;
    };

    std::string toHex(const void* data, size_t length);
    
    const char* level_to_string(LogLevel lvl);

    class Sink
    {
    public:
        virtual ~Sink() = default;

        virtual void log(const LogMessage& msg) = 0;
    };

    class Registry
    {
    public:
        static Registry& instance();

        void add_sink(std::shared_ptr<Sink> sink);

        void remove_all_sinks();

        void set_level(LogLevel level);

        LogLevel level() const;

        bool should_log(LogLevel level) const;

        void log(LogLevel level,
                 const char* function,
                 const char* file,
                 int32_t line,
                 std::string text) const;

    private:
        Registry() = default;

        mutable std::mutex mtx_;
        std::vector<std::shared_ptr<Sink> > sinks_;
        std::atomic<LogLevel> level_{LogLevel::Trace};
    };

    std::string vformat_snprintf(const char* fmt, va_list args);

    std::string format_snprintf(const char* fmt, ...);

#define REACTORMQ_LOG(level, fmt, ...) \
    do { \
        using namespace reactormq::logging; \
        if (Registry::instance().should_log(level)) { \
            std::string _msg = reactormq::logging::format_snprintf(fmt, ##__VA_ARGS__); \
            Registry::instance().log(level,  __func__, __FILE__, __LINE__, std::move(_msg)); \
        } \
    } while (0)

#define REACTORMQ_LOG_HEX(level, data, length, fmt, ...) \
    do { \
        using namespace reactormq::logging; \
        if (Registry::instance().should_log(level)) { \
            std::string _msg = reactormq::logging::format_snprintf(fmt, ##__VA_ARGS__); \
            _msg += " | "; \
            _msg += reactormq::logging::toHex(data, length); \
            Registry::instance().log(level, __func__, __FILE__, __LINE__, std::move(_msg)); \
        } \
    } while (0)
} // namespace reactormq::logging

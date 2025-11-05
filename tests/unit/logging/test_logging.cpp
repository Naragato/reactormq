#include "util/logging/console_sink.h"
#include "util/logging/file_sink.h"
#include "util/logging/logging.h"

#include <cstdio>
#include <gtest/gtest.h>

using namespace reactormq::logging;

namespace
{
    struct MemorySink final : public Sink
    {
        std::vector<LogMessage> messages;

        void log(const LogMessage& msg) override
        {
            messages.push_back(msg);
        }
    };
}

TEST(Logging, HexHelperSpacing)
{
    constexpr unsigned char data[4] = { 0x00, 0xAB, 0x7F, 0xFF };
    const std::string hex = toHex(data, 4);
    EXPECT_EQ(hex, std::string("00 ab 7f ff "));
}

TEST(Logging, RegistryLevelFilter)
{
    auto& reg = Registry::instance();
    reg.remove_all_sinks();
    const auto sink = std::make_shared<MemorySink>();
    reg.add_sink(sink);

    reg.set_level(LogLevel::Info);
    REACTORMQ_LOG(LogLevel::Debug, "Test", "debug %d", 1);
    REACTORMQ_LOG(LogLevel::Info, "ok");

    ASSERT_EQ(sink->messages.size(), 1u);
    EXPECT_EQ(sink->messages[0].level, LogLevel::Info);
}

TEST(Logging, MacroHexAppendsDump)
{
    auto& reg = Registry::instance();
    reg.remove_all_sinks();
    const auto sink = std::make_shared<MemorySink>();
    reg.add_sink(sink);
    reg.set_level(LogLevel::Trace);

    constexpr unsigned char data[3] = { 1, 2, 3 };
    REACTORMQ_LOG_HEX(LogLevel::Trace, data, 3, "payload %d", 42);

    ASSERT_EQ(sink->messages.size(), 1u);
    EXPECT_NE(sink->messages[0].text.find("payload 42"), std::string::npos);
    EXPECT_NE(sink->messages[0].text.find("01 02 03 "), std::string::npos);
}

TEST(Logging, FileSinkWrites)
{
    auto& reg = Registry::instance();
    reg.remove_all_sinks();
    const auto fs = std::make_shared<FileSink>("reactormq_test.log");
    reg.add_sink(fs);
    reg.set_level(LogLevel::Info);

    REACTORMQ_LOG(LogLevel::Info, "File", "hello %d", 5);

    std::FILE* f = std::fopen("reactormq_test.log", "rb");
    ASSERT_NE(f, nullptr);
    std::fseek(f, 0, SEEK_END);
    const long pos = std::ftell(f);
    ASSERT_GE(pos, 0);
    const auto sz = static_cast<size_t>(pos);
    std::fclose(f);
    EXPECT_GT(sz, 0u);
}
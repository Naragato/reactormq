//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "util/logging/console_sink.h"
#include "util/logging/file_sink.h"
#include "util/logging/log_message.h"
#include "util/logging/logging.h"

#include <array>
#include <cstdio>
#include <gtest/gtest.h>

using namespace reactormq::logging;

namespace
{
    struct MemorySink final : Sink
    {
        std::vector<LogMessage> messages;

        void log(const LogMessage& msg) override
        {
            messages.push_back(msg);
        }
    };
} // namespace

TEST(Logging, HexHelperSpacing)
{
    constexpr std::array<uint8_t, 4> data = { 0x00, 0xAB, 0x7F, 0xFF };
    const std::string hex = toHex(data);
    EXPECT_EQ(hex, std::string("00 ab 7f ff "));
}

TEST(Logging, RegistryLevelFilter)
{
    auto& reg = Registry::instance();
    reg.removeAllSinks();
    const auto sink = std::make_shared<MemorySink>();
    reg.addSink(sink);

    reg.setLevel(LogLevel::Info);
    REACTORMQ_LOG(LogLevel::Debug, "debug %d", 1);
    REACTORMQ_LOG(LogLevel::Info, "ok");

    ASSERT_EQ(sink->messages.size(), 1u);
    EXPECT_EQ(sink->messages[0].level, LogLevel::Info);
}

TEST(Logging, MacroHexAppendsDump)
{
    auto& reg = Registry::instance();
    reg.removeAllSinks();
    const auto sink = std::make_shared<MemorySink>();
    reg.addSink(sink);
    reg.setLevel(LogLevel::Trace);

    constexpr std::array<uint8_t, 3> data = { 1, 2, 3 };
    REACTORMQ_LOG_HEX(LogLevel::Trace, data.data(), 3, "payload %d", 42);

    ASSERT_EQ(sink->messages.size(), 1u);
    EXPECT_NE(sink->messages[0].text.find("payload 42"), std::string::npos);
    EXPECT_NE(sink->messages[0].text.find("01 02 03 "), std::string::npos);
}

TEST(Logging, FileSinkWrites)
{
    {
        std::remove("reactormq_test.log");
        auto& reg = Registry::instance();
        reg.removeAllSinks();
        const auto fs = std::make_shared<FileSink>("reactormq_test.log");
        reg.addSink(fs);
        reg.setLevel(LogLevel::Info);
        REACTORMQ_LOG(LogLevel::Info, "hello %d", 5);
        reg.removeAllSinks();
    }

    std::FILE* f = std::fopen("reactormq_test.log", "rb");
    ASSERT_NE(f, nullptr);
    std::fseek(f, 0, SEEK_END);
    const long pos = std::ftell(f);
    ASSERT_GE(pos, 0);
    const auto sz = static_cast<size_t>(pos);
    std::fclose(f);
    EXPECT_GT(sz, 0u);
    std::remove("reactormq_test.log");
}
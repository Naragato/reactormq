//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "log_level.h"

#include <chrono>
#include <string>
#include <thread>

namespace reactormq::logging
{
    /**
     * @brief Fully describes a single log event.
     * Instances of this struct are created and populated by the logging
     * infrastructure and passed to all registered sinks.
     */
    struct LogMessage
    {
        std::chrono::system_clock::time_point timestamp; ///< Time at which the log message was created.
        std::jthread::id thread_id; ///< Identifier of the thread that emitted the message.
        LogLevel level = LogLevel::Info; ///< Severity level of the message.
        std::string function; ///< Name of the function emitting the log call.
        std::string file; ///< Source file in which the log call appears.
        uint_least32_t line = 0; ///< Line number in the source file.
        std::string text; ///< Formatted log message text.
    };
} // namespace reactormq::logging
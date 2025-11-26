//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

namespace reactormq::logging
{
    /**
     * @brief Forward declaration of the log message structure.
     * Defined in @c log_message.h.
     */
    struct LogMessage;

    /**
     * @brief Abstract base class for log message sinks.
     *
     * Concrete implementations handle the actual delivery of log messages to
     * various destinations such as console, files, or external systems.
     *
     * Implementations must be thread-safe; @ref log is typically called from
     * multiple threads.
     */
    class Sink
    {
    public:
        virtual ~Sink() = default;

        /**
         * @brief Process a single log message.
         *
         * Implementations should perform any necessary formatting and output.
         *
         * @param msg Fully populated log message to be handled.
         */
        virtual void log(const LogMessage& msg) = 0;
    };
} // namespace reactormq::logging
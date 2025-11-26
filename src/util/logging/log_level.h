//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

namespace reactormq::logging
{
    /**
     * @brief Severity levels for log messages.
     * The global logging @ref Registry can be configured with a minimum level;
     * messages below that level will be suppressed.
     */
    enum class LogLevel
    {
        Trace, ///< Very fine-grained information for tracing program execution.
        Debug, ///< Detailed diagnostic information useful during development.
        Info, ///< General informational messages about normal operation.
        Warn, ///< Indications of potential problems or unusual situations.
        Error, ///< Recoverable errors that prevent an operation from succeeding.
        Critical, ///< Serious errors indicating a likely program or system failure.
        Off ///< Special level to disable logging entirely.
    };
} // namespace reactormq::logging
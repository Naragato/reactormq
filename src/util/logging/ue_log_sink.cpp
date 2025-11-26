//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "ue_log_sink.h"
#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK

#include "log_message.h"

namespace reactormq::logging
{
    ELogVerbosity::Type UeLogSink::ToVerbosity(const LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:
            return ELogVerbosity::VeryVerbose;
        case LogLevel::Debug:
            return ELogVerbosity::Verbose;
        case LogLevel::Info:
            return ELogVerbosity::Log;
        case LogLevel::Warn:
            return ELogVerbosity::Warning;
        case LogLevel::Error:
            return ELogVerbosity::Error;
        case LogLevel::Critical:
            return ELogVerbosity::Fatal;
        case LogLevel::Off:
        default:
            return ELogVerbosity::NoLogging;
        }
    }

    void UeLogSink::log(const LogMessage& msg)
    {
        if (!ShouldLogToUeCategory(msg.level))
        {
            return;
        }

        const FString File = UTF8_TO_TCHAR(msg.file.c_str());
        const FString Function = UTF8_TO_TCHAR(msg.function.c_str());
        const FString Text = UTF8_TO_TCHAR(msg.text.c_str());

        switch (ToVerbosity(msg.level))
        {
        case ELogVerbosity::Fatal:
            UE_LOG(LogReactorMQ, Fatal, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::Error:
            UE_LOG(LogReactorMQ, Error, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::Warning:
            UE_LOG(LogReactorMQ, Warning, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::Display:
            UE_LOG(LogReactorMQ, Display, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::Log:
            UE_LOG(LogReactorMQ, Log, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::Verbose:
            UE_LOG(LogReactorMQ, Verbose, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::VeryVerbose:
            UE_LOG(LogReactorMQ, VeryVerbose, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;

        case ELogVerbosity::SetColor:
            UE_LOG(LogReactorMQ, SetColor, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        case ELogVerbosity::BreakOnLog:
            UE_LOG(LogReactorMQ, BreakOnLog, TEXT("[%s:%u] [%s] %s"), *File, msg.line, *Function, *Text);
            break;
        default:
            UE_LOG(LogReactorMQ, Error, TEXT("[%u:unknown-mapping] [%s:%u] [%s] %s"), msg.level, *File, msg.line, *Function, *Text);
            break;
        }
    }
} // namespace reactormq::logging

#endif // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
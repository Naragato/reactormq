//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK

#include "logging.h"
#include "util/logging/sink.h"

#include "CoreMinimal.h"
#include "LogReactorMQ.h"

namespace reactormq::logging
{
    class UeLogSink final : public Sink
    {
    public:
        void log(const LogMessage& msg) override;

    private:
        static ELogVerbosity::Type ToVerbosity(LogLevel level);
    };
} // namespace reactormq::logging

#endif // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
#pragma once

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
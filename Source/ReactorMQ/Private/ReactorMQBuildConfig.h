#pragma once
#include "ReactorMQThreading.h"

#if REACTORMQ_THREAD == 0
constexpr EReactorMQThreadMode GReactorMQThreadMode = EReactorMQThreadMode::GameThread;
#elif REACTORMQ_THREAD == 1
// Background thread with callbacks marshalled back to the game thread
constexpr EReactorMQThreadMode GReactorMQThreadMode = EReactorMQThreadMode::BackgroundThreadMarshalled;
#elif REACTORMQ_THREAD == 2
// Background thread without additional callback marshalling
constexpr EReactorMQThreadMode GReactorMQThreadMode = EReactorMQThreadMode::BackgroundThread;
#else
constexpr EReactorMQThreadMode GReactorMQThreadMode = EReactorMQThreadMode::GameThread;
#endif
#pragma once
#include "HAL/Platform.h"

enum class EReactorMQThreadMode : uint8
{
    GameThread = 0,
    BackgroundThread = 1,
    BackgroundThreadMarshalled = 2
};
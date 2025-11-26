//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "HAL/Platform.h"

enum class EReactorMQThreadMode : uint8
{
    GameThread = 0,
    BackgroundThread = 1,
    BackgroundThreadMarshalled = 2
};
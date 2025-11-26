//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "IReactorMQClient.h"

class ITickableReactorMQClient : public IReactorMQClient
{
public:
    virtual void Tick() = 0;
};
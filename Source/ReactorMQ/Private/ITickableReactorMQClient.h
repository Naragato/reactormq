#pragma once

#include "IReactorMQClient.h"

class ITickableReactorMQClient : public IReactorMQClient
{
public:
    virtual void Tick() = 0;
};
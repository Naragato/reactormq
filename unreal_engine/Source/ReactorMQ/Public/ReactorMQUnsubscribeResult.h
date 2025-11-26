//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "ReactorMQTopicFilter.h"

struct REACTORMQ_API FReactorMQUnsubscribeResult
{
    FReactorMQTopicFilter Filter;
    bool bWasSuccessful = false;

    FReactorMQUnsubscribeResult() = default;

    FReactorMQUnsubscribeResult(FReactorMQTopicFilter InFilter, bool bInSuccess)
        : Filter(MoveTemp(InFilter))
          , bWasSuccessful(bInSuccess)
    {
    }
};
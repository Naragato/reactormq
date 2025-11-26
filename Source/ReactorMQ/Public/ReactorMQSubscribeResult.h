#pragma once
#include "CoreMinimal.h"
#include "ReactorMQTopicFilter.h"

struct REACTORMQ_API FReactorMQSubscribeResult
{
    FReactorMQTopicFilter Filter;
    bool bWasSuccessful = false;

    FReactorMQSubscribeResult() = default;

    FReactorMQSubscribeResult(FReactorMQTopicFilter InFilter, bool bInSuccess)
        : Filter(MoveTemp(InFilter))
          , bWasSuccessful(bInSuccess)
    {
    }
};
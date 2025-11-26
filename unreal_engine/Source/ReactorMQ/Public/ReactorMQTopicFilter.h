//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "ReactorMQTypes.h"

struct REACTORMQ_API FReactorMQTopicFilter
{
    FString Filter;
    EReactorMQQualityOfService QualityOfService = EReactorMQQualityOfService::AtMostOnce;
    bool bNoLocal = true;
    bool bRetainAsPublished = true;
    EReactorMQRetainHandling RetainHandling = EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTime;

    FReactorMQTopicFilter() = default;

    explicit FReactorMQTopicFilter(
        FString InFilter,
        EReactorMQQualityOfService InQoS = EReactorMQQualityOfService::AtMostOnce,
        bool bInNoLocal = true,
        bool bInRetainAsPublished = true,
        EReactorMQRetainHandling InRetainHandling = EReactorMQRetainHandling::SendRetainedMessagesAtSubscribeTime)
        : Filter(MoveTemp(InFilter))
          , QualityOfService(InQoS)
          , bNoLocal(bInNoLocal)
          , bRetainAsPublished(bInRetainAsPublished)
          , RetainHandling(InRetainHandling)
    {
    }

    bool operator==(const FReactorMQTopicFilter& Other) const
    {
        return Filter == Other.Filter
            && QualityOfService == Other.QualityOfService
            && bNoLocal == Other.bNoLocal
            && bRetainAsPublished == Other.bRetainAsPublished
            && RetainHandling == Other.RetainHandling;
    }

    bool operator!=(const FReactorMQTopicFilter& Other) const
    {
        return !(*this == Other);
    }
};
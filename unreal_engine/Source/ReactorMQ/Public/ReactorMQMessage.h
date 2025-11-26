//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "ReactorMQTypes.h"

struct REACTORMQ_API FReactorMQMessage
{
    FString Topic;
    TArray<uint8> Payload;
    EReactorMQQualityOfService QualityOfService = EReactorMQQualityOfService::AtMostOnce;
    bool bShouldRetain = false;
    FDateTime TimestampUtc;

    FReactorMQMessage() = default;

    FReactorMQMessage(FString InTopic, TArray<uint8> InPayload, EReactorMQQualityOfService InQoS, bool bInRetain)
        : Topic(MoveTemp(InTopic))
          , Payload(MoveTemp(InPayload))
          , QualityOfService(InQoS)
          , bShouldRetain(bInRetain)
          , TimestampUtc(FDateTime::UtcNow())
    {
    }
};
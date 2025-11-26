//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "ReactorMQUnsubscribeResult.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnReactorMQUnsubscribe, const TArray<FReactorMQUnsubscribeResult>&);
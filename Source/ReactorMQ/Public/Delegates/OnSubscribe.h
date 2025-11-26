#pragma once
#include "CoreMinimal.h"
#include "ReactorMQSubscribeResult.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnReactorMQSubscribe, const TArray<FReactorMQSubscribeResult>&);

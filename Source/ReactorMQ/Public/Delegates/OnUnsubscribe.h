#pragma once
#include "CoreMinimal.h"
#include "ReactorMQUnsubscribeResult.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnReactorMQUnsubscribe, const TArray<FReactorMQUnsubscribeResult>&);

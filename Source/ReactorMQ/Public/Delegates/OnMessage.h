#pragma once
#include "CoreMinimal.h"
#include "ReactorMQMessage.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnReactorMQMessage, const FReactorMQMessage&);

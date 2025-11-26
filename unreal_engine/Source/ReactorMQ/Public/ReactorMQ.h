//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FReactorMQClientPool;

class REACTORMQ_API FReactorMQModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FReactorMQModule& Get();

    FReactorMQClientPool& GetClientPool() const;

private:
    TSharedPtr<FReactorMQClientPool> ClientPool;
};
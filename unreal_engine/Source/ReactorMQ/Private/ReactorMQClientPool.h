//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "CoreMinimal.h"
#include "ReactorMQConnectionSettings.h"
#include "Containers/Ticker.h"
#include "HAL/Runnable.h"
#include "Misc/SingleThreadRunnable.h"

#include <atomic>

class FReactorMQClient;
class IReactorMQClient;

/**
 * @brief Pool of ReactorMQ clients with optional background ticking.
 */
class FReactorMQClientPool final
    : public FRunnable
      , public FSingleThreadRunnable
      , public TSharedFromThis<FReactorMQClientPool>
{
private:
    using FDeleter = TFunction<void(FReactorMQClient*)>;

    static TSharedPtr<FReactorMQClient> Create(const FReactorMQConnectionSettings& InConnectionSettings, FDeleter&& InDeleter);

public:
    FReactorMQClientPool();
    virtual ~FReactorMQClientPool() override;

    FReactorMQClientPool(const FReactorMQClientPool&) = delete;
    FReactorMQClientPool& operator=(const FReactorMQClientPool&) = delete;

    /**
     * @brief Get or create a client for the given connection settings.
     */
    TSharedPtr<IReactorMQClient, ESPMode::ThreadSafe> GetOrCreateClient(const FReactorMQConnectionSettings& InConnectionSettings);

    void Kill();

    // FRunnable
    virtual uint32 Run() override;
    virtual void Exit() override;
    virtual bool Init() override;
    virtual void Stop() override;
    virtual FSingleThreadRunnable* GetSingleThreadInterface() override;

    // FSingleThreadRunnable
    virtual void Tick() override;

private:
    void TickInternal();
    bool GameThreadTick(float DeltaTime);

    mutable FCriticalSection ClientMapLock;
    TMap<uint32, TWeakPtr<FReactorMQClient, ESPMode::ThreadSafe>> Clients;

    FRunnableThread* Thread = nullptr;
    std::atomic<bool> bIsRunning{ false };

    FTSTicker::FDelegateHandle TickHandle;
};
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "ReactorMQClientPool.h"

#include "ReactorMQBuildConfig.h"
#include "ReactorMQClient.h"
#include "Containers/BackgroundableTicker.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"

using namespace UE;

TSharedPtr<FReactorMQClient> FReactorMQClientPool::Create(
    const FReactorMQConnectionSettings& InConnectionSettings,
    FDeleter&& InDeleter)
{
    return MakeShareable(new FReactorMQClient(InConnectionSettings), MoveTemp(InDeleter));
}

FReactorMQClientPool::FReactorMQClientPool()
{
    if constexpr (GReactorMQThreadMode == EReactorMQThreadMode::GameThread)
    {
        const FTickerDelegate TickDelegate = FTickerDelegate::CreateRaw(this, &FReactorMQClientPool::GameThreadTick);
        TickHandle = FTSBackgroundableTicker::GetCoreTicker().AddTicker(TickDelegate, 0.0f);
    }
}

FReactorMQClientPool::~FReactorMQClientPool()
{
    Kill();
}

TSharedPtr<IReactorMQClient, ESPMode::ThreadSafe> FReactorMQClientPool::GetOrCreateClient(
    const FReactorMQConnectionSettings& InConnectionSettings)
{
    const uint32 Hash = InConnectionSettings.GetHashCode();

    {
        FScopeLock Lock(&ClientMapLock);
        if (TWeakPtr<FReactorMQClient, ESPMode::ThreadSafe>* Existing = Clients.Find(Hash))
        {
            if (TSharedPtr<FReactorMQClient, ESPMode::ThreadSafe> ExistingClient = Existing->Pin())
            {
                return StaticCastSharedPtr<IReactorMQClient>(ExistingClient);
            }
        }
    }

    TWeakPtr<FReactorMQClientPool> WeakSelf = AsShared();

    auto Deleter = [WeakSelf](FReactorMQClient* InClient)
    {
        TSharedPtr<FReactorMQClientPool> StrongSelf = WeakSelf.Pin();
        if (!StrongSelf.IsValid())
        {
            delete InClient;
            return;
        }

        const uint32 DeleterHash = InClient->GetConnectionSettings().GetHashCode();

        {
            FScopeLock DeleterLock(&StrongSelf->ClientMapLock);
            StrongSelf->Clients.Remove(DeleterHash);
        }

        delete InClient;
    };

    TSharedPtr<FReactorMQClient> OutClient = Create(InConnectionSettings, MoveTemp(Deleter));

    if (!OutClient.IsValid())
    {
        return nullptr;
    }

    if constexpr (GReactorMQThreadMode != EReactorMQThreadMode::GameThread)
    {
        bool bExpected = false;
        if (Thread == nullptr && bIsRunning.compare_exchange_strong(bExpected, true, std::memory_order_acq_rel))
        {
            Thread = FRunnableThread::Create(
                this,
                TEXT("FReactorMQClientPool"),
                0,
                TPri_Normal,
                FPlatformAffinity::GetPoolThreadMask());
        }
    }

    {
        FScopeLock Lock(&ClientMapLock);
        Clients.Add(Hash, OutClient.ToSharedRef());
    }

    return StaticCastSharedPtr<IReactorMQClient>(OutClient);
}

void FReactorMQClientPool::Kill()
{
    bIsRunning.store(false, std::memory_order_release);

    if (Thread != nullptr)
    {
        Thread->Kill(true);
        delete Thread;
        Thread = nullptr;
    }

    if (TickHandle.IsValid())
    {
        FTSBackgroundableTicker::GetCoreTicker().RemoveTicker(TickHandle);
        TickHandle.Reset();
    }

    FScopeLock Lock(&ClientMapLock);
    Clients.Empty();
}

uint32 FReactorMQClientPool::Run()
{
    constexpr float MaxTickRate = 1.0f / 60.0f;
    constexpr float MinWaitTime = 0.001f;

    while (bIsRunning.load(std::memory_order_acquire))
    {
        const double StartTime = FPlatformTime::Seconds();
        Tick();
        const double EndTime = FPlatformTime::Seconds();
        const float SleepTime = MaxTickRate - static_cast<float>(EndTime - StartTime);
        FPlatformProcess::SleepNoStats(FMath::Max(MinWaitTime, SleepTime));
    }

    return 0;
}

void FReactorMQClientPool::Exit()
{
    FRunnable::Exit();
}

bool FReactorMQClientPool::Init()
{
    return FRunnable::Init();
}

void FReactorMQClientPool::Stop()
{
    FScopeLock Lock(&ClientMapLock);
    Clients.Empty();
}

FSingleThreadRunnable* FReactorMQClientPool::GetSingleThreadInterface()
{
    return this;
}

void FReactorMQClientPool::TickInternal()
{
    // Take a snapshot of the current clients under the lock, then tick them
    // without holding the lock to avoid modifying the container while it is
    // being iterated (which triggers SparseArray's safety ensures).
    TArray<TWeakPtr<FReactorMQClient, ESPMode::ThreadSafe>> ClientsSnapshot;
    {
        FScopeLock Lock(&ClientMapLock);
        ClientsSnapshot.Reserve(Clients.Num());
        for (const auto& Pair : Clients)
        {
            ClientsSnapshot.Add(Pair.Value);
        }
    }

    for (const TWeakPtr<FReactorMQClient, ESPMode::ThreadSafe>& WeakClient : ClientsSnapshot)
    {
        if (TSharedPtr<FReactorMQClient, ESPMode::ThreadSafe> Client = WeakClient.Pin())
        {
            Client->Tick();
        }
    }
}

void FReactorMQClientPool::Tick()
{
    TickInternal();
}

bool FReactorMQClientPool::GameThreadTick(float DeltaTime)
{
    check(IsInGameThread());
    TickInternal();
    return true;
}
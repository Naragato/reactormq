#include "ReactorMQ.h"
#include "LogReactorMQ.h"
#include "ReactorMQClientPool.h"

DEFINE_LOG_CATEGORY(LogReactorMQ);

#define LOCTEXT_NAMESPACE "FReactorMQModule"

void FReactorMQModule::StartupModule()
{
    ClientPool = MakeShared<FReactorMQClientPool>();
}

void FReactorMQModule::ShutdownModule()
{
    if (ClientPool.IsValid())
    {
        ClientPool->Kill();
        ClientPool.Reset();
    }
}

FReactorMQModule& FReactorMQModule::Get()
{
    return FModuleManager::LoadModuleChecked<FReactorMQModule>(TEXT("ReactorMQ"));
}

FReactorMQClientPool& FReactorMQModule::GetClientPool() const
{
    check(ClientPool.IsValid());
    return *ClientPool.Get();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FReactorMQModule, ReactorMQ)
#pragma once
#include "Templates/SharedPointer.h"

template<typename TResultType>
class TReactorMQResult
{
private:
    TSharedPtr<TResultType> Result;
    bool bSuccess;

public:
    explicit TReactorMQResult(const bool bInSuccess)
        : Result{ nullptr }
          , bSuccess{ bInSuccess }
    {
    }

    explicit TReactorMQResult(const bool bInSuccess, TResultType&& InResult)
        : Result(MakeShared<TResultType>(MoveTemp(InResult)))
          , bSuccess{ bInSuccess }
    {
    }

    TSharedPtr<TResultType> GetResult() const
    {
        return Result;
    }

    bool HasSucceeded() const
    {
        return bSuccess;
    }
};

template<>
class TReactorMQResult<void>
{
private:
    bool bSuccess;

public:
    explicit TReactorMQResult(const bool bInSuccess)
        : bSuccess{ bInSuccess }
    {
    }

    bool HasSucceeded() const
    {
        return bSuccess;
    }
};
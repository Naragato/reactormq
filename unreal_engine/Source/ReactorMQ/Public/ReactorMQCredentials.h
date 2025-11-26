//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Represents MQTT credentials for ReactorMQ.
 */
struct REACTORMQ_API FReactorMQCredentials final
{
    FString Username;
    FString Password;
};

/**
 * @brief Interface for MQTT credentials providers used by ReactorMQ.
 */
class REACTORMQ_API IReactorMQCredentialsProvider
{
public:
    virtual ~IReactorMQCredentialsProvider() = default;

    /**
     * @brief Get credentials for the connection.
     */
    virtual FReactorMQCredentials GetCredentials() = 0;

    /**
     * @brief Optional enhanced authentication method name (MQTT v5 AUTH).
     */
    virtual FString GetAuthMethod()
    {
        return FString();
    }

    /**
     * @brief Optional initial authentication data for CONNECT.
     */
    virtual TArray<uint8> GetInitialAuthData()
    {
        return TArray<uint8>();
    }

    /**
     * @brief Handle a server AUTH challenge and return the next client data.
     */
    virtual TArray<uint8> OnAuthChallenge(const TArray<uint8>& /*ServerData*/)
    {
        return TArray<uint8>();
    }
};

using FReactorMQCredentialsProviderPtr = TSharedPtr<IReactorMQCredentialsProvider>;
using FReactorMQCredentialsProviderRef = TSharedRef<IReactorMQCredentialsProvider>;
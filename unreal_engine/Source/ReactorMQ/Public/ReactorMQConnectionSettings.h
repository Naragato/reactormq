//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "CoreMinimal.h"
#include "ReactorMQTypes.h"
#include "ReactorMQCredentials.h"

struct REACTORMQ_API FReactorMQConnectionSettings
{
    FString ClientId;
    FString Host;
    FString Path;
    int32 Port = 1883;
    EReactorMQConnectionProtocol Protocol = EReactorMQConnectionProtocol::Tcp;
    uint32 MaxPacketSize = 1024 * 1024;
    uint32 MaxBufferSize = 64 * 1024 * 1024;
    uint32 MaxOutboundQueueBytes = 10 * 1024 * 1024;
    uint16 PacketRetryIntervalSeconds = 5;
    double PacketRetryBackoffMultiplier = 1.5;
    uint16 MaxPacketRetryIntervalSeconds = 60;
    uint16 SocketConnectionTimeoutSeconds = 30;
    uint16 KeepAliveIntervalSeconds = 60;
    uint16 MqttConnectionTimeoutSeconds = 30;
    uint16 InitialRetryConnectionIntervalSeconds = 5;
    uint8 MaxConnectionRetries = 5;
    uint8 MaxPacketRetries = 3;
    bool bShouldVerifyServerCertificate = true;
    uint32 SessionExpiryInterval = 0;
    bool bAutoReconnectEnabled = false;
    uint32 AutoReconnectInitialDelayMs = 1000;
    uint32 AutoReconnectMaxDelayMs = 60000;
    double AutoReconnectMultiplier = 2.0;
    bool bStrictMode = false;
    bool bEnforceMaxPacketSize = true;
    uint32 MaxInboundPacketsPerTick = 100;
    uint32 MaxPendingCommands = 1000;

    /**
     * @brief Credentials provider used to resolve authentication data.
     */
    FReactorMQCredentialsProviderPtr CredentialsProvider;

    uint32 GetHashCode() const
    {
        uint32 Hash = 0;
        Hash = HashCombine(Hash, GetTypeHash(ClientId));
        Hash = HashCombine(Hash, GetTypeHash(Protocol));
        Hash = HashCombine(Hash, GetTypeHash(Port));
        Hash = HashCombine(Hash, GetTypeHash(Host));
        Hash = HashCombine(Hash, GetTypeHash(InitialRetryConnectionIntervalSeconds));
        Hash = HashCombine(Hash, GetTypeHash(PacketRetryIntervalSeconds));
        Hash = HashCombine(Hash, GetTypeHash(PacketRetryBackoffMultiplier));
        Hash = HashCombine(Hash, GetTypeHash(MaxPacketRetryIntervalSeconds));
        Hash = HashCombine(Hash, GetTypeHash(SocketConnectionTimeoutSeconds));
        Hash = HashCombine(Hash, GetTypeHash(KeepAliveIntervalSeconds));
        Hash = HashCombine(Hash, GetTypeHash(MqttConnectionTimeoutSeconds));
        Hash = HashCombine(Hash, GetTypeHash(MaxConnectionRetries));
        return Hash;
    }
};

inline uint32 GetTypeHash(const FReactorMQConnectionSettings& Settings)
{
    return Settings.GetHashCode();
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once
#include "HAL/Platform.h"

enum class EReactorMQConnectionProtocol : uint8
{
    Tcp = 0,
    Tls = 1,
    Ws = 2,
    Wss = 3,
    Unknown = 255
};

enum class EReactorMQQualityOfService : uint8
{
    AtMostOnce = 0,
    AtLeastOnce = 1,
    ExactlyOnce = 2
};

enum class EReactorMQConnectReturnCode : uint8
{
    Accepted = 0,
    RefusedProtocolVersion = 1,
    RefusedIdentifierRejected = 2,
    RefusedServerUnavailable = 3,
    RefusedBadUserNameOrPassword = 4,
    RefusedNotAuthorized = 5,
    Cancelled = 255
};

enum class EReactorMQSubscribeReturnCode : uint8
{
    SuccessQualityOfService0 = 0,
    SuccessQualityOfService1 = 1,
    SuccessQualityOfService2 = 2,
    Failure = 128
};

enum class EReactorMQRetainHandling : uint8
{
    SendRetainedMessagesAtSubscribeTime = 0,
    SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist = 1,
    DoNotSendRetainedMessages = 2
};
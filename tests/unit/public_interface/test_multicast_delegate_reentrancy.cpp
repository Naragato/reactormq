//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>
#include "reactormq/mqtt/delegates.h"

using namespace reactormq::mqtt;

TEST(Delegates_MulticastDelegate, Reentrancy_DisconnectNextCallback)
{
    MulticastDelegate<void(int)> delegate;
    int callCount1 = 0;
    int callCount2 = 0;
    DelegateHandle handle2;

    auto handle1 = delegate.add([&](int) {
        callCount1++;
        handle2.disconnect();
    });

    handle2 = delegate.add([&](int) {
        callCount2++;
    });

    delegate.broadcast(1);

    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 0) << "Callback 2 should not be called if disconnected by Callback 1";
}

TEST(Delegates_MulticastDelegate, Reentrancy_DisconnectSelf)
{
    MulticastDelegate<void(int)> delegate;
    int callCount = 0;
    DelegateHandle handle;

    handle = delegate.add([&](int) {
        callCount++;
        handle.disconnect();
    });

    delegate.broadcast(1);
    EXPECT_EQ(callCount, 1);

    delegate.broadcast(1);
    EXPECT_EQ(callCount, 1);
}

TEST(Delegates_MulticastDelegate, Reentrancy_AddDuringBroadcast)
{
    MulticastDelegate<void(int)> delegate;
    int callCount1 = 0;
    int callCount2 = 0;
    
    auto handle1 = delegate.add([&](int val) {
        callCount1++;
        delegate.add([&](int) {
            callCount2++;
        });
    });

    delegate.broadcast(1);
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 0);

    delegate.broadcast(1);
    EXPECT_EQ(callCount1, 2);
    EXPECT_EQ(callCount2, 1);
}

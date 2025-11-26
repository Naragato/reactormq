//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "reactormq/mqtt/result.h"

#include <memory>

using namespace reactormq::mqtt;

TEST(MqttTypes_Result, GenericAndVoid)
{
    const Result ok{ true, 42 };
    ASSERT_TRUE(ok.hasSucceeded());
    const auto val = ok.getResult();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 42);

    const Result<int> fail{ false };
    EXPECT_FALSE(fail.hasSucceeded());
    EXPECT_EQ(fail.getResult(), nullptr);

    const Result<void> voidOk{ true };
    EXPECT_TRUE(voidOk.hasSucceeded());
    const Result<void> voidFail{ false };
    EXPECT_FALSE(voidFail.hasSucceeded());
}
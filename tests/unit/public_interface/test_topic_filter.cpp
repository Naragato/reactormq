//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>
#include <string>

#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/retain_handling_options.h"
#include "reactormq/mqtt/topic_filter.h"

using namespace reactormq::mqtt;

TEST(MqttTypes_TopicFilter, DefaultAndParameterized)
{
    TopicFilter def{};
    EXPECT_EQ(def.getFilter(), "");
    EXPECT_EQ(def.getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_TRUE(def.getIsNoLocal());
    EXPECT_TRUE(def.getIsRetainAsPublished());
    EXPECT_EQ(def.getRetainHandlingOptions(), RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime);

    TopicFilter t{ "sensors/+/temp",
                   QualityOfService::AtLeastOnce,
                   false,
                   false,
                   RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime };

    EXPECT_EQ(t.getFilter(), std::string{ "sensors/+/temp" });
    EXPECT_EQ(t.getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_FALSE(t.getIsNoLocal());
    EXPECT_FALSE(t.getIsRetainAsPublished());
    EXPECT_EQ(t.getRetainHandlingOptions(), RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime);

    TopicFilter
        t2{ "sensors/+/temp", QualityOfService::AtLeastOnce, false, false, RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime };
    EXPECT_TRUE(t == t2);
    EXPECT_FALSE(t != t2);
    EXPECT_TRUE(t == std::string{ "sensors/+/temp" });
    EXPECT_FALSE(t != std::string{ "sensors/+/temp" });
}
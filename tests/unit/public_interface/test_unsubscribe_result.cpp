//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>
#include <reactormq/mqtt/quality_of_service.h>

#include "reactormq/mqtt/topic_filter.h"
#include "reactormq/mqtt/unsubscribe_result.h"

using namespace reactormq::mqtt;

TEST(MqttTypes_UnsubscribeResult, CarriesData)
{
    const TopicFilter tf{ "a/b/#", QualityOfService::ExactlyOnce };
    const UnsubscribeResult r{ tf, true };

    EXPECT_TRUE(r.wasSuccessful());
    EXPECT_EQ(r.getFilter(), tf);
}
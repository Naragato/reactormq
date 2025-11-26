//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "reactormq/mqtt/connection_protocol.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/subscribe_return_code.h"

using namespace reactormq::mqtt;

TEST(MqttTypes_Enums, QualityOfService_ToString)
{
    EXPECT_STREQ(qualityOfServiceToString(QualityOfService::AtMostOnce), "QoS 0 - At most once");
    EXPECT_STREQ(qualityOfServiceToString(QualityOfService::AtLeastOnce), "QoS 1 - At least once");
    EXPECT_STREQ(qualityOfServiceToString(QualityOfService::ExactlyOnce), "QoS 2 - Exactly once");
}

TEST(MqttTypes_Enums, SubscribeReturnCode)
{
    EXPECT_EQ(static_cast<uint8_t>(SubscribeReturnCode::SuccessQualityOfService0), 0);
    EXPECT_EQ(static_cast<uint8_t>(SubscribeReturnCode::SuccessQualityOfService1), 1);
    EXPECT_EQ(static_cast<uint8_t>(SubscribeReturnCode::SuccessQualityOfService2), 2);
    EXPECT_EQ(static_cast<uint8_t>(SubscribeReturnCode::Failure), 128);
}
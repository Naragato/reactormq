//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "mqtt/client/client_factory.h"
#include "mqtt/client/client_impl.h"
#include "reactormq/mqtt/connection_settings_builder.h"

using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;

TEST(ClientFactoryTest, ReturnsNonNullClientInstance)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883);
    const auto settings = builder.build();
    const auto client = createClient(settings);
    EXPECT_NE(client, nullptr);
}

TEST(ClientFactoryTest, ReturnsSameTypeForSameSettingsType)
{
    ConnectionSettingsBuilder builder;
    builder.setHost("localhost").setPort(1883);
    const auto settings = builder.build();
    const auto client = createClient(settings);

    const auto impl = std::reinterpret_pointer_cast<ClientImpl>(client);
    EXPECT_NE(impl, nullptr);
}
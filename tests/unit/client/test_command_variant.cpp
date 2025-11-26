//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "mqtt/client/command.h"

using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;

TEST(CommandVariantTest, HoldsConnectCommand)
{
    ConnectCommand c{ true, std::promise<Result<void>>{} };
    Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<ConnectCommand>(v));
    EXPECT_NE(std::get_if<ConnectCommand>(&v), nullptr);
}

TEST(CommandVariantTest, HoldsPublishCommand)
{
    Message msg{ "t", Message::Payload{ 1, 2, 3 }, false, QualityOfService::AtMostOnce };
    PublishCommand c{ std::move(msg), std::promise<Result<void>>{} };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<PublishCommand>(v));
}

TEST(CommandVariantTest, HoldsSubscribesCommand)
{
    SubscribesCommand c{ { TopicFilter{ "a/b", QualityOfService::AtLeastOnce } }, std::promise<Result<std::vector<SubscribeResult>>>{} };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<SubscribesCommand>(v));
}

TEST(CommandVariantTest, HoldsSubscribeCommand)
{
    SubscribeCommand c{ TopicFilter{ "a/b", QualityOfService::AtLeastOnce }, std::promise<Result<SubscribeResult>>{} };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<SubscribeCommand>(v));
}

TEST(CommandVariantTest, HoldsUnsubscribesCommand)
{
    UnsubscribesCommand c{ { "a/b" }, std::promise<Result<std::vector<UnsubscribeResult>>>{} };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<UnsubscribesCommand>(v));
}

TEST(CommandVariantTest, HoldsUnsubscribeCommand)
{
    UnsubscribeCommand c{ "a/b", std::promise<Result<UnsubscribeResult>>{} };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<UnsubscribeCommand>(v));
}

TEST(CommandVariantTest, HoldsDisconnectCommand)
{
    DisconnectCommand c{ std::promise<Result<void>>{} };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<DisconnectCommand>(v));
}

TEST(CommandVariantTest, HoldsCloseSocketCommand)
{
    CloseSocketCommand c{ 1000, "bye" };
    const Command v = std::move(c);
    EXPECT_TRUE(std::holds_alternative<CloseSocketCommand>(v));
}
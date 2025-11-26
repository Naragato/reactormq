//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "mqtt/client/client_factory.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/message.h"

#include <future>

using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;

namespace
{
    ConnectionSettingsPtr makeSettings()
    {
        ConnectionSettingsBuilder b;
        b.setHost("localhost");
        return b.build();
    }
} // namespace

TEST(ClientImplTest, ConnectAsyncEnqueuesConnectCommandAndReturnsFuture)
{
    const auto client = createClient(makeSettings());
    const auto future = client->connectAsync(true);
    EXPECT_TRUE(future.valid());

    client->tick();
}

TEST(ClientImplTest, DisconnectAsyncEnqueuesDisconnectCommandAndReturnsFuture)
{
    const auto client = createClient(makeSettings());
    auto future = client->disconnectAsync();

    client->tick();

    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_TRUE(result.isSuccess());
}

TEST(ClientImplTest, PublishAsyncMovesMessageIntoCommandQueue)
{
    const auto client = createClient(makeSettings());
    Message msg{ "a/b", Message::Payload{ 1, 2, 3 }, false, QualityOfService::AtMostOnce };
    auto future = client->publishAsync(std::move(msg));

    client->tick();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_FALSE(result.isSuccess());
}

TEST(ClientImplTest, SubscribeAsyncVectorEnqueuesSubscribesCommand)
{
    const auto client = createClient(makeSettings());
    const std::vector topicFilters{ TopicFilter{ "a/b", QualityOfService::AtLeastOnce } };
    auto future = client->subscribeAsync(topicFilters);

    client->tick();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_FALSE(result.isSuccess());
}

TEST(ClientImplTest, SubscribeAsyncSingleTopicFilterEnqueuesSubscribeCommand)
{
    const auto client = createClient(makeSettings());
    auto future = client->subscribeAsync(TopicFilter{ "a/b", QualityOfService::AtLeastOnce });

    client->tick();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_FALSE(result.isSuccess());
}

TEST(ClientImplTest, SubscribeAsyncStringCreatesTopicFilterAndEnqueues)
{
    const auto client = createClient(makeSettings());
    auto future = client->subscribeAsync("a/b");

    client->tick();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_FALSE(result.isSuccess());
}

TEST(ClientImplTest, UnsubscribeAsyncEnqueuesUnsubscribesCommand)
{
    const auto client = createClient(makeSettings());
    auto future = client->unsubscribeAsync({ "a/b" });

    client->tick();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_FALSE(result.isSuccess());
}

TEST(ClientImplTest, EventAccessorsReturnSameDelegateInstances)
{
    const auto client = createClient(makeSettings());
    auto& c1 = client->onConnect();
    auto& c2 = client->onConnect();
    EXPECT_EQ(&c1, &c2);

    auto& d1 = client->onDisconnect();
    auto& d2 = client->onDisconnect();
    EXPECT_EQ(&d1, &d2);

    auto& p1 = client->onPublish();
    auto& p2 = client->onPublish();
    EXPECT_EQ(&p1, &p2);

    auto& s1 = client->onSubscribe();
    auto& s2 = client->onSubscribe();
    EXPECT_EQ(&s1, &s2);

    auto& u1 = client->onUnsubscribe();
    auto& u2 = client->onUnsubscribe();
    EXPECT_EQ(&u1, &u2);

    auto& m1 = client->onMessage();
    auto& m2 = client->onMessage();
    EXPECT_EQ(&m1, &m2);
}

TEST(ClientImplTest, IsConnectedReflectsUnderlyingReactorState)
{
    const auto client = createClient(makeSettings());
    EXPECT_FALSE(client->isConnected());
}

TEST(ClientImplTest, CloseSocketEnqueuesCloseSocketCommand)
{
    const auto client = createClient(makeSettings());
    client->closeSocket(1000, "test");
    client->tick();
}

TEST(ClientImplTest, TickDelegatesToReactorTick)
{
    const auto client = createClient(makeSettings());
    client->tick();
}
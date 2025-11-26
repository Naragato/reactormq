//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/client/command.h"
#include "mqtt/client/context.h"
#include "mqtt/packets/conn_ack.h"
#include "mqtt/packets/connect.h"
#include "mqtt/packets/interface/control_packet.h"
#include "mqtt/packets/properties/properties.h"
#include "mqtt/packets/publish.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"

#include <gtest/gtest.h>
#include <thread>

using namespace reactormq;
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

TEST(ContextTest, AllocatePacketIdReturnsValuesInRange1To65535)
{
    Context ctx(makeSettings());
    const auto id = ctx.allocatePacketId();
    EXPECT_GE(id, 1u);
    EXPECT_LE(id, 65535u);
    EXPECT_TRUE(ctx.isPacketIdInUse(id));
}

TEST(ContextTest, PacketIdIsMarkedInUseAfterAllocation)
{
    Context ctx(makeSettings());
    const auto id = ctx.allocatePacketId();
    EXPECT_TRUE(ctx.isPacketIdInUse(id));
}

TEST(ContextTest, ReleasePacketIdMakesItAvailableAgain)
{
    Context ctx(makeSettings());
    const auto id = ctx.allocatePacketId();
    ctx.releasePacketId(id);
    EXPECT_FALSE(ctx.isPacketIdInUse(id));
}

TEST(ContextTest, AllocatePacketIdReturnsZeroWhenPoolExhausted)
{
    Context ctx(makeSettings());
    std::vector<uint16_t> ids;
    ids.reserve(65535);
    for (uint32_t i = 0; i < 65535; ++i)
    {
        auto id = ctx.allocatePacketId();
        if (id == 0)
        {
            break;
        }
        ids.push_back(id);
    }
    const auto exhausted = ctx.allocatePacketId();
    EXPECT_EQ(exhausted, 0u);
}

TEST(ContextTest, IsPacketIdInUseReflectsReleasedIds)
{
    Context ctx(makeSettings());
    const auto id = ctx.allocatePacketId();
    ASSERT_NE(id, 0u);
    EXPECT_TRUE(ctx.isPacketIdInUse(id));
    ctx.releasePacketId(id);
    EXPECT_FALSE(ctx.isPacketIdInUse(id));
}

TEST(ContextTest, OutboundQueueStartsAtZero)
{
    const Context ctx(makeSettings());
    EXPECT_EQ(ctx.getOutboundQueueSize(), 0u);
}

TEST(ContextTest, CanAddToOutboundQueueWhenBelowLimit)
{
    ConnectionSettingsBuilder b;
    b.setHost("localhost").setMaxOutboundQueueBytes(100);
    Context ctx(b.build());
    EXPECT_TRUE(ctx.canAddToOutboundQueue(50));
    ctx.addOutboundQueueSize(50);
    EXPECT_EQ(ctx.getOutboundQueueSize(), 50u);
}

TEST(ContextTest, CanAddToOutboundQueueReturnsFalseWhenExceedingLimit)
{
    ConnectionSettingsBuilder b;
    b.setHost("localhost").setMaxOutboundQueueBytes(100);
    Context ctx(b.build());
    ctx.addOutboundQueueSize(90);
    EXPECT_FALSE(ctx.canAddToOutboundQueue(20));
}

TEST(ContextTest, SubtractOutboundQueueSizeDoesNotUnderflow)
{
    Context ctx(makeSettings());
    ctx.subtractOutboundQueueSize(10);
    EXPECT_EQ(ctx.getOutboundQueueSize(), 0u);
}

TEST(ContextTest, SubtractOutboundQueueSizeReducesSize)
{
    Context ctx(makeSettings());
    ctx.addOutboundQueueSize(80);
    ctx.subtractOutboundQueueSize(30);
    EXPECT_EQ(ctx.getOutboundQueueSize(), 50u);
}

// Delegates and callback invocation
TEST(ContextTest, OnConnectDelegateIsAccessible)
{
    Context ctx(makeSettings());
    auto& a = ctx.getOnConnect();
    auto& b = ctx.getOnConnect();
    EXPECT_EQ(&a, &b);
}

TEST(ContextTest, OnDisconnectDelegateIsAccessible)
{
    Context ctx(makeSettings());
    auto& a = ctx.getOnDisconnect();
    auto& b = ctx.getOnDisconnect();
    EXPECT_EQ(&a, &b);
}

TEST(ContextTest, OnPublishDelegateIsAccessible)
{
    Context ctx(makeSettings());
    auto& a = ctx.getOnPublish();
    auto& b = ctx.getOnPublish();
    EXPECT_EQ(&a, &b);
}

TEST(ContextTest, OnSubscribeDelegateIsAccessible)
{
    Context ctx(makeSettings());
    auto& a = ctx.getOnSubscribe();
    auto& b = ctx.getOnSubscribe();
    EXPECT_EQ(&a, &b);
}

TEST(ContextTest, OnUnsubscribeDelegateIsAccessible)
{
    Context ctx(makeSettings());
    auto& a = ctx.getOnUnsubscribe();
    auto& b = ctx.getOnUnsubscribe();
    EXPECT_EQ(&a, &b);
}

TEST(ContextTest, OnMessageDelegateIsAccessible)
{
    Context ctx(makeSettings());
    auto& a = ctx.getOnMessage();
    auto& b = ctx.getOnMessage();
    EXPECT_EQ(&a, &b);
}

TEST(ContextTest, InvokeCallbackCallsDirectlyWhenNoExecutor)
{
    Context ctx(makeSettings());
    bool ran = false;
    ctx.invokeCallback(
        [&ran]
        {
            ran = true;
        });
    EXPECT_TRUE(ran);
}

TEST(ContextTest, InvokeCallbackUsesExecutorWhenProvided)
{
    bool usedExecutor = false;
    ConnectionSettingsBuilder b;
    b.setHost("localhost")
        .setCallbackExecutor(
            [&usedExecutor](const auto& cb)
            {
                usedExecutor = true;
                cb();
            });
    Context ctx(b.build());
    bool ran = false;
    ctx.invokeCallback(
        [&ran]
        {
            ran = true;
        });
    EXPECT_TRUE(usedExecutor);
    EXPECT_TRUE(ran);
}

// Pending command maps (publish/subscribe/unsubscribe)
TEST(ContextTest, StoreAndTakePendingPublishByPacketId)
{
    Context ctx(makeSettings());
    PublishCommand cmd{ Message{ "t", Message::Payload{ 1 }, false, QualityOfService::AtMostOnce }, std::promise<Result<void>>{} };
    ctx.storePendingPublish(42, std::move(cmd));
    const auto out = ctx.takePendingPublish(42);
    EXPECT_TRUE(out.has_value());
}

TEST(ContextTest, TakePendingPublishReturnsEmptyForUnknownId)
{
    Context ctx(makeSettings());
    const auto out = ctx.takePendingPublish(999);
    EXPECT_FALSE(out.has_value());
}

TEST(ContextTest, StoreAndTakePendingSubscribeByPacketId)
{
    Context ctx(makeSettings());
    SubscribeCommand cmd{ TopicFilter{ "a/b", QualityOfService::AtLeastOnce }, std::promise<Result<SubscribeResult>>{} };
    ctx.storePendingSubscribe(1, std::move(cmd));
    const auto out = ctx.takePendingSubscribe(1);
    EXPECT_TRUE(out.has_value());
}

TEST(ContextTest, StoreAndTakePendingSubscribesMultiByPacketId)
{
    Context ctx(makeSettings());
    SubscribesCommand cmd{ { TopicFilter{ "a/b", QualityOfService::AtLeastOnce } }, std::promise<Result<std::vector<SubscribeResult>>>{} };
    ctx.storePendingSubscribes(2, std::move(cmd));
    const auto out = ctx.takePendingSubscribes(2);
    EXPECT_TRUE(out.has_value());
}

TEST(ContextTest, StoreAndTakePendingUnsubscribesByPacketId)
{
    Context ctx(makeSettings());
    UnsubscribesCommand cmd{ { "a/b" }, std::promise<Result<std::vector<UnsubscribeResult>>>{} };
    ctx.storePendingUnsubscribes(3, std::move(cmd));
    const auto out = ctx.takePendingUnsubscribes(3);
    EXPECT_TRUE(out.has_value());
}

TEST(ContextTest, GetPendingPublishesReturnsConstMapWithStoredEntries)
{
    Context ctx(makeSettings());
    PublishCommand cmd{ Message{ "t", Message::Payload{ 1 }, false, QualityOfService::AtMostOnce }, std::promise<Result<void>>{} };
    ctx.storePendingPublish(5, std::move(cmd));
    const auto& m = ctx.getPendingPublishes();
    const auto it = m.find(5);
    EXPECT_NE(it, m.end());
}

// Incoming QoS2 messages tracking
TEST(ContextTest, StoreAndTakePendingIncomingQos2Message)
{
    Context ctx(makeSettings());
    ctx.storePendingIncomingQos2Message(10, Message{ "t", Message::Payload{ 1, 2 }, false, QualityOfService::ExactlyOnce });
    const auto out = ctx.takePendingIncomingQos2Message(10);
    EXPECT_TRUE(out.has_value());
}

TEST(ContextTest, TakePendingIncomingQos2MessageReturnsEmptyForUnknownId)
{
    Context ctx(makeSettings());
    const auto out = ctx.takePendingIncomingQos2Message(11);
    EXPECT_FALSE(out.has_value());
}

// Activity / keepalive / ping
TEST(ContextTest, RecordActivityUpdatesLastActivityTime)
{
    Context ctx(makeSettings());
    const auto before = ctx.getTimeSinceLastActivity();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    ctx.recordActivity();
    const auto after = ctx.getTimeSinceLastActivity();
    EXPECT_LE(after.count(), before.count());
}

TEST(ContextTest, GetTimeSinceLastActivityGrowsWithTime)
{
    Context ctx(makeSettings());
    ctx.recordActivity();
    const auto t0 = ctx.getTimeSinceLastActivity();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto t1 = ctx.getTimeSinceLastActivity();
    EXPECT_GE(t1.count(), t0.count());
}

TEST(ContextTest, PingPendingFlagDefaultIsFalse)
{
    const Context ctx(makeSettings());
    EXPECT_FALSE(ctx.isPingPending());
}

TEST(ContextTest, SetPingPendingUpdatesFlag)
{
    Context ctx(makeSettings());
    ctx.setPingPending(true);
    EXPECT_TRUE(ctx.isPingPending());
}

// Publish timeout tracking
TEST(ContextTest, RecordPublishSentStoresTimestampPerPacketId)
{
    Context ctx(makeSettings());
    ctx.recordPublishSent(77);
    const auto elapsed = ctx.getPublishElapsedTime(77);
    EXPECT_GE(elapsed.count(), 0);
}

TEST(ContextTest, GetPublishElapsedTimeIsZeroForUnknownPacketId)
{
    const Context ctx(makeSettings());
    const auto elapsed = ctx.getPublishElapsedTime(1234);
    EXPECT_EQ(elapsed.count(), 0);
}

TEST(ContextTest, GetPublishElapsedTimeIsPositiveAfterDelay)
{
    Context ctx(makeSettings());
    ctx.recordPublishSent(78);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto elapsed = ctx.getPublishElapsedTime(78);
    EXPECT_GT(elapsed.count(), 0);
}

TEST(ContextTest, ClearPublishTimeoutRemovesEntry)
{
    Context ctx(makeSettings());
    ctx.recordPublishSent(79);
    ctx.clearPublishTimeout(79);
    const auto elapsed = ctx.getPublishElapsedTime(79);
    EXPECT_EQ(elapsed.count(), 0);
}

// Incoming packet ID duplicate detection
TEST(ContextTest, TrackIncomingPacketIdReturnsTrueForNewId)
{
    Context ctx(makeSettings());
    EXPECT_TRUE(ctx.trackIncomingPacketId(200));
}

TEST(ContextTest, TrackIncomingPacketIdReturnsFalseForDuplicateId)
{
    Context ctx(makeSettings());
    ASSERT_TRUE(ctx.trackIncomingPacketId(201));
    EXPECT_FALSE(ctx.trackIncomingPacketId(201));
}

TEST(ContextTest, ReleaseIncomingPacketIdRemovesTracking)
{
    Context ctx(makeSettings());
    ASSERT_TRUE(ctx.trackIncomingPacketId(202));
    ctx.releaseIncomingPacketId(202);
    EXPECT_FALSE(ctx.hasIncomingPacketId(202));
}

TEST(ContextTest, HasIncomingPacketIdReflectsTrackingState)
{
    Context ctx(makeSettings());
    EXPECT_FALSE(ctx.hasIncomingPacketId(203));
    EXPECT_TRUE(ctx.trackIncomingPacketId(203));
    EXPECT_TRUE(ctx.hasIncomingPacketId(203));
}

// Pending command count limit
TEST(ContextTest, GetPendingCommandCountCountsPublishesSubscribesUnsubscribes)
{
    using enum QualityOfService;
    Context ctx(makeSettings());
    ctx.storePendingPublish(1, PublishCommand{ Message{ "t", Message::Payload{}, false, AtMostOnce }, std::promise<Result<void>>{} });
    ctx.storePendingSubscribe(2, SubscribeCommand{ TopicFilter{ "a", AtLeastOnce }, std::promise<Result<SubscribeResult>>{} });
    ctx.storePendingSubscribes(
        3,
        SubscribesCommand{ { TopicFilter{ "b", AtLeastOnce } }, std::promise<Result<std::vector<SubscribeResult>>>{} });
    ctx.storePendingUnsubscribes(4, UnsubscribesCommand{ { "a" }, std::promise<Result<std::vector<UnsubscribeResult>>>{} });
    EXPECT_EQ(ctx.getPendingCommandCount(), 4u);
}

TEST(ContextTest, CanAddPendingCommandReturnsTrueBelowLimit)
{
    ConnectionSettingsBuilder b;
    b.setHost("localhost").setMaxPendingCommands(3);
    const Context ctx(b.build());
    EXPECT_TRUE(ctx.canAddPendingCommand());
}

TEST(ContextTest, CanAddPendingCommandReturnsFalseAtLimit)
{
    ConnectionSettingsBuilder b;
    b.setHost("localhost").setMaxPendingCommands(2);
    Context ctx(b.build());
    ctx.storePendingPublish(
        1,
        PublishCommand{ Message{ "t", Message::Payload{}, false, QualityOfService::AtMostOnce }, std::promise<Result<void>>{} });
    ctx.storePendingSubscribe(
        2,
        SubscribeCommand{ TopicFilter{ "a", QualityOfService::AtLeastOnce }, std::promise<Result<SubscribeResult>>{} });
    EXPECT_FALSE(ctx.canAddPendingCommand());
}

TEST(ContextTest, ParsePacketReturnsNullptrOnInvalidData)
{
    const Context ctx(makeSettings());
    EXPECT_EQ(ctx.parsePacket(nullptr, 10), nullptr);
    const std::string buf{ 1, '\0' };
    EXPECT_EQ(ctx.parsePacket(reinterpret_cast<const uint8_t*>(buf.c_str()), 1), nullptr);
}

TEST(ContextTest, ParsePacketReturnsNullptrWhenExceedsMaxPacketSize)
{
    ConnectionSettingsBuilder b;
    b.setHost("localhost").setMaxPacketSize(4);
    const Context ctx(b.build());
    const std::string buf{ 8, '\0' };
    EXPECT_EQ(ctx.parsePacket(reinterpret_cast<const uint8_t*>(buf.c_str()), 8), nullptr);
}

TEST(ContextTest, ParsePacketParsesConnectPacketForV311)
{
    Context ctx(makeSettings());
    ctx.setProtocolVersion(packets::ProtocolVersion::V311);

    std::vector<std::byte> buffer;
    serialize::ByteWriter writer(buffer);
    const packets::Connect<packets::ProtocolVersion::V311> connect("client", 60, "", "", true, false, "", "", QualityOfService::AtMostOnce);
    connect.encode(writer);

    const auto pkt = ctx.parsePacket(std::span{ buffer.data(), buffer.size() });
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->getPacketType(), packets::PacketType::Connect);
}

TEST(ContextTest, ParsePacketParsesConnAckPacketForV5)
{
    Context ctx(makeSettings());
    ctx.setProtocolVersion(packets::ProtocolVersion::V5);

    std::vector<std::byte> buffer;
    serialize::ByteWriter writer(buffer);
    const packets::ConnAck<packets::ProtocolVersion::V5> ack(true, ReasonCode::Success, packets::properties::Properties{});
    ack.encode(writer);

    const auto pkt = ctx.parsePacket(std::span{ buffer.data(), buffer.size() });
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->getPacketType(), packets::PacketType::ConnAck);
}

TEST(ContextTest, ParsePacketParsesPublishPacket)
{
    Context ctx(makeSettings());
    ctx.setProtocolVersion(packets::ProtocolVersion::V5);

    std::vector<std::byte> buffer;
    serialize::ByteWriter writer(buffer);
    const std::string topic = "a/b";
    const std::vector<std::uint8_t> payload = { 1, 2, 3 };
    const packets::Publish<packets::ProtocolVersion::V5> publish(topic, payload, QualityOfService::AtMostOnce, false, 0, {});
    publish.encode(writer);
    const auto pkt = ctx.parsePacket(std::span{ buffer.data(), buffer.size() });
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->getPacketType(), packets::PacketType::Publish);
}
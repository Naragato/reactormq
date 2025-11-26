//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/subscribe.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/retain_handling_options.h"
#include "reactormq/mqtt/topic_filter.h"
#include "serialize/bytes.h"

using namespace reactormq::mqtt::packets;
using namespace reactormq::mqtt::packets::properties;
using namespace reactormq::mqtt;
using namespace reactormq::serialize;

static std::vector<std::byte> toVec(const std::initializer_list<unsigned int> bytes)
{
    std::vector<std::byte> v;
    v.reserve(bytes.size());
    for (const auto b : bytes)
    {
        v.push_back(std::byte{ static_cast<unsigned char>(b) });
    }
    return v;
}

TEST(Subscribe3, Constructor_SingleTopicFilter)
{
    const std::vector topicFilters = { TopicFilter("test/topic", QualityOfService::AtMostOnce) };
    const Subscribe3 packet(topicFilters, 42);

    EXPECT_EQ(packet.getPacketType(), PacketType::Subscribe);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0].getFilter(), "test/topic");
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_TRUE(packet.isValid());
}

TEST(Subscribe3, Constructor_MultipleTopicFilters)
{
    using enum QualityOfService;
    const std::vector topicFilters
        = { TopicFilter("topic1", AtMostOnce),
            TopicFilter("topic2", AtLeastOnce),
            TopicFilter("topic3", ExactlyOnce) };
    const Subscribe3 packet(topicFilters, 100);

    EXPECT_EQ(packet.getPacketId(), 100u);
    EXPECT_EQ(packet.getTopicFilters().size(), 3u);
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_EQ(packet.getTopicFilters()[1].getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_EQ(packet.getTopicFilters()[2].getQualityOfService(), QualityOfService::ExactlyOnce);
}

TEST(Subscribe3, GetLength_SingleTopic)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe3 packet(topicFilters, 1);

    EXPECT_EQ(packet.getLength(), 9u);
}

TEST(Subscribe3, GetLength_MultipleTopic)
{
    const std::vector topicFilters = { TopicFilter("a", QualityOfService::AtMostOnce), TopicFilter("bc", QualityOfService::AtLeastOnce) };
    const Subscribe3 packet(topicFilters, 1);

    EXPECT_EQ(packet.getLength(), 11u);
}

TEST(Subscribe3, Encode_SingleTopicFilter)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtLeastOnce) };
    const Subscribe3 packet(topicFilters, 256);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GT(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x82);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(Subscribe3, Encode_VerifyTopicFilterEncoding)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::ExactlyOnce) };
    const Subscribe3 packet(topicFilters, 42);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GT(buffer.size(), 0u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[buffer.size() - 1]), 0x02);
}

TEST(Subscribe3, Decode_SingleTopicFilter)
{
    const auto payload = toVec({ 0x00, 0x01, 0x00, 0x04, 't', 'e', 's', 't', 0x01 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x82, 0x09 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const Subscribe3 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 1u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0].getFilter(), "test");
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtLeastOnce);
}

TEST(Subscribe3, Decode_MultipleTopicFilters)
{
    const auto payload = toVec({ 0x00, 0x10, 0x00, 0x02, 'a', 'b', 0x00, 0x00, 0x03, 'x', 'y', 'z', 0x02 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x82, 0x0E });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const Subscribe3 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_EQ(packet.getTopicFilters().size(), 2u);
    EXPECT_EQ(packet.getTopicFilters()[0].getFilter(), "ab");
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_EQ(packet.getTopicFilters()[1].getFilter(), "xyz");
    EXPECT_EQ(packet.getTopicFilters()[1].getQualityOfService(), QualityOfService::ExactlyOnce);
}

TEST(Subscribe3, RoundTrip_SingleTopic)
{
    const std::vector topicFilters = { TopicFilter("test/topic", QualityOfService::AtLeastOnce) };
    const Subscribe3 packet1(topicFilters, 1234);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Subscribe3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getTopicFilters().size(), packet1.getTopicFilters().size());
    EXPECT_EQ(packet2.getTopicFilters()[0].getFilter(), packet1.getTopicFilters()[0].getFilter());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Subscribe3, RoundTrip_MultipleTopic)
{
    const std::vector topicFilters
        = { TopicFilter("home/temp", QualityOfService::AtMostOnce), TopicFilter("home/humid", QualityOfService::AtLeastOnce) };
    const Subscribe3 packet1(topicFilters, 99);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Subscribe3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getTopicFilters().size(), 2u);
    EXPECT_EQ(packet2.getTopicFilters()[0].getFilter(), "home/temp");
    EXPECT_EQ(packet2.getTopicFilters()[1].getFilter(), "home/humid");
}

TEST(Subscribe3, FixedHeader_HasRequiredFlags)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe3 packet(topicFilters, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GT(buffer.size(), 0u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x82);
}

TEST(Subscribe5, Constructor_SingleTopicFilter)
{
    const std::vector topicFilters = { TopicFilter("test/topic", QualityOfService::AtMostOnce) };
    const Subscribe5 packet(topicFilters, 42);

    EXPECT_EQ(packet.getPacketType(), PacketType::Subscribe);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Subscribe5, Constructor_WithProperties)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe5 packet(topicFilters, 42, {});

    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
}

TEST(Subscribe5, Constructor_WithSubscriptionOptions)
{
    const std::vector topicFilters = { TopicFilter(
        "test/topic",
        QualityOfService::AtLeastOnce,
        true,
        false,
        RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime) };
    const Subscribe5 packet(topicFilters, 100);

    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_TRUE(packet.getTopicFilters()[0].getIsNoLocal());
    EXPECT_FALSE(packet.getTopicFilters()[0].getIsRetainAsPublished());
    EXPECT_EQ(packet.getTopicFilters()[0].getRetainHandlingOptions(), RetainHandlingOptions::DontSendRetainedMessagesAtSubscribeTime);
}

TEST(Subscribe5, GetLength_SingleTopic)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe5 packet(topicFilters, 1);

    EXPECT_EQ(packet.getLength(), 10u);
}

TEST(Subscribe5, Encode_SingleTopicFilter)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtLeastOnce) };
    const Subscribe5 packet(topicFilters, 256);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GT(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x82);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
}

TEST(Subscribe5, Encode_SubscriptionOptions)
{
    const std::vector topicFilters = { TopicFilter(
        "test",
        QualityOfService::ExactlyOnce,
        true,
        true,
        RetainHandlingOptions::SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist) };
    const Subscribe5 packet(topicFilters, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GT(buffer.size(), 0u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[buffer.size() - 1]), 0x1E);
}

TEST(Subscribe5, Decode_SingleTopicFilter)
{
    const auto payload = toVec({ 0x00, 0x01, 0x00, 0x00, 0x04, 't', 'e', 's', 't', 0x01 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x82, 0x0A });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const Subscribe5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 1u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0].getFilter(), "test");
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtLeastOnce);
}

TEST(Subscribe5, Decode_SubscriptionOptions)
{
    const auto payload = toVec({ 0x00, 0x10, 0x00, 0x00, 0x02, 'a', 'b', 0x1E });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x82, 0x09 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const Subscribe5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0].getQualityOfService(), QualityOfService::ExactlyOnce);
    EXPECT_TRUE(packet.getTopicFilters()[0].getIsNoLocal());
    EXPECT_TRUE(packet.getTopicFilters()[0].getIsRetainAsPublished());
    EXPECT_EQ(
        packet.getTopicFilters()[0].getRetainHandlingOptions(),
        RetainHandlingOptions::SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist);
}

TEST(Subscribe5, Decode_MultipleTopicFilters)
{
    const auto payload = toVec({ 0x00, 0x05, 0x00, 0x00, 0x01, 'a', 0x00, 0x00, 0x01, 'b', 0x01 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x82, 0x0B });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const Subscribe5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getTopicFilters().size(), 2u);
    EXPECT_EQ(packet.getTopicFilters()[0].getFilter(), "a");
    EXPECT_EQ(packet.getTopicFilters()[1].getFilter(), "b");
}

TEST(Subscribe5, Decode_InvalidSubscriptionOptions_ReservedBitsSet)
{
    const auto payload = toVec({ 0x00, 0x01, 0x00, 0x00, 0x02, 'a', 'b', 0xC1 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x82, 0x08 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const Subscribe5 packet(reader, header);

    EXPECT_FALSE(packet.isValid());
}

TEST(Subscribe5, RoundTrip_SingleTopic)
{
    std::vector topicFilters = {
        TopicFilter("test/topic", QualityOfService::AtLeastOnce, false, true, RetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
    };
    Subscribe5 packet1(topicFilters, 1234);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    Subscribe5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet2.getTopicFilters()[0].getFilter(), "test/topic");
    EXPECT_EQ(packet2.getTopicFilters()[0].getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_FALSE(packet2.getTopicFilters()[0].getIsNoLocal());
    EXPECT_TRUE(packet2.getTopicFilters()[0].getIsRetainAsPublished());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Subscribe5, FixedHeader_HasRequiredFlags)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe5 packet(topicFilters, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GT(buffer.size(), 0u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x82);
}

TEST(Subscribe, IsValid_DefaultTrue)
{
    const std::vector topicFilters3 = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe3 packet3(topicFilters3, 100);

    const std::vector topicFilters5 = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe5 packet5(topicFilters5, 100);

    EXPECT_TRUE(packet3.isValid());
    EXPECT_TRUE(packet5.isValid());
}

TEST(Subscribe, GetPacketType_Correct)
{
    const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
    const Subscribe3 packet3(topicFilters, 100);
    const Subscribe5 packet5(topicFilters, 100);

    EXPECT_EQ(packet3.getPacketType(), PacketType::Subscribe);
    EXPECT_EQ(packet5.getPacketType(), PacketType::Subscribe);
}

TEST(Subscribe, MultiplePackets_Consistency)
{
    for (uint16_t i = 1; i <= 5; ++i)
    {
        const std::vector topicFilters = { TopicFilter("test", QualityOfService::AtMostOnce) };
        Subscribe3 packet3(topicFilters, i);
        Subscribe5 packet5(topicFilters, i);

        EXPECT_EQ(packet3.getPacketId(), i);
        EXPECT_EQ(packet5.getPacketId(), i);
        EXPECT_EQ(packet3.getPacketType(), PacketType::Subscribe);
        EXPECT_EQ(packet5.getPacketType(), PacketType::Subscribe);
    }
}
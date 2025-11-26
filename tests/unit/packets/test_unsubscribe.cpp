//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/unsubscribe.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace reactormq::mqtt::packets;
using namespace reactormq::mqtt::packets::properties;
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

TEST(Unsubscribe3, Constructor_SingleTopic)
{
    const std::vector<std::string> topics = { "test/topic" };
    const Unsubscribe3 packet(topics, 42);

    EXPECT_EQ(packet.getPacketType(), PacketType::Unsubscribe);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0], "test/topic");
    EXPECT_TRUE(packet.isValid());
}

TEST(Unsubscribe3, Constructor_MultipleTopics)
{
    const std::vector<std::string> topics = { "topic1", "topic2", "topic3" };
    const Unsubscribe3 packet(topics, 100);

    EXPECT_EQ(packet.getPacketId(), 100u);
    EXPECT_EQ(packet.getTopicFilters().size(), 3u);
    EXPECT_EQ(packet.getTopicFilters()[0], "topic1");
    EXPECT_EQ(packet.getTopicFilters()[1], "topic2");
    EXPECT_EQ(packet.getTopicFilters()[2], "topic3");
}

TEST(Unsubscribe3, GetLength_SingleTopic)
{
    const std::vector<std::string> topics = { "test" };
    const Unsubscribe3 packet(topics, 1);

    EXPECT_EQ(packet.getLength(), 8u);
}

TEST(Unsubscribe3, GetLength_MultipleTopics)
{
    const std::vector<std::string> topics = { "a", "bb", "ccc" };
    const Unsubscribe3 packet(topics, 1);

    EXPECT_EQ(packet.getLength(), 14u);
}

TEST(Unsubscribe3, Encode_SingleTopic)
{
    const std::vector<std::string> topics = { "test" };
    const Unsubscribe3 packet(topics, 42);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 9u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xA2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x08);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x04);
    EXPECT_EQ(static_cast<char>(buffer[6]), 't');
    EXPECT_EQ(static_cast<char>(buffer[7]), 'e');
    EXPECT_EQ(static_cast<char>(buffer[8]), 's');
}

TEST(Unsubscribe3, Encode_MultipleTopics)
{
    const std::vector<std::string> topics = { "a", "bb" };
    const Unsubscribe3 packet(topics, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 11u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xA2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x09);

    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x01);

    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x01);
    EXPECT_EQ(static_cast<char>(buffer[6]), 'a');

    EXPECT_EQ(static_cast<uint8_t>(buffer[7]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[8]), 0x02);
    EXPECT_EQ(static_cast<char>(buffer[9]), 'b');
    EXPECT_EQ(static_cast<char>(buffer[10]), 'b');
}

TEST(Unsubscribe3, Decode_SingleTopic)
{
    const auto data = toVec({ 0xA2, 0x0B, 0x00, 0x10, 0x00, 0x07, 'a', '/', 't', 'o', 'p', 'i', 'c' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Unsubscribe3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0], "a/topic");
}

TEST(Unsubscribe3, Decode_MultipleTopics)
{
    const auto data = toVec({ 0xA2, 0x0E, 0x00, 0x05, 0x00, 0x02, 't', '1', 0x00, 0x02, 't', '2', 0x00, 0x02, 't', '3' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Unsubscribe3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 5u);
    EXPECT_EQ(packet.getTopicFilters().size(), 3u);
    EXPECT_EQ(packet.getTopicFilters()[0], "t1");
    EXPECT_EQ(packet.getTopicFilters()[1], "t2");
    EXPECT_EQ(packet.getTopicFilters()[2], "t3");
}

TEST(Unsubscribe3, Decode_EmptyTopicList_Invalid)
{
    const auto data = toVec({ 0xA2, 0x02, 0x00, 0x01 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Unsubscribe3 packet(payloadReader, header);

    EXPECT_FALSE(packet.isValid());
}

TEST(Unsubscribe3, RoundTrip_SingleTopic)
{
    const std::vector<std::string> topics = { "test/roundtrip" };
    const Unsubscribe3 packet1(topics, 999);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Unsubscribe3 packet2(payloadReader, header);

    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getTopicFilters(), packet1.getTopicFilters());
}

TEST(Unsubscribe3, RoundTrip_MultipleTopics)
{
    const std::vector<std::string> topics = { "sensor/temp", "sensor/humidity", "sensor/pressure" };
    const Unsubscribe3 packet1(topics, 12345);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Unsubscribe3 packet2(payloadReader, header);

    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    ASSERT_EQ(packet2.getTopicFilters().size(), 3u);
    EXPECT_EQ(packet2.getTopicFilters()[0], "sensor/temp");
    EXPECT_EQ(packet2.getTopicFilters()[1], "sensor/humidity");
    EXPECT_EQ(packet2.getTopicFilters()[2], "sensor/pressure");
}

TEST(Unsubscribe3, WireFormat_MatchesSpec)
{
    const std::vector<std::string> topics = { "test" };
    const Unsubscribe3 packet(topics, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xA2) << "Fixed header must be 0xA2 (UNSUBSCRIBE + 0x02 flags)";
}

TEST(Unsubscribe5, Constructor_SingleTopic)
{
    const std::vector<std::string> topics = { "test/topic" };
    const Unsubscribe5 packet(topics, 42);

    EXPECT_EQ(packet.getPacketType(), PacketType::Unsubscribe);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0], "test/topic");
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Unsubscribe5, Constructor_WithProperties)
{
    const std::vector<std::string> topics = { "topic" };
    const Properties props;
    const Unsubscribe5 packet(topics, 10, {});

    EXPECT_EQ(packet.getPacketId(), 10u);
    EXPECT_EQ(packet.getProperties(), props);
}

TEST(Unsubscribe5, GetLength_SingleTopic)
{
    const std::vector<std::string> topics = { "test" };
    const Unsubscribe5 packet(topics, 1);

    EXPECT_EQ(packet.getLength(), 9u);
}

TEST(Unsubscribe5, GetLength_MultipleTopics)
{
    const std::vector<std::string> topics = { "a", "bb" };
    const Unsubscribe5 packet(topics, 1);

    EXPECT_EQ(packet.getLength(), 10u);
}

TEST(Unsubscribe5, Encode_SingleTopic)
{
    const std::vector<std::string> topics = { "test" };
    const Unsubscribe5 packet(topics, 42);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 10u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xA2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x09);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0x04);
}

TEST(Unsubscribe5, Encode_MultipleTopics)
{
    const std::vector<std::string> topics = { "a", "bb" };
    const Unsubscribe5 packet(topics, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 12u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xA2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x0A);
}

TEST(Unsubscribe5, Decode_SingleTopic)
{
    const auto data = toVec({ 0xA2, 0x0C, 0x00, 0x10, 0x00, 0x00, 0x07, 'a', '/', 't', 'o', 'p', 'i', 'c' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Unsubscribe5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_EQ(packet.getTopicFilters().size(), 1u);
    EXPECT_EQ(packet.getTopicFilters()[0], "a/topic");
}

TEST(Unsubscribe5, Decode_MultipleTopics)
{
    const auto data = toVec({ 0xA2, 0x0F, 0x00, 0x05, 0x00, 0x00, 0x02, 't', '1', 0x00, 0x02, 't', '2', 0x00, 0x02, 't', '3' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Unsubscribe5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 5u);
    EXPECT_EQ(packet.getTopicFilters().size(), 3u);
}

TEST(Unsubscribe5, RoundTrip_SingleTopic)
{
    const std::vector<std::string> topics = { "mqtt/v5" };
    const Unsubscribe5 packet1(topics, 500);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Unsubscribe5 packet2(payloadReader, header);

    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getTopicFilters(), packet1.getTopicFilters());
}

TEST(Unsubscribe5, RoundTrip_MultipleTopics)
{
    const std::vector<std::string> topics = { "topic/one", "topic/two" };
    const Unsubscribe5 packet1(topics, 777);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Unsubscribe5 packet2(payloadReader, header);

    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getTopicFilters().size(), 2u);
    EXPECT_EQ(packet2.getTopicFilters()[0], "topic/one");
    EXPECT_EQ(packet2.getTopicFilters()[1], "topic/two");
}

TEST(Unsubscribe5, WireFormat_MatchesSpec)
{
    const std::vector<std::string> topics = { "test" };
    const Unsubscribe5 packet(topics, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xA2) << "Fixed header must be 0xA2 (UNSUBSCRIBE + 0x02 flags)";
}
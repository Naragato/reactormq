//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/publish.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <vector>

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

TEST(Publish3, Constructor_QoS0WithPayload)
{
    const std::string topic = "test/topic";
    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03, 0x04 };

    const Publish3 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, false);

    EXPECT_EQ(packet.getPacketType(), PacketType::Publish);
    EXPECT_EQ(packet.getTopicName(), topic);
    EXPECT_EQ(packet.getPayload(), payload);
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_FALSE(packet.getShouldRetain());
    EXPECT_FALSE(packet.getIsDuplicate());
    EXPECT_EQ(packet.getPacketId(), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Publish3, Constructor_QoS1WithPacketId)
{
    const std::string topic = "sensor/data";
    const std::vector<uint8_t> payload = { 0xAA, 0xBB };

    const Publish3 packet(topic, payload, QualityOfService::AtLeastOnce, false, 42, false);

    EXPECT_EQ(packet.getTopicName(), topic);
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_EQ(packet.getPacketId(), 42u);
}

TEST(Publish3, Constructor_QoS2WithRetain)
{
    const std::string topic = "home/temperature";
    const std::vector<uint8_t> payload = { 0x15 };

    const Publish3 packet(topic, payload, QualityOfService::ExactlyOnce, true, 100, false);

    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::ExactlyOnce);
    EXPECT_TRUE(packet.getShouldRetain());
    EXPECT_EQ(packet.getPacketId(), 100u);
}

TEST(Publish3, Constructor_WithDuplicateFlag)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload = { 0x01 };

    const Publish3 packet(topic, payload, QualityOfService::AtLeastOnce, false, 5, true);

    EXPECT_TRUE(packet.getIsDuplicate());
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtLeastOnce);
}

TEST(Publish3, GetLength_QoS0)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03 };

    const Publish3 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, false);

    EXPECT_EQ(packet.getLength(), 9u);
}

TEST(Publish3, GetLength_QoS1)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03 };

    const Publish3 packet(topic, payload, QualityOfService::AtLeastOnce, false, 1, false);

    EXPECT_EQ(packet.getLength(), 11u);
}

TEST(Publish3, GetLength_QoS2)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03 };

    const Publish3 packet(topic, payload, QualityOfService::ExactlyOnce, false, 1, false);

    EXPECT_EQ(packet.getLength(), 11u);
}

TEST(Publish3, GetLength_EmptyPayload)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload;

    const Publish3 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, false);

    EXPECT_EQ(packet.getLength(), 6u);
}

TEST(Publish3, Encode_QoS0NoRetain)
{
    const std::string topic = "t";
    const std::vector<uint8_t> payload = { 0xAB };

    const Publish3 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x30);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x01);
    EXPECT_EQ(static_cast<char>(buffer[4]), 't');
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0xAB);
}

TEST(Publish3, Encode_QoS1WithPacketId)
{
    const std::string topic = "t";
    const std::vector<uint8_t> payload = { 0xCD };

    const Publish3 packet(topic, payload, QualityOfService::AtLeastOnce, false, 0x1234, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 8u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x32);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x06);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x12);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0x34);
    EXPECT_EQ(static_cast<uint8_t>(buffer[7]), 0xCD);
}

TEST(Publish3, Encode_QoS2WithRetain)
{
    const std::string topic = "t";
    const std::vector<uint8_t> payload = { 0xEF };

    const Publish3 packet(topic, payload, QualityOfService::ExactlyOnce, true, 1, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x35);
}

TEST(Publish3, Encode_WithDuplicateFlag)
{
    const std::string topic = "t";
    const std::vector<uint8_t> payload = { 0x01 };

    const Publish3 packet(topic, payload, QualityOfService::AtLeastOnce, false, 1, true);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x3A);
}

TEST(Publish3, Decode_QoS0)
{
    const auto data = toVec({ 0x30, 0x06, 0x00, 0x02, 't', '1', 0xAA, 0xBB });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Publish3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getTopicName(), "t1");
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_EQ(packet.getPayload().size(), 2u);
    EXPECT_EQ(packet.getPayload()[0], 0xAA);
    EXPECT_EQ(packet.getPayload()[1], 0xBB);
}

TEST(Publish3, Decode_QoS1WithPacketId)
{
    const auto data = toVec({ 0x32, 0x08, 0x00, 0x02, 't', '2', 0x00, 0x2A, 0x11, 0x22 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Publish3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getTopicName(), "t2");
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getPayload().size(), 2u);
}

TEST(Publish3, Decode_QoS2WithRetain)
{
    const auto data = toVec({ 0x35, 0x08, 0x00, 0x02, 't', '3', 0x00, 0x64, 0xFF, 0xEE });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Publish3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::ExactlyOnce);
    EXPECT_TRUE(packet.getShouldRetain());
    EXPECT_EQ(packet.getPacketId(), 100u);
}

TEST(Publish3, Decode_WithDuplicateFlag)
{
    const auto data = toVec({ 0x3A, 0x06, 0x00, 0x01, 't', 0x00, 0x05, 0x99 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Publish3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_TRUE(packet.getIsDuplicate());
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtLeastOnce);
}

TEST(Publish3, RoundTrip_QoS0)
{
    std::string topic = "test/topic";
    std::vector<uint8_t> payload = { 0x01, 0x02, 0x03, 0x04, 0x05 };

    Publish3 packet1(topic, payload, QualityOfService::AtMostOnce, false, 0, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Publish3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getTopicName(), packet1.getTopicName());
    EXPECT_EQ(packet2.getQualityOfService(), packet1.getQualityOfService());
    EXPECT_EQ(packet2.getPayload(), packet1.getPayload());
    EXPECT_EQ(packet2.getShouldRetain(), packet1.getShouldRetain());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Publish3, RoundTrip_QoS1WithAllFlags)
{
    std::string topic = "sensor/temp";
    std::vector<uint8_t> payload = { 0xDE, 0xAD, 0xBE, 0xEF };

    Publish3 packet1(topic, payload, QualityOfService::AtLeastOnce, true, 12345, true);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Publish3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getTopicName(), packet1.getTopicName());
    EXPECT_EQ(packet2.getQualityOfService(), packet1.getQualityOfService());
    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPayload(), packet1.getPayload());
    EXPECT_EQ(packet2.getShouldRetain(), packet1.getShouldRetain());
    EXPECT_EQ(packet2.getIsDuplicate(), packet1.getIsDuplicate());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Publish5, Constructor_QoS0WithPayload)
{
    const std::string topic = "test/topic";
    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03 };

    const Publish5 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, Properties{}, false);

    EXPECT_EQ(packet.getPacketType(), PacketType::Publish);
    EXPECT_EQ(packet.getTopicName(), topic);
    EXPECT_EQ(packet.getPayload(), payload);
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_FALSE(packet.getShouldRetain());
    EXPECT_TRUE(packet.isValid());
}

TEST(Publish5, Constructor_QoS1WithProperties)
{
    const std::string topic = "data";
    const std::vector<uint8_t> payload = { 0xAA };
    const Properties props;

    const Publish5 packet(topic, payload, QualityOfService::AtLeastOnce, false, 42, props, false);

    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getProperties(), props);
}

TEST(Publish5, GetLength_QoS0)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload = { 0x01, 0x02 };

    const Publish5 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, Properties{}, false);

    EXPECT_EQ(packet.getLength(), 9u);
}

TEST(Publish5, GetLength_QoS1WithPacketId)
{
    const std::string topic = "test";
    const std::vector<uint8_t> payload = { 0x01, 0x02 };

    const Publish5 packet(topic, payload, QualityOfService::AtLeastOnce, false, 1, Properties{}, false);

    EXPECT_EQ(packet.getLength(), 11u);
}

TEST(Publish5, Encode_QoS0)
{
    const std::string topic = "t";
    const std::vector<uint8_t> payload = { 0xAB };

    const Publish5 packet(topic, payload, QualityOfService::AtMostOnce, false, 0, Properties{}, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 7u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x30);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x05);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0xAB);
}

TEST(Publish5, Encode_QoS1WithPacketId)
{
    const std::string topic = "t";
    const std::vector<uint8_t> payload = { 0xCD };

    const Publish5 packet(topic, payload, QualityOfService::AtLeastOnce, false, 0x5678, Properties{}, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 9u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x32);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x56);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0x78);
    EXPECT_EQ(static_cast<uint8_t>(buffer[7]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[8]), 0xCD);
}

TEST(Publish5, Decode_QoS0)
{
    const auto data = toVec({ 0x30, 0x07, 0x00, 0x02, 't', '1', 0x00, 0xAA, 0xBB });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Publish5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getTopicName(), "t1");
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtMostOnce);
    EXPECT_EQ(packet.getPayload().size(), 2u);
    EXPECT_EQ(packet.getPayload()[0], 0xAA);
    EXPECT_EQ(packet.getPayload()[1], 0xBB);
}

TEST(Publish5, Decode_QoS1WithPacketId)
{
    const auto data = toVec({ 0x32, 0x09, 0x00, 0x02, 't', '2', 0x00, 0x2A, 0x00, 0x11, 0x22 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Publish5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getTopicName(), "t2");
    EXPECT_EQ(packet.getQualityOfService(), QualityOfService::AtLeastOnce);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getPayload().size(), 2u);
}

TEST(Publish5, RoundTrip_QoS0)
{
    std::string topic = "test/topic";
    std::vector<uint8_t> payload = { 0x01, 0x02, 0x03 };

    Publish5 packet1(topic, payload, QualityOfService::AtMostOnce, false, 0, Properties{}, false);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Publish5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getTopicName(), packet1.getTopicName());
    EXPECT_EQ(packet2.getQualityOfService(), packet1.getQualityOfService());
    EXPECT_EQ(packet2.getPayload(), packet1.getPayload());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Publish5, RoundTrip_QoS2WithRetainAndDup)
{
    std::string topic = "sensor/data";
    std::vector<uint8_t> payload = { 0xDE, 0xAD };

    Publish5 packet1(topic, payload, QualityOfService::ExactlyOnce, true, 999, Properties{}, true);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Publish5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getTopicName(), packet1.getTopicName());
    EXPECT_EQ(packet2.getQualityOfService(), packet1.getQualityOfService());
    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPayload(), packet1.getPayload());
    EXPECT_EQ(packet2.getShouldRetain(), packet1.getShouldRetain());
    EXPECT_EQ(packet2.getIsDuplicate(), packet1.getIsDuplicate());
    EXPECT_TRUE(packet2.isValid());
}
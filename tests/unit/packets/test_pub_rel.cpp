//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/pub_rel.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <gtest/gtest.h>
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

TEST(PubRel3, Constructor_WithPacketId)
{
    const PubRel3 packet(42);

    EXPECT_EQ(packet.getPacketType(), PacketType::PubRel);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(PubRel3, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0x62, 0x02, 0x00, 0x10 });
    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const PubRel3 packet(payloadReader, header);

    EXPECT_EQ(packet.getPacketType(), PacketType::PubRel);
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_TRUE(packet.isValid());
}

TEST(PubRel3, Encode_ValidPacket)
{
    const PubRel3 packet(256);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x62);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(PubRel3, Decode_ValidPacket)
{
    const auto payload = toVec({ 0x12, 0x34 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x62, 0x02 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const PubRel3 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
}

TEST(PubRel3, RoundTrip_EncodeAndDecode)
{
    const PubRel3 packet1(1234);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const PubRel3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_TRUE(packet2.isValid());
}

TEST(PubRel3, GetLength_AlwaysTwo)
{
    const PubRel3 packet1(1);
    const PubRel3 packet2(65535);

    EXPECT_EQ(packet1.getLength(), 2u);
    EXPECT_EQ(packet2.getLength(), 2u);
}

TEST(PubRel3, GetPacketType_Correct)
{
    const PubRel3 packet(100);
    EXPECT_EQ(packet.getPacketType(), PacketType::PubRel);
}

TEST(PubRel3, WireFormat_MatchesSpec)
{
    const PubRel3 packet(42);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x62) << "Byte 0 should be 0x62 (PubRel type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02) << "Byte 1 should be 0x02 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
}

TEST(PubRel5, Constructor_WithPacketIdOnly)
{
    const PubRel5 packet(42);

    EXPECT_EQ(packet.getPacketType(), PacketType::PubRel);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(PubRel5, Constructor_WithReasonCode)
{
    const PubRel5 packet(42, ReasonCode::NoMatchingSubscribers);

    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::NoMatchingSubscribers);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_EQ(packet.getLength(), 4u);
}

TEST(PubRel5, Constructor_WithProperties)
{
    const PubRel5 packet(42, ReasonCode::Success, {});

    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
}

TEST(PubRel5, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0x62, 0x03, 0x00, 0x10, 0x10 });
    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 3);
    const PubRel5 packet(payloadReader, header);

    EXPECT_EQ(packet.getPacketType(), PacketType::PubRel);
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_EQ(packet.getReasonCode(), static_cast<ReasonCode>(0x10));
    EXPECT_TRUE(packet.isValid());
}

TEST(PubRel5, Encode_SuccessNoProperties_Optimized)
{
    const PubRel5 packet(256);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x62);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(PubRel5, Encode_WithReasonCode)
{
    const PubRel5 packet(256, ReasonCode::NoMatchingSubscribers);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x62);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00);
}

TEST(PubRel5, Decode_SuccessNoProperties)
{
    const auto payload = toVec({ 0x12, 0x34 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x62, 0x02 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const PubRel5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
}

TEST(PubRel5, Decode_WithReasonCode)
{
    const auto payload = toVec({ 0x12, 0x34, 0x10, 0x00 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x62, 0x04 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const PubRel5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
    EXPECT_EQ(packet.getReasonCode(), static_cast<ReasonCode>(0x10));
}

TEST(PubRel5, RoundTrip_SuccessNoProperties)
{
    const PubRel5 packet1(1234);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const PubRel5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(PubRel5, RoundTrip_WithReasonCode)
{
    const PubRel5 packet1(1234, ReasonCode::NotAuthorized);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const PubRel5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(PubRel5, GetLength_Optimized)
{
    const PubRel5 packet1(1);
    EXPECT_EQ(packet1.getLength(), 2u);

    const PubRel5 packet2(1, ReasonCode::NoMatchingSubscribers);
    EXPECT_EQ(packet2.getLength(), 4u);
}

TEST(PubRel5, GetPacketType_Correct)
{
    const PubRel5 packet(100);
    EXPECT_EQ(packet.getPacketType(), PacketType::PubRel);
}

TEST(PubRel5, WireFormat_MatchesSpec_Optimized)
{
    const PubRel5 packet(42);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x62) << "Byte 0 should be 0x62 (PubRel type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02) << "Byte 1 should be 0x02 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
}

TEST(PubRel5, WireFormat_MatchesSpec_WithReasonCode)
{
    const PubRel5 packet(42, ReasonCode::NotAuthorized);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x62) << "Byte 0 should be 0x62 (PubRel type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04) << "Byte 1 should be 0x04 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x87) << "Byte 4 should be 0x87 (NotAuthorized)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00) << "Byte 5 should be 0x00 (empty properties)";
}

TEST(PubRel, IsValid_DefaultTrue)
{
    const PubRel3 packet3(100);
    const PubRel5 packet5(100);

    EXPECT_TRUE(packet3.isValid());
    EXPECT_TRUE(packet5.isValid());
}

TEST(PubRel, MultiplePackets_Consistency)
{
    for (uint16_t i = 1; i <= 5; ++i)
    {
        PubRel3 packet3(i);
        PubRel5 packet5(i);

        EXPECT_EQ(packet3.getPacketId(), i);
        EXPECT_EQ(packet5.getPacketId(), i);
        EXPECT_EQ(packet3.getPacketType(), PacketType::PubRel);
        EXPECT_EQ(packet5.getPacketType(), PacketType::PubRel);
    }
}
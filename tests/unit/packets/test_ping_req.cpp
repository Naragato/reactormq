//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/ping_req.h"
#include "serialize/bytes.h"

using namespace reactormq::mqtt::packets;
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

TEST(PingReq, DefaultConstructor)
{
    const PingReq packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::PingReq);
    EXPECT_EQ(packet.getLength(), 0u);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 0u);
}

TEST(PingReq, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0xC0, 0x00 });
    ByteReader reader(data.data(), data.size());
    const FixedHeader header = FixedHeader::create(reader);

    const PingReq packet(header);

    EXPECT_EQ(packet.getPacketType(), PacketType::PingReq);
    EXPECT_EQ(packet.getLength(), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(PingReq, Encode_ValidPacket)
{
    const PingReq packet;
    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);

    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xC0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);
}

TEST(PingReq, Decode_ValidPacket)
{
    const std::vector<std::byte> buffer;
    ByteReader reader(buffer.data(), buffer.size());
    const auto data = toVec({ 0xC0, 0x00 });
    ByteReader headerReader(data.data(), data.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    PingReq packet(header);
    const bool result = packet.decode(reader);

    EXPECT_TRUE(result);
    EXPECT_TRUE(packet.isValid());
}

TEST(PingReq, RoundTrip_EncodeAndDecode)
{
    const PingReq packet1;
    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);

    packet1.encode(writer);
    ByteReader reader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(reader);
    PingReq packet2(header);
    const bool result = packet2.decode(reader);

    EXPECT_TRUE(result);
    EXPECT_EQ(packet2.getPacketType(), PacketType::PingReq);
    EXPECT_EQ(packet2.getLength(), 0u);
    EXPECT_TRUE(packet2.isValid());
}

TEST(PingReq, GetLength_AlwaysZero)
{
    const PingReq packet;

    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(PingReq, GetPacketType_Correct)
{
    const PingReq packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::PingReq);
}

TEST(PingReq, GetPacketId_AlwaysZero)
{
    const PingReq packet;

    EXPECT_EQ(packet.getPacketId(), 0u);
}

TEST(PingReq, IsValid_DefaultTrue)
{
    const PingReq packet;

    EXPECT_TRUE(packet.isValid());
}

TEST(PingReq, InvalidPacket_NonZeroRemainingLength)
{
    const auto data = toVec({ 0xC0, 0x01, 0x00 });
    ByteReader reader(data.data(), data.size());
    const FixedHeader header = FixedHeader::create(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::PingReq);
    EXPECT_EQ(header.getRemainingLength(), 1u);

    const PingReq packet(header);

    EXPECT_EQ(packet.getPacketType(), PacketType::PingReq);
}

TEST(PingReq, Encode_MultiplePackets)
{
    for (int i = 0; i < 5; ++i)
    {
        PingReq packet;

        std::vector<std::byte> buffer;
        ByteWriter writer(buffer);
        packet.encode(writer);

        ASSERT_EQ(buffer.size(), 2u);
        EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xC0);
        EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);
    }
}

TEST(PingReq, WireFormat_MatchesMqttifySpec)
{
    const PingReq packet;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xC0) << "Byte 0 should be 0xC0 (PingReq packet type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00) << "Byte 1 should be 0x00 (remaining length = 0)";
}
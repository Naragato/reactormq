//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

#include "mqtt/packets/packet_type.h"
#include "mqtt/packets/ping_resp.h"
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

TEST(PingResp, DefaultConstructor)
{
    const PingResp packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::PingResp);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getLength(), 0u);
    EXPECT_EQ(packet.getPacketId(), 0u);
}

TEST(PingResp, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0xD0, 0x00 });
    ByteReader reader(data.data(), data.size());
    const FixedHeader header = FixedHeader::create(reader);

    const PingResp packet(header);

    EXPECT_EQ(packet.getPacketType(), PacketType::PingResp);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(PingResp, Encode_ValidPacket)
{
    const PingResp packet;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(buffer.size(), 2u);
    EXPECT_EQ(buffer[0], std::byte{ 0xD0 });
    EXPECT_EQ(buffer[1], std::byte{ 0x00 });
}

TEST(PingResp, Decode_ValidPacket)
{
    const auto data = toVec({ 0xD0, 0x00 });
    ByteReader reader(data.data(), data.size());
    const FixedHeader header = FixedHeader::create(reader);

    PingResp packet(header);
    const bool result = packet.decode(reader);

    EXPECT_TRUE(result);
    EXPECT_EQ(packet.getPacketType(), PacketType::PingResp);
    EXPECT_TRUE(packet.isValid());
}

TEST(PingResp, RoundTrip_EncodeAndDecode)
{
    const PingResp packet1;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader reader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(reader);
    PingResp packet2(header);
    const bool result = packet2.decode(reader);

    EXPECT_TRUE(result);
    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_EQ(packet2.getLength(), packet1.getLength());
    EXPECT_TRUE(reader.isEof());
}

TEST(PingResp, GetLength_AlwaysReturnsZero)
{
    const PingResp packet;
    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(PingResp, GetPacketType_ReturnsPingResp)
{
    const PingResp packet;
    EXPECT_EQ(packet.getPacketType(), PacketType::PingResp);
}

TEST(PingResp, GetPacketId_ReturnsZero)
{
    const PingResp packet;
    EXPECT_EQ(packet.getPacketId(), 0u);
}

TEST(PingResp, IsValid_DefaultsToTrue)
{
    const PingResp packet;
    EXPECT_TRUE(packet.isValid());
}

TEST(PingResp, InvalidPacket_NonZeroRemainingLength)
{
    const auto data = toVec({ 0xD0, 0x01 });
    ByteReader reader(data.data(), data.size());
    const FixedHeader header = FixedHeader::create(reader);

    const PingResp packet(header);

    EXPECT_EQ(packet.getPacketType(), PacketType::PingResp);
}

TEST(PingResp, MultiplePackets_Consistency)
{
    const PingResp packet1;
    const PingResp packet2;

    std::vector<std::byte> buffer1;
    ByteWriter writer1(buffer1);
    packet1.encode(writer1);

    std::vector<std::byte> buffer2;
    ByteWriter writer2(buffer2);
    packet2.encode(writer2);

    EXPECT_EQ(buffer1.size(), buffer2.size());
    EXPECT_EQ(buffer1, buffer2);
}

TEST(PingResp, WireFormat_MatchesSpec)
{
    const PingResp packet;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    const std::vector<std::byte> expected = toVec({ 0xD0, 0x00 });

    EXPECT_EQ(buffer, expected);
}
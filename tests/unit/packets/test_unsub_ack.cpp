//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/unsub_ack.h"
#include "reactormq/mqtt/reason_code.h"
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

TEST(UnsubAck3, Constructor_WithPacketId)
{
    const UnsubAck3 packet(42);

    EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(UnsubAck3, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0xB0, 0x02, 0x00, 0x10 });
    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const UnsubAck3 packet(payloadReader, header);

    EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_TRUE(packet.isValid());
}

TEST(UnsubAck3, Encode_ValidPacket)
{
    const UnsubAck3 packet(256);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xB0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(UnsubAck3, Decode_ValidPacket)
{
    const auto payload = toVec({ 0x12, 0x34 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0xB0, 0x02 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const UnsubAck3 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
}

TEST(UnsubAck3, RoundTrip_EncodeAndDecode)
{
    const UnsubAck3 packet1(1234);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const UnsubAck3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_TRUE(packet2.isValid());
}

TEST(UnsubAck3, GetLength_AlwaysTwo)
{
    const UnsubAck3 packet1(1);
    const UnsubAck3 packet2(65535);

    EXPECT_EQ(packet1.getLength(), 2u);
    EXPECT_EQ(packet2.getLength(), 2u);
}

TEST(UnsubAck3, GetPacketType_Correct)
{
    const UnsubAck3 packet(100);
    EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
}

TEST(UnsubAck3, WireFormat_MatchesSpec)
{
    const UnsubAck3 packet(42);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xB0) << "Byte 0 should be 0xB0 (UNSUBACK type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02) << "Byte 1 should be 0x02 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
}

TEST(UnsubAck3, Decode_EmptyPayload_Invalid)
{
    const std::vector<std::byte> payload;
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0xB0, 0x00 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const UnsubAck3 packet(reader, header);

    EXPECT_FALSE(packet.isValid());
}

TEST(UnsubAck3, MultiplePackets_Consistency)
{
    for (uint16_t i = 1; i <= 5; ++i)
    {
        UnsubAck3 packet(i);

        EXPECT_EQ(packet.getPacketId(), i);
        EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
        EXPECT_EQ(packet.getLength(), 2u);
    }
}

TEST(UnsubAck5, Constructor_WithPacketIdAndReasonCodes)
{
    const std::vector reasonCodes = { ReasonCode::Success };
    const UnsubAck5 packet(42, reasonCodes);

    EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(UnsubAck5, Constructor_WithMultipleReasonCodes)
{
    using enum ReasonCode;
    const std::vector reasonCodes = { Success, NoSubscriptionExisted, NotAuthorized };
    const UnsubAck5 packet(100, reasonCodes);

    EXPECT_EQ(packet.getPacketId(), 100u);
    EXPECT_EQ(packet.getReasonCodes().size(), 3u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
    EXPECT_EQ(packet.getReasonCodes()[1], ReasonCode::NoSubscriptionExisted);
    EXPECT_EQ(packet.getReasonCodes()[2], ReasonCode::NotAuthorized);
}

TEST(UnsubAck5, Constructor_WithProperties)
{
    const std::vector reasonCodes = { ReasonCode::Success };

    const UnsubAck5 packet(42, reasonCodes, {});

    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
}

TEST(UnsubAck5, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0xB0, 0x04, 0x00, 0x10, 0x00, 0x11 });
    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 4);
    const UnsubAck5 packet(payloadReader, header);

    EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::NoSubscriptionExisted);
    EXPECT_TRUE(packet.isValid());
}

TEST(UnsubAck5, Encode_SingleReasonCode)
{
    const std::vector reasonCodes = { ReasonCode::Success };
    const UnsubAck5 packet(256, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xB0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00);
}

TEST(UnsubAck5, Encode_MultipleReasonCodes)
{
    using enum ReasonCode;
    const std::vector reasonCodes = { Success, NoSubscriptionExisted, NotAuthorized };
    const UnsubAck5 packet(1, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 8u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xB0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x06);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0x11);
    EXPECT_EQ(static_cast<uint8_t>(buffer[7]), 0x87);
}

TEST(UnsubAck5, Decode_SingleReasonCode)
{
    const auto payload = toVec({ 0x12, 0x34, 0x00, 0x00 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0xB0, 0x04 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const UnsubAck5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
}

TEST(UnsubAck5, Decode_MultipleReasonCodes)
{
    const auto payload = toVec({ 0x00, 0x01, 0x00, 0x00, 0x11, 0x87 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0xB0, 0x06 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const UnsubAck5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 1u);
    EXPECT_EQ(packet.getReasonCodes().size(), 3u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
    EXPECT_EQ(packet.getReasonCodes()[1], ReasonCode::NoSubscriptionExisted);
    EXPECT_EQ(packet.getReasonCodes()[2], ReasonCode::NotAuthorized);
}

TEST(UnsubAck5, RoundTrip_EncodeAndDecode)
{
    const std::vector reasonCodes = { ReasonCode::Success, ReasonCode::NotAuthorized };
    const UnsubAck5 packet1(1234, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const UnsubAck5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getReasonCodes().size(), packet1.getReasonCodes().size());
    for (size_t i = 0; i < packet1.getReasonCodes().size(); ++i)
    {
        EXPECT_EQ(packet2.getReasonCodes()[i], packet1.getReasonCodes()[i]);
    }
}

TEST(UnsubAck5, GetLength_Varies)
{
    using enum ReasonCode;
    const std::vector codes1 = { Success };
    const UnsubAck5 packet1(1, codes1);
    EXPECT_EQ(packet1.getLength(), 4u);

    const std::vector codes2 = { Success, NoSubscriptionExisted, NotAuthorized };
    const UnsubAck5 packet2(1, codes2);
    EXPECT_EQ(packet2.getLength(), 6u);
}

TEST(UnsubAck5, GetPacketType_Correct)
{
    const std::vector reasonCodes = { ReasonCode::Success };
    const UnsubAck5 packet(100, reasonCodes);
    EXPECT_EQ(packet.getPacketType(), PacketType::UnsubAck);
}

TEST(UnsubAck5, WireFormat_MatchesSpec)
{
    const std::vector reasonCodes = { ReasonCode::NoSubscriptionExisted };
    const UnsubAck5 packet(42, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xB0) << "Byte 0 should be 0xB0 (UNSUBACK type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04) << "Byte 1 should be 0x04 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00) << "Byte 4 should be 0x00 (empty properties)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x11) << "Byte 5 should be 0x11 (NoSubscriptionExisted)";
}

TEST(UnsubAck, IsValid_DefaultTrue)
{
    const UnsubAck3 packet3(100);

    const std::vector reasonCodes5 = { ReasonCode::Success };
    const UnsubAck5 packet5(100, reasonCodes5);

    EXPECT_TRUE(packet3.isValid());
    EXPECT_TRUE(packet5.isValid());
}

TEST(UnsubAck, MultiplePackets_Consistency)
{
    for (uint16_t i = 1; i <= 5; ++i)
    {
        UnsubAck3 packet3(i);

        std::vector reasonCodes5 = { ReasonCode::Success };
        UnsubAck5 packet5(i, reasonCodes5);

        EXPECT_EQ(packet3.getPacketId(), i);
        EXPECT_EQ(packet5.getPacketId(), i);
        EXPECT_EQ(packet3.getPacketType(), PacketType::UnsubAck);
        EXPECT_EQ(packet5.getPacketType(), PacketType::UnsubAck);
    }
}
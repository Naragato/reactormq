//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/sub_ack.h"
#include "reactormq/mqtt/reason_code.h"
#include "reactormq/mqtt/subscribe_return_code.h"
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

TEST(SubAck3, Constructor_SingleReturnCode)
{
    const std::vector returnCodes = { SubscribeReturnCode::SuccessQualityOfService0 };
    const SubAck3 packet(42, returnCodes);

    EXPECT_EQ(packet.getPacketType(), PacketType::SubAck);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getLength(), 3u);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], SubscribeReturnCode::SuccessQualityOfService0);
}

TEST(SubAck3, Constructor_MultipleReturnCodes)
{
    using enum SubscribeReturnCode;
    const std::vector returnCodes = { SuccessQualityOfService0, SuccessQualityOfService1, SuccessQualityOfService2, Failure };
    const SubAck3 packet(100, returnCodes);

    EXPECT_EQ(packet.getPacketId(), 100u);
    EXPECT_EQ(packet.getLength(), 6u);
    EXPECT_EQ(packet.getReasonCodes().size(), 4u);
}

TEST(SubAck3, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0x90, 0x03, 0x00, 0x10, 0x00 });
    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 3);
    const SubAck3 packet(payloadReader, header);

    EXPECT_EQ(packet.getPacketType(), PacketType::SubAck);
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], SubscribeReturnCode::SuccessQualityOfService0);
}

TEST(SubAck3, Encode_SingleReturnCode)
{
    const std::vector returnCodes = { SubscribeReturnCode::SuccessQualityOfService1 };
    const SubAck3 packet(256, returnCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x90);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x03);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x01);
}

TEST(SubAck3, Encode_MultipleReturnCodes)
{
    using enum SubscribeReturnCode;
    const std::vector returnCodes = { SuccessQualityOfService0, SuccessQualityOfService2, Failure };
    const SubAck3 packet(42, returnCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 7u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x90);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x05);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0x80);
}

TEST(SubAck3, Decode_SingleReturnCode)
{
    const auto payload = toVec({ 0x12, 0x34, 0x01 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x90, 0x03 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const SubAck3 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], SubscribeReturnCode::SuccessQualityOfService1);
}

TEST(SubAck3, Decode_MultipleReturnCodes)
{
    const auto payload = toVec({ 0x00, 0x01, 0x00, 0x01, 0x02, 0x80 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x90, 0x06 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const SubAck3 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 1u);
    EXPECT_EQ(packet.getReasonCodes().size(), 4u);
    EXPECT_EQ(packet.getReasonCodes()[0], SubscribeReturnCode::SuccessQualityOfService0);
    EXPECT_EQ(packet.getReasonCodes()[1], SubscribeReturnCode::SuccessQualityOfService1);
    EXPECT_EQ(packet.getReasonCodes()[2], SubscribeReturnCode::SuccessQualityOfService2);
    EXPECT_EQ(packet.getReasonCodes()[3], SubscribeReturnCode::Failure);
}

TEST(SubAck3, RoundTrip_EncodeAndDecode)
{
    const std::vector returnCodes = { SubscribeReturnCode::SuccessQualityOfService1, SubscribeReturnCode::Failure };
    const SubAck3 packet1(1234, returnCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const SubAck3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getReasonCodes().size(), packet1.getReasonCodes().size());
    for (size_t i = 0; i < packet1.getReasonCodes().size(); ++i)
    {
        EXPECT_EQ(packet2.getReasonCodes()[i], packet1.getReasonCodes()[i]);
    }
}

TEST(SubAck3, GetLength_Varies)
{
    using enum SubscribeReturnCode;
    const std::vector codes1 = { SuccessQualityOfService0 };
    const SubAck3 packet1(1, codes1);
    EXPECT_EQ(packet1.getLength(), 3u);

    const std::vector codes2 = { SuccessQualityOfService0, SuccessQualityOfService1, SuccessQualityOfService2 };
    const SubAck3 packet2(1, codes2);
    EXPECT_EQ(packet2.getLength(), 5u);
}

TEST(SubAck3, GetPacketType_Correct)
{
    const std::vector returnCodes = { SubscribeReturnCode::SuccessQualityOfService0 };
    const SubAck3 packet(100, returnCodes);
    EXPECT_EQ(packet.getPacketType(), PacketType::SubAck);
}

TEST(SubAck3, WireFormat_MatchesSpec)
{
    const std::vector returnCodes = { SubscribeReturnCode::SuccessQualityOfService2 };
    const SubAck3 packet(42, returnCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x90) << "Byte 0 should be 0x90 (SUBACK type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x03) << "Byte 1 should be 0x03 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x02) << "Byte 4 should be 0x02 (QoS 2)";
}

TEST(SubAck5, Constructor_SingleReasonCode)
{
    const std::vector reasonCodes = { ReasonCode::Success };
    const SubAck5 packet(42, reasonCodes);

    EXPECT_EQ(packet.getPacketType(), PacketType::SubAck);
    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(SubAck5, Constructor_WithProperties)
{
    const std::vector reasonCodes = { ReasonCode::Success };

    const SubAck5 packet(42, reasonCodes, {});

    EXPECT_EQ(packet.getPacketId(), 42u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
}

TEST(SubAck5, Constructor_MultipleReasonCodes)
{
    using enum ReasonCode;
    const std::vector reasonCodes = { Success, GrantedQualityOfService1, GrantedQualityOfService2, NotAuthorized };
    const SubAck5 packet(100, reasonCodes);

    EXPECT_EQ(packet.getPacketId(), 100u);
    EXPECT_EQ(packet.getReasonCodes().size(), 4u);
}

TEST(SubAck5, ConstructorFromFixedHeader)
{
    const auto data = toVec({ 0x90, 0x04, 0x00, 0x10, 0x00, 0x00 });
    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 4);
    const SubAck5 packet(payloadReader, header);

    EXPECT_EQ(packet.getPacketType(), PacketType::SubAck);
    EXPECT_EQ(packet.getPacketId(), 16u);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
}

TEST(SubAck5, Encode_SingleReasonCode)
{
    const std::vector reasonCodes = { ReasonCode::GrantedQualityOfService1 };
    const SubAck5 packet(256, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x90);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x01);
}

TEST(SubAck5, Encode_MultipleReasonCodes)
{
    using enum ReasonCode;
    const std::vector reasonCodes = { Success, GrantedQualityOfService2, NotAuthorized };
    const SubAck5 packet(42, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 8u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x90);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x06);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[7]), 0x87);
}

TEST(SubAck5, Decode_SingleReasonCode)
{
    const auto payload = toVec({ 0x12, 0x34, 0x00, 0x01 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x90, 0x04 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const SubAck5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 4660u);
    EXPECT_EQ(packet.getReasonCodes().size(), 1u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::GrantedQualityOfService1);
}

TEST(SubAck5, Decode_MultipleReasonCodes)
{
    const auto payload = toVec({ 0x00, 0x01, 0x00, 0x00, 0x01, 0x02, 0x87 });
    ByteReader reader(payload.data(), payload.size());

    const auto headerData = toVec({ 0x90, 0x07 });
    ByteReader headerReader(headerData.data(), headerData.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const SubAck5 packet(reader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketId(), 1u);
    EXPECT_EQ(packet.getReasonCodes().size(), 4u);
    EXPECT_EQ(packet.getReasonCodes()[0], ReasonCode::Success);
    EXPECT_EQ(packet.getReasonCodes()[1], ReasonCode::GrantedQualityOfService1);
    EXPECT_EQ(packet.getReasonCodes()[2], ReasonCode::GrantedQualityOfService2);
    EXPECT_EQ(packet.getReasonCodes()[3], ReasonCode::NotAuthorized);
}

TEST(SubAck5, RoundTrip_EncodeAndDecode)
{
    const std::vector reasonCodes = { ReasonCode::GrantedQualityOfService1, ReasonCode::NotAuthorized };
    const SubAck5 packet1(1234, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const SubAck5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketId(), packet1.getPacketId());
    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_TRUE(packet2.isValid());
    EXPECT_EQ(packet2.getReasonCodes().size(), packet1.getReasonCodes().size());
    for (size_t i = 0; i < packet1.getReasonCodes().size(); ++i)
    {
        EXPECT_EQ(packet2.getReasonCodes()[i], packet1.getReasonCodes()[i]);
    }
}

TEST(SubAck5, GetLength_Varies)
{
    using enum ReasonCode;
    const std::vector codes1 = { Success };
    const SubAck5 packet1(1, codes1);
    EXPECT_EQ(packet1.getLength(), 4u);

    const std::vector codes2 = { Success, GrantedQualityOfService1, GrantedQualityOfService2 };

    const SubAck5 packet2(1, codes2);
    EXPECT_EQ(packet2.getLength(), 6u);
}

TEST(SubAck5, GetPacketType_Correct)
{
    const std::vector reasonCodes = { ReasonCode::Success };
    const SubAck5 packet(100, reasonCodes);
    EXPECT_EQ(packet.getPacketType(), PacketType::SubAck);
}

TEST(SubAck5, WireFormat_MatchesSpec)
{
    const std::vector reasonCodes = { ReasonCode::GrantedQualityOfService2 };
    const SubAck5 packet(42, reasonCodes);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 6u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x90) << "Byte 0 should be 0x90 (SUBACK type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x04) << "Byte 1 should be 0x04 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00) << "Byte 2 should be 0x00 (packet ID high byte)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x2A) << "Byte 3 should be 0x2A (packet ID low byte = 42)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00) << "Byte 4 should be 0x00 (empty properties)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[5]), 0x02) << "Byte 5 should be 0x02 (GrantedQualityOfService2)";
}

TEST(SubAck, IsValid_DefaultTrue)
{
    const std::vector returnCodes3 = { SubscribeReturnCode::SuccessQualityOfService0 };
    const SubAck3 packet3(100, returnCodes3);

    const std::vector reasonCodes5 = { ReasonCode::Success };
    const SubAck5 packet5(100, reasonCodes5);

    EXPECT_TRUE(packet3.isValid());
    EXPECT_TRUE(packet5.isValid());
}

TEST(SubAck, MultiplePackets_Consistency)
{
    for (uint16_t i = 1; i <= 5; ++i)
    {
        std::vector returnCodes3 = { SubscribeReturnCode::SuccessQualityOfService0 };
        SubAck3 packet3(i, returnCodes3);

        std::vector reasonCodes5 = { ReasonCode::Success };
        SubAck5 packet5(i, reasonCodes5);

        EXPECT_EQ(packet3.getPacketId(), i);
        EXPECT_EQ(packet5.getPacketId(), i);
        EXPECT_EQ(packet3.getPacketType(), PacketType::SubAck);
        EXPECT_EQ(packet5.getPacketType(), PacketType::SubAck);
    }
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet.h"
#include "mqtt/packets/packet_type.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"

using namespace reactormq::mqtt::packets;
using namespace reactormq::mqtt;
using namespace reactormq::serialize;

class MockControlPacket final : public IControlPacket
{
public:
    MockControlPacket(const PacketType type, const uint32_t length, const uint16_t packetId = 0, const bool valid = true)
        : m_type(type)
        , m_length(length)
        , m_packetId(packetId)
        , m_valid(valid)
    {
    }

    [[nodiscard]] uint32_t getLength() const override
    {
        return m_length;
    }

    [[nodiscard]] PacketType getPacketType() const override
    {
        return m_type;
    }

    [[nodiscard]] uint16_t getPacketId() const override
    {
        return m_packetId;
    }

    [[nodiscard]] bool isValid() const override
    {
        return m_valid;
    }

    void encode(ByteWriter&) const override
    {
        // Intentionally empty
    }

    bool decode(ByteReader&) override
    {
        return true;
    }

private:
    PacketType m_type;
    uint32_t m_length;
    uint16_t m_packetId;
    bool m_valid;
};

// Helper to convert initializer list to vector
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

TEST(FixedHeader, DefaultConstructor)
{
    const FixedHeader header;

    EXPECT_EQ(header.getRemainingLength(), 0u);
    EXPECT_EQ(header.getPacketType(), PacketType::None);
    EXPECT_EQ(header.getFlags(), 0u);
}

TEST(FixedHeader, CreateFromPacketConnect)
{
    const MockControlPacket packet(PacketType::Connect, 42);

    const FixedHeader header = FixedHeader::create(&packet);

    EXPECT_EQ(header.getPacketType(), PacketType::Connect);
    EXPECT_EQ(header.getRemainingLength(), 42u);
    EXPECT_EQ(header.getFlags(), 0x10);
}

TEST(FixedHeader, CreateFromPacketConnAck)
{
    const MockControlPacket packet(PacketType::ConnAck, 128);

    const FixedHeader header = FixedHeader::create(&packet);

    EXPECT_EQ(header.getPacketType(), PacketType::ConnAck);
    EXPECT_EQ(header.getRemainingLength(), 128u);
    EXPECT_EQ(header.getFlags(), 0x20);
}

TEST(FixedHeader, CreateFromPacketPubRel_SpecialFlags)
{
    const MockControlPacket packet(PacketType::PubRel, 2);
    const FixedHeader header = FixedHeader::create(&packet);

    EXPECT_EQ(header.getPacketType(), PacketType::PubRel);
    EXPECT_EQ(header.getFlags(), 0x62);
}

TEST(FixedHeader, CreateFromPacketSubscribe_SpecialFlags)
{
    const MockControlPacket packet(PacketType::Subscribe, 10);
    const FixedHeader header = FixedHeader::create(&packet);

    EXPECT_EQ(header.getPacketType(), PacketType::Subscribe);
    EXPECT_EQ(header.getFlags(), 0x82);
}

TEST(FixedHeader, CreateFromPacketUnsubscribe_SpecialFlags)
{
    const MockControlPacket packet(PacketType::Unsubscribe, 5);
    const FixedHeader header = FixedHeader::create(&packet);

    EXPECT_EQ(header.getPacketType(), PacketType::Unsubscribe);
    EXPECT_EQ(header.getFlags(), 0xA2);
}

TEST(FixedHeader, CreateWithCustomFlags_NoFlagsSet)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, false, QualityOfService::AtMostOnce, false);

    EXPECT_EQ(header.getPacketType(), PacketType::Publish);
    EXPECT_EQ(header.getRemainingLength(), 100u);
    EXPECT_EQ(header.getFlags(), 0x30);
}

TEST(FixedHeader, CreateWithCustomFlags_RetainSet)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, true, QualityOfService::AtMostOnce, false);

    EXPECT_EQ(header.getFlags(), 0x31);
}

TEST(FixedHeader, CreateWithCustomFlags_QoS1)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, false, QualityOfService::AtLeastOnce, false);

    EXPECT_EQ(header.getFlags(), 0x32);
}

TEST(FixedHeader, CreateWithCustomFlags_QoS2)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, false, QualityOfService::ExactlyOnce, false);

    EXPECT_EQ(header.getFlags(), 0x34);
}

TEST(FixedHeader, CreateWithCustomFlags_DupWithQoS1)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, false, QualityOfService::AtLeastOnce, true);

    EXPECT_EQ(header.getFlags(), 0x3A);
}

TEST(FixedHeader, CreateWithCustomFlags_DupIgnoredWithQoS0)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, false, QualityOfService::AtMostOnce, true);

    EXPECT_EQ(header.getFlags(), 0x30);
}

TEST(FixedHeader, CreateWithCustomFlags_AllFlagsSet)
{
    const MockControlPacket packet(PacketType::Publish, 100);
    const FixedHeader header = FixedHeader::create(&packet, 100, true, QualityOfService::ExactlyOnce, true);

    EXPECT_EQ(header.getFlags(), 0x3D);
}

TEST(FixedHeader, CreateFromNullPacket)
{
    const FixedHeader header = FixedHeader::create(nullptr);
    EXPECT_EQ(header.getRemainingLength(), 0u);
    EXPECT_EQ(header.getFlags(), 0u);
}

TEST(FixedHeader, CreateWithCustomFlagsFromNullPacket)
{
    const FixedHeader header = FixedHeader::create(nullptr, 100, true, QualityOfService::AtLeastOnce, true);
    EXPECT_EQ(header.getRemainingLength(), 0u);
    EXPECT_EQ(header.getFlags(), 0u);
}

TEST(FixedHeader, Encode_Connect)
{
    const MockControlPacket packet(PacketType::Connect, 42);
    const FixedHeader header = FixedHeader::create(&packet);
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    header.encode(writer);

    EXPECT_EQ(buffer.size(), 2u);
    EXPECT_EQ(buffer[0], std::byte{ 0x10 });
    EXPECT_EQ(buffer[1], std::byte{ 42 });
}

TEST(FixedHeader, Encode_LargeRemainingLength)
{
    const MockControlPacket packet(PacketType::Publish, 16384);
    const FixedHeader header = FixedHeader::create(&packet);

    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);
    header.encode(writer);

    EXPECT_EQ(buffer.size(), 4u);
    EXPECT_EQ(buffer[0], std::byte{ 0x30 });
    EXPECT_EQ(buffer[1], std::byte{ 0x80 });
    EXPECT_EQ(buffer[2], std::byte{ 0x80 });
    EXPECT_EQ(buffer[3], std::byte{ 0x01 });
}

TEST(FixedHeader, Decode_Connect)
{
    const auto data = toVec({ 0x10, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0,    0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::Connect);
    EXPECT_EQ(header.getRemainingLength(), 42u);
    EXPECT_EQ(header.getFlags(), 0x10);
}

TEST(FixedHeader, Decode_PubRelWithFlags)
{
    const auto data = toVec({ 0x62, 2, 0, 0 });
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::PubRel);
    EXPECT_EQ(header.getRemainingLength(), 2u);
    EXPECT_EQ(header.getFlags(), 0x62);
}

TEST(FixedHeader, Decode_PublishWithAllFlags)
{
    const auto data = toVec({ 0x3D, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0,    0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0,    0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::Publish);
    EXPECT_EQ(header.getRemainingLength(), 100u);
    EXPECT_EQ(header.getFlags(), 0x3D);
}

TEST(FixedHeader, Decode_LargeRemainingLength)
{
    std::vector<std::byte> data = toVec({ 0x30, 0x80, 0x80, 0x01 });
    for (int i = 0; i < 16384; ++i)
    {
        data.push_back(std::byte{ 0 });
    }
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::Publish);
    EXPECT_EQ(header.getRemainingLength(), 16384u);
}

TEST(FixedHeader, Decode_InvalidPacketType)
{
    const auto data = toVec({ 0xF0, 0 });
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::Auth);
    EXPECT_EQ(header.getFlags(), 0xF0u);
    EXPECT_EQ(header.getRemainingLength(), 0u);
}

TEST(FixedHeader, Decode_PacketTypeNone)
{
    const auto data = toVec({ 0x00, 0 });
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getFlags(), 0u);
    EXPECT_EQ(header.getRemainingLength(), 0u);
}

TEST(FixedHeader, Decode_EmptyBuffer)
{
    const std::vector<std::byte> data;
    ByteReader reader(data.data(), data.size());

    FixedHeader header;
    header.decode(reader);

    EXPECT_EQ(header.getFlags(), 0u);
    EXPECT_EQ(header.getRemainingLength(), 0u);
}

TEST(FixedHeader, CreateFromReader)
{
    std::vector<std::byte> data = toVec({ 0x20, 0x80, 0x01 });
    for (int i = 0; i < 128; ++i)
    {
        data.push_back(std::byte{ 0 });
    }
    ByteReader reader(data.data(), data.size());

    const FixedHeader header = FixedHeader::create(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::ConnAck);
    EXPECT_EQ(header.getRemainingLength(), 128u);
    EXPECT_EQ(header.getFlags(), 0x20);
}

TEST(FixedHeader, RoundTrip_Connect)
{
    const MockControlPacket packet(PacketType::Connect, 42);
    const FixedHeader header1 = FixedHeader::create(&packet);

    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);
    header1.encode(writer);

    for (uint32_t i = 0; i < header1.getRemainingLength(); ++i)
    {
        buffer.push_back(std::byte{ 0 });
    }

    ByteReader reader(buffer.data(), buffer.size());
    const FixedHeader header2 = FixedHeader::create(reader);

    EXPECT_EQ(header2.getPacketType(), header1.getPacketType());
    EXPECT_EQ(header2.getRemainingLength(), header1.getRemainingLength());
    EXPECT_EQ(header2.getFlags(), header1.getFlags());
}

TEST(FixedHeader, RoundTrip_PublishWithFlags)
{
    const MockControlPacket packet(PacketType::Publish, 256);
    const FixedHeader header1 = FixedHeader::create(&packet, 256, true, QualityOfService::ExactlyOnce, true);

    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);
    header1.encode(writer);

    for (uint32_t i = 0; i < header1.getRemainingLength(); ++i)
    {
        buffer.push_back(std::byte{ 0 });
    }

    ByteReader reader(buffer.data(), buffer.size());
    const FixedHeader header2 = FixedHeader::create(reader);

    EXPECT_EQ(header2.getPacketType(), header1.getPacketType());
    EXPECT_EQ(header2.getRemainingLength(), header1.getRemainingLength());
    EXPECT_EQ(header2.getFlags(), header1.getFlags());
}

TEST(FixedHeader, AllPacketTypes)
{
    PacketType types[]
        = { PacketType::Connect,  PacketType::ConnAck, PacketType::Publish,   PacketType::PubAck,     PacketType::PubRec,
            PacketType::PubRel,   PacketType::PubComp, PacketType::Subscribe, PacketType::SubAck,     PacketType::Unsubscribe,
            PacketType::UnsubAck, PacketType::PingReq, PacketType::PingResp,  PacketType::Disconnect, PacketType::Auth };

    for (auto type : types)
    {
        MockControlPacket packet(type, 10);
        FixedHeader header = FixedHeader::create(&packet);

        EXPECT_EQ(header.getPacketType(), type);
        EXPECT_EQ(header.getRemainingLength(), 10u);
    }
}
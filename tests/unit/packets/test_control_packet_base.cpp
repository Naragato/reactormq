//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

using namespace reactormq::mqtt::packets;
using namespace reactormq::serialize;

class TestConnectPacket final : public TControlPacket<PacketType::Connect>
{
public:
    TestConnectPacket() = default;

    explicit TestConnectPacket(const FixedHeader& header)
        : TControlPacket(header)
    {
    }

    [[nodiscard]] uint32_t getLength() const override
    {
        return 42;
    }

    void encode(ByteWriter&) const override
    {
        // Intentionally empty
    }

    bool decode(ByteReader&) override
    {
        return true;
    }
};

class TestConnAckPacket final : public TControlPacket<PacketType::ConnAck>
{
public:
    TestConnAckPacket() = default;

    explicit TestConnAckPacket(const FixedHeader& header)
        : TControlPacket(header)
    {
    }

    [[nodiscard]] uint32_t getLength() const override
    {
        return 10;
    }

    void encode(ByteWriter&) const override
    {
        // Intentionally empty
    }

    bool decode(ByteReader&) override
    {
        return true;
    }
};

class TestPublishPacket final : public TControlPacket<PacketType::Publish>
{
public:
    TestPublishPacket() = default;

    explicit TestPublishPacket(const FixedHeader& header)
        : TControlPacket(header)
    {
    }

    [[nodiscard]] uint32_t getLength() const override
    {
        return 100;
    }

    void encode(ByteWriter&) const override
    {
        // Intentionally empty
    }

    bool decode(ByteReader&) override
    {
        return true;
    }
};

class TestDisconnectPacket final : public TControlPacket<PacketType::Disconnect>
{
public:
    TestDisconnectPacket() = default;

    explicit TestDisconnectPacket(const FixedHeader& header)
        : TControlPacket(header)
    {
    }

    [[nodiscard]] uint32_t getLength() const override
    {
        return 0;
    }

    void encode(ByteWriter&) const override
    {
        // Intentionally empty
    }

    bool decode(ByteReader&) override
    {
        return true;
    }
};

// Test default constructor
TEST(TControlPacket, DefaultConstructor_Connect)
{
    const TestConnectPacket packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Connect);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getLength(), 42u);
}

TEST(TControlPacket, DefaultConstructor_ConnAck)
{
    const TestConnAckPacket packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::ConnAck);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getLength(), 10u);
}

TEST(TControlPacket, DefaultConstructor_Publish)
{
    const TestPublishPacket packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Publish);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getLength(), 100u);
}

TEST(TControlPacket, DefaultConstructor_Disconnect)
{
    const TestDisconnectPacket packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Disconnect);
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getLength(), 0u);
}

// Test constructor with FixedHeader
TEST(TControlPacket, ConstructorWithFixedHeader_Connect)
{
    const FixedHeader header;
    const TestConnectPacket packet(header);

    EXPECT_EQ(packet.getPacketType(), PacketType::Connect);
    EXPECT_TRUE(packet.isValid());
}

TEST(TControlPacket, ConstructorWithFixedHeader_Publish)
{
    const std::vector buffer = { std::byte{ 0x30 }, std::byte{ 100 } };
    ByteReader reader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(reader);

    const TestPublishPacket packet(header);

    EXPECT_EQ(packet.getPacketType(), PacketType::Publish);
    EXPECT_TRUE(packet.isValid());
}

// Test getPacketType for various packet types
TEST(TControlPacket, GetPacketType_AllTypes)
{
    const TestConnectPacket connect;
    EXPECT_EQ(connect.getPacketType(), PacketType::Connect);

    const TestConnAckPacket connack;
    EXPECT_EQ(connack.getPacketType(), PacketType::ConnAck);

    const TestPublishPacket publish;
    EXPECT_EQ(publish.getPacketType(), PacketType::Publish);

    const TestDisconnectPacket disconnect;
    EXPECT_EQ(disconnect.getPacketType(), PacketType::Disconnect);
}

// Test isValid
TEST(TControlPacket, IsValid_DefaultTrue)
{
    const TestPublishPacket packet;
    EXPECT_TRUE(packet.isValid());
}

// Test FixedHeader access
TEST(TControlPacket, FixedHeaderAccess)
{
    TestPublishPacket packet;

    const FixedHeader& header = packet.getFixedHeader();

    EXPECT_EQ(header.getRemainingLength(), 0u);
    EXPECT_EQ(header.getPacketType(), PacketType::None);
}

// Test static constant (kInvalidPacket is public)
TEST(TControlPacket, StaticConstants)
{
    EXPECT_STREQ(TestConnectPacket::kInvalidPacket, "Invalid packet");
    EXPECT_STREQ(TestPublishPacket::kInvalidPacket, "Invalid packet");
}

// Test polymorphism through IControlPacket interface
TEST(TControlPacket, PolymorphismThroughInterface)
{
    const TestConnectPacket connectPacket;
    const TestPublishPacket publishPacket;

    const IControlPacket* pConnect = &connectPacket;
    const IControlPacket* pPublish = &publishPacket;

    EXPECT_EQ(pConnect->getPacketType(), PacketType::Connect);
    EXPECT_EQ(pPublish->getPacketType(), PacketType::Publish);

    EXPECT_TRUE(pConnect->isValid());
    EXPECT_TRUE(pPublish->isValid());

    EXPECT_EQ(pConnect->getLength(), 42u);
    EXPECT_EQ(pPublish->getLength(), 100u);
}

// Test that different instantiations are distinct types
TEST(TControlPacket, DistinctTypes)
{
    TestConnectPacket connect;
    TestPublishPacket publish;

    EXPECT_NE(typeid(connect).name(), typeid(publish).name());
}

// Test encode/decode through base interface
TEST(TControlPacket, EncodeDecodeThroughInterface)
{
    TestPublishPacket packet;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);

    packet.encode(writer);

    ByteReader reader(buffer.data(), buffer.size());
    const bool result = packet.decode(reader);
    EXPECT_TRUE(result);
}

TEST(TControlPacket, WithMeaningfulFixedHeader)
{
    std::vector buffer = { std::byte{ 0x10 }, std::byte{ 42 } };
    for (int i = 0; i < 42; ++i)
    {
        buffer.push_back(std::byte{ 0 });
    }

    ByteReader reader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(reader);

    EXPECT_EQ(header.getPacketType(), PacketType::Connect);
    EXPECT_EQ(header.getRemainingLength(), 42u);

    const TestConnectPacket packet(header);
    EXPECT_EQ(packet.getPacketType(), PacketType::Connect);
    EXPECT_TRUE(packet.isValid());
}
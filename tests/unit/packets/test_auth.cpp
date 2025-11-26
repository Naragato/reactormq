//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

#include "mqtt/packets/auth.h"
#include "mqtt/packets/fixed_header.h"
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

TEST(Auth, Constructor_Success)
{
    const Auth packet(ReasonCode::Success);

    EXPECT_EQ(packet.getPacketType(), PacketType::Auth);
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
    EXPECT_EQ(packet.getLength(), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Auth, Constructor_ContinueAuthentication)
{
    const Auth packet(ReasonCode::ContinueAuthentication);

    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ContinueAuthentication);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Auth, Constructor_ReAuthenticate)
{
    const Auth packet(ReasonCode::ReAuthenticate);

    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ReAuthenticate);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Auth, Constructor_WithProperties)
{
    const Properties props;
    const Auth packet(ReasonCode::ContinueAuthentication, props);

    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ContinueAuthentication);
    EXPECT_EQ(packet.getProperties(), props);
}

TEST(Auth, Constructor_InvalidReasonCode)
{
    const Auth packet(ReasonCode::ServerBusy);

    EXPECT_FALSE(packet.isValid());
}

TEST(Auth, GetLength_SuccessOptimization)
{
    const Auth packet(ReasonCode::Success);

    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(Auth, GetLength_NonSuccessReasonCode)
{
    const Auth packet(ReasonCode::ContinueAuthentication);

    EXPECT_EQ(packet.getLength(), 2u);
}

TEST(Auth, Encode_SuccessOptimization)
{
    const Auth packet(ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xF0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);
}

TEST(Auth, Encode_WithReasonCode)
{
    const Auth packet(ReasonCode::ContinueAuthentication);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xF0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x18);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(Auth, Encode_ReAuthenticate)
{
    const Auth packet(ReasonCode::ReAuthenticate);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x19);
}

TEST(Auth, Decode_SuccessOptimization)
{
    const auto data = toVec({ 0xF0, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    const std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    const Auth packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
}

TEST(Auth, Decode_WithReasonCode)
{
    const auto data = toVec({ 0xF0, 0x02, 0x18, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const Auth packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ContinueAuthentication);
}

TEST(Auth, Decode_ReAuthenticate)
{
    const auto data = toVec({ 0xF0, 0x02, 0x19, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const Auth packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ReAuthenticate);
}

TEST(Auth, Decode_InvalidReasonCode)
{
    const auto data = toVec({ 0xF0, 0x02, 0x87, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const Auth packet(payloadReader, header);

    EXPECT_FALSE(packet.isValid());
}

TEST(Auth, RoundTrip_Success)
{
    const Auth packet1(ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    const Auth packet2(payloadReader, header);

    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Auth, RoundTrip_WithReasonCode)
{
    const Auth packet1(ReasonCode::ContinueAuthentication);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    constexpr size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    const Auth packet2(payloadReader, header);

    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Auth, IsReasonCodeValid_ValidCodes)
{
    ReasonCode validCodes[] = { ReasonCode::Success, ReasonCode::ContinueAuthentication, ReasonCode::ReAuthenticate };

    for (auto code : validCodes)
    {
        EXPECT_TRUE(Auth::isReasonCodeValid(code)) << "Code " << static_cast<int>(code) << " should be valid";
    }
}

TEST(Auth, IsReasonCodeValid_InvalidCodes)
{
    ReasonCode invalidCodes[]
        = { ReasonCode::UnspecifiedError,
            ReasonCode::NotAuthorized,
            ReasonCode::ServerBusy,
            ReasonCode::MalformedPacket,
            ReasonCode::GrantedQualityOfService1 };

    for (auto code : invalidCodes)
    {
        EXPECT_FALSE(Auth::isReasonCodeValid(code)) << "Code " << static_cast<int>(code) << " should be invalid";
    }
}

TEST(Auth, WireFormat_MatchesSpec)
{
    const Auth packet(ReasonCode::ContinueAuthentication);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xF0) << "Byte 0 should be 0xF0 (AUTH type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02) << "Byte 1 should be 0x02 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x18) << "Byte 2 should be 0x18 (ContinueAuthentication)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00) << "Byte 3 should be 0x00 (properties length)";
}

TEST(Auth, ArePropertiesValid_EmptyProperties)
{
    const Properties props;
    EXPECT_TRUE(Auth::arePropertiesValid(props));
}

// MQTT 5 AUTH: Success optimization should ignore any provided properties
TEST(AuthSuccessOptimization, Encode_IgnoresPropertiesWhenSuccess)
{
    std::vector<Property> propsList;
    propsList.emplace_back(
        Property::create<PropertyIdentifier::UserProperty, std::pair<std::string, std::string>>(
            std::make_pair(std::string("k"), std::string("v"))));
    Properties props(propsList);

    Auth packet(ReasonCode::Success, props);

    EXPECT_EQ(packet.getLength(), 0u);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xF0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    Auth decoded(payloadReader, header);

    EXPECT_TRUE(decoded.isValid());
    EXPECT_EQ(decoded.getReasonCode(), ReasonCode::Success);
    EXPECT_EQ(decoded.getProperties().getLength(false), 0u);
}
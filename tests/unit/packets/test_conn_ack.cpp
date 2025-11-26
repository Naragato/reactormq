//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/conn_ack.h"
#include "mqtt/packets/fixed_header.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"

#include <array>
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

TEST(ConnAck3, Constructor_AcceptedNoSession)
{
    const ConnAck3 packet(false, ConnectReturnCode::Accepted);

    EXPECT_EQ(packet.getPacketType(), PacketType::ConnAck);
    EXPECT_FALSE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::Accepted);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(ConnAck3, Constructor_AcceptedWithSession)
{
    const ConnAck3 packet(true, ConnectReturnCode::Accepted);

    EXPECT_TRUE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::Accepted);
}

TEST(ConnAck3, Constructor_RefusedProtocolVersion)
{
    const ConnAck3 packet(false, ConnectReturnCode::RefusedProtocolVersion);

    EXPECT_FALSE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::RefusedProtocolVersion);
}

TEST(ConnAck3, Constructor_RefusedNotAuthorized)
{
    const ConnAck3 packet(false, ConnectReturnCode::RefusedNotAuthorized);

    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::RefusedNotAuthorized);
}

TEST(ConnAck3, GetLength_AlwaysTwo)
{
    const ConnAck3 packet1(false, ConnectReturnCode::Accepted);
    const ConnAck3 packet2(true, ConnectReturnCode::RefusedNotAuthorized);

    EXPECT_EQ(packet1.getLength(), 2u);
    EXPECT_EQ(packet2.getLength(), 2u);
}

TEST(ConnAck3, Encode_AcceptedNoSession)
{
    const ConnAck3 packet(false, ConnectReturnCode::Accepted);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(ConnAck3, Encode_AcceptedWithSession)
{
    const ConnAck3 packet(true, ConnectReturnCode::Accepted);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(ConnAck3, Encode_RefusedProtocolVersion)
{
    const ConnAck3 packet(false, ConnectReturnCode::RefusedProtocolVersion);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x01);
}

TEST(ConnAck3, Encode_RefusedNotAuthorized)
{
    const ConnAck3 packet(false, ConnectReturnCode::RefusedNotAuthorized);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x05);
}

TEST(ConnAck3, Decode_AcceptedNoSession)
{
    const auto data = toVec({ 0x20, 0x02, 0x00, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const ConnAck3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_FALSE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::Accepted);
}

TEST(ConnAck3, Decode_AcceptedWithSession)
{
    const auto data = toVec({ 0x20, 0x02, 0x01, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const ConnAck3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_TRUE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::Accepted);
}

TEST(ConnAck3, Decode_RefusedIdentifierRejected)
{
    const auto data = toVec({ 0x20, 0x02, 0x00, 0x02 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const ConnAck3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ConnectReturnCode::RefusedIdentifierRejected);
}

TEST(ConnAck3, Decode_InvalidReturnCode)
{
    const auto data = toVec({ 0x20, 0x02, 0x00, 0xFF });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const ConnAck3 packet(payloadReader, header);

    EXPECT_FALSE(packet.isValid());
}

TEST(ConnAck3, RoundTrip_AcceptedNoSession)
{
    const ConnAck3 packet1(false, ConnectReturnCode::Accepted);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const ConnAck3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getSessionPresent(), packet1.getSessionPresent());
    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(ConnAck3, RoundTrip_AcceptedWithSession)
{
    const ConnAck3 packet1(true, ConnectReturnCode::Accepted);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const ConnAck3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getSessionPresent(), packet1.getSessionPresent());
    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(ConnAck3, RoundTrip_AllReturnCodes)
{
    using enum ConnectReturnCode;
    constexpr std::array codes
        = { Accepted,
            RefusedProtocolVersion,
            RefusedIdentifierRejected,
            RefusedServerUnavailable,
            RefusedBadUserNameOrPassword,
            RefusedNotAuthorized };

    for (auto code : codes)
    {
        ConnAck3 packet1(false, code);

        std::vector<std::byte> buffer;
        ByteWriter writer(buffer);
        packet1.encode(writer);

        ByteReader headerReader(buffer.data(), buffer.size());
        FixedHeader header = FixedHeader::create(headerReader);

        ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
        ConnAck3 packet2(payloadReader, header);

        EXPECT_EQ(packet2.getReasonCode(), code);
        EXPECT_TRUE(packet2.isValid());
    }
}

TEST(ConnAck5, Constructor_AcceptedNoSession)
{
    const ConnAck5 packet(false, ReasonCode::Success);

    EXPECT_EQ(packet.getPacketType(), PacketType::ConnAck);
    EXPECT_FALSE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
    EXPECT_EQ(packet.getProperties().getLength(false), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(ConnAck5, Constructor_AcceptedWithSession)
{
    const ConnAck5 packet(true, ReasonCode::Success);

    EXPECT_TRUE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
}

TEST(ConnAck5, Constructor_WithProperties)
{
    // MQTT v5 User Property “test”=“test” encodes to 13 bytes: 1 (identifier)
    // + 2+4 (key length + “test”) + 2+4 (value length + “test”).
    const ConnAck5 packet(
        false,
        ReasonCode::Success,
        Properties{ { Property::create<PropertyIdentifier::UserProperty, std::pair<std::string, std::string>>(
            std::make_pair(std::string("test"), std::string("test"))) } });
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
    EXPECT_EQ(packet.getProperties().getLength(false), 13u);
    EXPECT_EQ(packet.getProperties().getLength(true), 14u);
}

TEST(ConnAck5, Constructor_InvalidReasonCode)
{
    const ConnAck5 packet(false, ReasonCode::GrantedQualityOfService1);

    EXPECT_FALSE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::UnspecifiedError);
}

TEST(ConnAck5, GetLength_MinimalPacket)
{
    const ConnAck5 packet(false, ReasonCode::Success);

    EXPECT_EQ(packet.getLength(), 3u);
}

TEST(ConnAck5, Encode_AcceptedNoSession)
{
    const ConnAck5 packet(false, ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x03);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00);
}

TEST(ConnAck5, Encode_AcceptedWithSession)
{
    const ConnAck5 packet(true, ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(ConnAck5, Encode_ServerUnavailable)
{
    const ConnAck5 packet(false, ReasonCode::ServerUnavailable);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x88);
}

TEST(ConnAck5, Decode_AcceptedNoSession)
{
    const auto data = toVec({ 0x20, 0x03, 0x00, 0x00, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 3);
    const ConnAck5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_FALSE(packet.getSessionPresent());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
}

TEST(ConnAck5, Decode_AcceptedWithSession)
{
    const auto data = toVec({ 0x20, 0x03, 0x01, 0x00, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 3);
    const ConnAck5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_TRUE(packet.getSessionPresent());
}

TEST(ConnAck5, Decode_NotAuthorized)
{
    const auto data = toVec({ 0x20, 0x03, 0x00, 0x87, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 3);
    const ConnAck5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::NotAuthorized);
}

TEST(ConnAck5, Decode_InvalidReasonCode)
{
    const auto data = toVec({ 0x20, 0x03, 0x00, 0x01, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 3);
    const ConnAck5 packet(payloadReader, header);

    EXPECT_FALSE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::UnspecifiedError);
}

TEST(ConnAck5, RoundTrip_AcceptedNoSession)
{
    const ConnAck5 packet1(false, ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const ConnAck5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getSessionPresent(), packet1.getSessionPresent());
    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(ConnAck5, RoundTrip_AcceptedWithSession)
{
    const ConnAck5 packet1(true, ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const ConnAck5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getSessionPresent(), packet1.getSessionPresent());
    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(ConnAck5, IsReasonCodeValid_ValidCodes)
{
    ReasonCode validCodes[] = {
        ReasonCode::Success,
        ReasonCode::UnspecifiedError,
        ReasonCode::MalformedPacket,
        ReasonCode::ProtocolError,
        ReasonCode::ImplementationSpecificError,
        ReasonCode::UnsupportedProtocolVersion,
        ReasonCode::ClientIdentifierNotValid,
        ReasonCode::BadUsernameOrPassword,
        ReasonCode::NotAuthorized,
        ReasonCode::ServerUnavailable,
        ReasonCode::ServerBusy,
        ReasonCode::Banned,
        ReasonCode::BadAuthenticationMethod,
        ReasonCode::TopicNameInvalid,
        ReasonCode::PacketTooLarge,
        ReasonCode::QuotaExceeded,
        ReasonCode::PayloadFormatInvalid,
        ReasonCode::RetainNotSupported,
        ReasonCode::QoSNotSupported,
        ReasonCode::UseAnotherServer,
        ReasonCode::ServerMoved,
        ReasonCode::ConnectionRateExceeded
    };

    for (auto code : validCodes)
    {
        EXPECT_TRUE(ConnAck5::isReasonCodeValid(code)) << "Code " << static_cast<int>(code) << " should be valid";
    }
}

TEST(ConnAck5, IsReasonCodeValid_InvalidCodes)
{
    ReasonCode invalidCodes[]
        = { ReasonCode::GrantedQualityOfService1,
            ReasonCode::GrantedQualityOfService2,
            ReasonCode::NoMatchingSubscribers,
            ReasonCode::ContinueAuthentication,
            ReasonCode::PacketIdentifierInUse };

    for (auto code : invalidCodes)
    {
        EXPECT_FALSE(ConnAck5::isReasonCodeValid(code)) << "Code " << static_cast<int>(code) << " should be invalid";
    }
}

TEST(ConnAck5, WireFormat_MatchesSpec)
{
    const ConnAck5 packet(true, ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x20) << "Byte 0 should be 0x20 (CONNACK type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x03) << "Byte 1 should be 0x03 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01) << "Byte 2 should be 0x01 (session present)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00) << "Byte 3 should be 0x00 (Success)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[4]), 0x00) << "Byte 4 should be 0x00 (properties length)";
}

// ConnAck (MQTT 5) with properties round-trip
TEST(ConnAck5Properties, EncodeDecode_UserProperty)
{
    std::vector<Property> propsList;
    propsList.emplace_back(
        Property::create<PropertyIdentifier::UserProperty, std::pair<std::string, std::string>>(
            std::make_pair(std::string("test"), std::string("test"))));
    const Properties props(propsList);
    const ConnAck5 packet(true, ReasonCode::Banned, Properties{ propsList });

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ByteReader headerReader(buffer.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const ConnAck5 decoded(payloadReader, header);

    EXPECT_TRUE(decoded.isValid());
    EXPECT_TRUE(decoded.getSessionPresent());
    EXPECT_EQ(decoded.getReasonCode(), ReasonCode::Banned);
    EXPECT_EQ(decoded.getProperties(), props);
}
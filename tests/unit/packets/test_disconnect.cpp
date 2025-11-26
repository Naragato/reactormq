//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/disconnect.h"
#include "mqtt/packets/fixed_header.h"
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

TEST(Disconnect3, Constructor_Default)
{
    const Disconnect3 packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Disconnect);
    EXPECT_EQ(packet.getLength(), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Disconnect3, GetLength_AlwaysZero)
{
    const Disconnect3 packet;

    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(Disconnect3, Encode_MinimalPacket)
{
    const Disconnect3 packet;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xE0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);
}

TEST(Disconnect3, Decode_MinimalPacket)
{
    const auto data = toVec({ 0xE0, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    const std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    const Disconnect3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getPacketType(), PacketType::Disconnect);
}

TEST(Disconnect3, RoundTrip_Encode_Decode)
{
    const Disconnect3 packet1;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    const Disconnect3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getPacketType(), packet1.getPacketType());
    EXPECT_EQ(packet2.getLength(), packet1.getLength());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Disconnect5, Constructor_DefaultSuccess)
{
    const Disconnect5 packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Disconnect);
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::NormalDisconnection);
    EXPECT_EQ(packet.getLength(), 0u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Disconnect5, Constructor_WithReasonCode)
{
    const Disconnect5 packet(ReasonCode::ServerBusy);

    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ServerBusy);
    EXPECT_EQ(packet.getLength(), 2u);
    EXPECT_TRUE(packet.isValid());
}

TEST(Disconnect5, Constructor_WithProperties)
{
    const Properties props;
    const Disconnect5 packet(ReasonCode::ServerShuttingDown, {});

    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ServerShuttingDown);
    EXPECT_EQ(packet.getProperties(), props);
}

TEST(Disconnect5, Constructor_InvalidReasonCode)
{
    const Disconnect5 packet(ReasonCode::GrantedQualityOfService1);

    EXPECT_FALSE(packet.isValid());
}

TEST(Disconnect5, GetLength_SuccessOptimization)
{
    const Disconnect5 packet(ReasonCode::Success);

    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(Disconnect5, GetLength_NormalDisconnection)
{
    const Disconnect5 packet(ReasonCode::NormalDisconnection);

    EXPECT_EQ(packet.getLength(), 0u);
}

TEST(Disconnect5, GetLength_NonSuccessReasonCode)
{
    const Disconnect5 packet(ReasonCode::ServerBusy);

    EXPECT_EQ(packet.getLength(), 2u);
}

TEST(Disconnect5, Encode_SuccessOptimization)
{
    const Disconnect5 packet(ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xE0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);
}

TEST(Disconnect5, Encode_WithReasonCode)
{
    const Disconnect5 packet(ReasonCode::ServerBusy);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xE0);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x89);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00);
}

TEST(Disconnect5, Decode_SuccessOptimization)
{
    const auto data = toVec({ 0xE0, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    const std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    const Disconnect5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::Success);
}

TEST(Disconnect5, Decode_WithReasonCode)
{
    const auto data = toVec({ 0xE0, 0x02, 0x8B, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const Disconnect5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::ServerShuttingDown);
}

TEST(Disconnect5, Decode_ReasonCodeOnly)
{
    const auto data = toVec({ 0xE0, 0x01, 0x8D });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 1);
    const Disconnect5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getReasonCode(), ReasonCode::KeepAliveTimeout);
}

TEST(Disconnect5, Decode_InvalidReasonCode)
{
    const auto data = toVec({ 0xE0, 0x02, 0x01, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, 2);
    const Disconnect5 packet(payloadReader, header);

    EXPECT_FALSE(packet.isValid());
}

TEST(Disconnect5, RoundTrip_Success)
{
    const Disconnect5 packet1(ReasonCode::Success);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    const std::vector<std::byte> empty;
    ByteReader payloadReader(empty.data(), 0);
    const Disconnect5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Disconnect5, RoundTrip_WithReasonCode)
{
    const Disconnect5 packet1(ReasonCode::NotAuthorized);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    const FixedHeader header = FixedHeader::create(headerReader);

    constexpr size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    const Disconnect5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getReasonCode(), packet1.getReasonCode());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Disconnect5, IsReasonCodeValid_ValidCodes)
{
    ReasonCode validCodes[] = {
        ReasonCode::NormalDisconnection,
        ReasonCode::DisconnectWithWillMessage,
        ReasonCode::UnspecifiedError,
        ReasonCode::MalformedPacket,
        ReasonCode::ProtocolError,
        ReasonCode::ImplementationSpecificError,
        ReasonCode::NotAuthorized,
        ReasonCode::ServerBusy,
        ReasonCode::ServerShuttingDown,
        ReasonCode::KeepAliveTimeout,
        ReasonCode::SessionTakenOver,
        ReasonCode::TopicFilterInvalid,
        ReasonCode::TopicNameInvalid,
        ReasonCode::ReceiveMaximumExceeded,
        ReasonCode::TopicAliasInvalid,
        ReasonCode::PacketTooLarge,
        ReasonCode::MessageRateTooHigh,
        ReasonCode::QuotaExceeded,
        ReasonCode::AdministrativeAction,
        ReasonCode::PayloadFormatInvalid,
        ReasonCode::RetainNotSupported,
        ReasonCode::QoSNotSupported,
        ReasonCode::UseAnotherServer,
        ReasonCode::ServerMoved,
        ReasonCode::SharedSubscriptionsNotSupported,
        ReasonCode::ConnectionRateExceeded,
        ReasonCode::MaximumConnectTime,
        ReasonCode::SubscriptionIdentifiersNotSupported,
        ReasonCode::WildcardSubscriptionsNotSupported
    };

    for (auto code : validCodes)
    {
        EXPECT_TRUE(Disconnect5::isReasonCodeValid(code)) << "Code " << static_cast<int>(code) << " should be valid";
    }
}

TEST(Disconnect5, IsReasonCodeValid_InvalidCodes)
{
    ReasonCode invalidCodes[]
        = { ReasonCode::GrantedQualityOfService1,
            ReasonCode::GrantedQualityOfService2,
            ReasonCode::NoMatchingSubscribers,
            ReasonCode::ContinueAuthentication,
            ReasonCode::ReAuthenticate };

    for (auto code : invalidCodes)
    {
        EXPECT_FALSE(Disconnect5::isReasonCodeValid(code)) << "Code " << static_cast<int>(code) << " should be invalid";
    }
}

TEST(Disconnect5, WireFormat_MatchesSpec)
{
    const Disconnect5 packet(ReasonCode::ServerBusy);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xE0) << "Byte 0 should be 0xE0 (DISCONNECT type)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x02) << "Byte 1 should be 0x02 (remaining length)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x89) << "Byte 2 should be 0x89 (ServerBusy)";
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x00) << "Byte 3 should be 0x00 (properties length)";
}

// MQTT 5 DISCONNECT with properties: encode/decode round-trip
TEST(Disconnect5Properties, EncodeDecode_UserProperty)
{
    std::vector<Property> propsList;
    propsList.emplace_back(
        Property::create<PropertyIdentifier::UserProperty, std::pair<std::string, std::string>>(
            std::make_pair(std::string("test"), std::string("test"))));
    const Properties props(propsList);

    const Disconnect5 packet(ReasonCode::ServerBusy, Properties{ propsList });

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ByteReader headerReader(buffer.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    const Disconnect5 decoded(payloadReader, header);

    EXPECT_TRUE(decoded.isValid());
    EXPECT_EQ(decoded.getReasonCode(), ReasonCode::ServerBusy);
    EXPECT_EQ(decoded.getProperties(), props);
}
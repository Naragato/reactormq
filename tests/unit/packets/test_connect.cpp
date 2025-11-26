//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/connect.h"
#include "mqtt/packets/fixed_header.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/reason_code.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <string>
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

TEST(Connect3, Constructor_DefaultParameters)
{
    Connect3 packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Connect);
    EXPECT_EQ(packet.getClientId(), "");
    EXPECT_EQ(packet.getKeepAliveSeconds(), 60);
    EXPECT_EQ(packet.getUsername(), "");
    EXPECT_EQ(packet.getPassword(), "");
    EXPECT_FALSE(packet.getCleanSession());
    EXPECT_FALSE(packet.getRetainWill());
    EXPECT_EQ(packet.getWillTopic(), "");
    EXPECT_EQ(packet.getWillMessage(), "");
    EXPECT_EQ(packet.getWillQos(), QualityOfService::AtMostOnce);
    EXPECT_TRUE(packet.isValid());
}

TEST(Connect3, Constructor_WithClientId)
{
    const Connect3 packet("TestClient");

    EXPECT_EQ(packet.getPacketType(), PacketType::Connect);
    EXPECT_EQ(packet.getClientId(), "TestClient");
    EXPECT_EQ(packet.getKeepAliveSeconds(), 60);
    EXPECT_TRUE(packet.isValid());
}

TEST(Connect3, Constructor_WithAllParameters)
{
    const Connect3 packet("MyClient", 120, "myuser", "mypass", true, false, "", "", QualityOfService::AtMostOnce);

    EXPECT_EQ(packet.getClientId(), "MyClient");
    EXPECT_EQ(packet.getKeepAliveSeconds(), 120);
    EXPECT_EQ(packet.getUsername(), "myuser");
    EXPECT_EQ(packet.getPassword(), "mypass");
    EXPECT_TRUE(packet.getCleanSession());
    EXPECT_FALSE(packet.getRetainWill());
}

TEST(Connect3, Constructor_WithWill)
{
    const Connect3 packet("Client1", 60, "", "", false, true, "will/topic", "Client disconnected", QualityOfService::AtLeastOnce);

    EXPECT_EQ(packet.getWillTopic(), "will/topic");
    EXPECT_EQ(packet.getWillMessage(), "Client disconnected");
    EXPECT_TRUE(packet.getRetainWill());
    EXPECT_EQ(packet.getWillQos(), QualityOfService::AtLeastOnce);
}

TEST(Connect3, GetLength_EmptyClientId)
{
    const Connect3 packet("", 60);

    EXPECT_EQ(packet.getLength(), 12u);
}

TEST(Connect3, GetLength_WithClientId)
{
    const Connect3 packet("TestClient", 60);

    EXPECT_EQ(packet.getLength(), 22u);
}

TEST(Connect3, GetLength_WithCredentials)
{
    const Connect3 packet("Client1", 60, "user", "pass", false, false, "", "", QualityOfService::AtMostOnce);

    EXPECT_EQ(packet.getLength(), 31u);
}

TEST(Connect3, GetLength_WithWill)
{
    const Connect3 packet("Client1", 60, "", "", false, false, "topic", "message", QualityOfService::AtMostOnce);

    EXPECT_EQ(packet.getLength(), 35u);
}

TEST(Connect3, Encode_MinimalPacket)
{
    Connect3 packet("", 60, "", "", false, false, "", "", QualityOfService::AtMostOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 14u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x0C);

    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x04);
    EXPECT_EQ(static_cast<char>(buffer[4]), 'M');
    EXPECT_EQ(static_cast<char>(buffer[5]), 'Q');
    EXPECT_EQ(static_cast<char>(buffer[6]), 'T');
    EXPECT_EQ(static_cast<char>(buffer[7]), 'T');

    EXPECT_EQ(static_cast<uint8_t>(buffer[8]), 0x04);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0x00);

    EXPECT_EQ(static_cast<uint8_t>(buffer[10]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[11]), 0x3C);

    EXPECT_EQ(static_cast<uint8_t>(buffer[12]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[13]), 0x00);
}

TEST(Connect3, Encode_WithClientId)
{
    const Connect3 packet("Test", 60);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    constexpr size_t clientIdOffset = 12;
    EXPECT_EQ(static_cast<uint8_t>(buffer[clientIdOffset]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[clientIdOffset + 1]), 0x04);
    EXPECT_EQ(static_cast<char>(buffer[clientIdOffset + 2]), 'T');
    EXPECT_EQ(static_cast<char>(buffer[clientIdOffset + 3]), 'e');
    EXPECT_EQ(static_cast<char>(buffer[clientIdOffset + 4]), 's');
    EXPECT_EQ(static_cast<char>(buffer[clientIdOffset + 5]), 't');
}

TEST(Connect3, Encode_WithCleanSession)
{
    const Connect3 packet("", 60, "", "", true, false, "", "", QualityOfService::AtMostOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0x02);
}

TEST(Connect3, Encode_WithUsername)
{
    const Connect3 packet("", 60, "user", "", false, false, "", "", QualityOfService::AtMostOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0x80);
}

TEST(Connect3, Encode_WithPassword)
{
    const Connect3 packet("", 60, "", "pass", false, false, "", "", QualityOfService::AtMostOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0x40);
}

TEST(Connect3, Encode_WithUsernameAndPassword)
{
    const Connect3 packet("", 60, "user", "pass", false, false, "", "", QualityOfService::AtMostOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0xC0);
}

TEST(Connect3, Encode_WithWill)
{
    const Connect3 packet("", 60, "", "", false, true, "topic", "message", QualityOfService::ExactlyOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0x34);
}

TEST(Connect3, Decode_MinimalPacket)
{
    const auto data = toVec({ 0x10, 0x0C, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x00, 0x00, 0x3C, 0x00, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Connect3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getClientId(), "");
    EXPECT_EQ(packet.getKeepAliveSeconds(), 60);
    EXPECT_FALSE(packet.getCleanSession());
}

TEST(Connect3, Decode_WithClientId)
{
    const auto data = toVec({ 0x10, 0x10, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x00, 0x00, 0x3C, 0x00, 0x04, 'T', 'e', 's', 't' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Connect3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getClientId(), "Test");
}

TEST(Connect3, Decode_WithCleanSession)
{
    const auto data = toVec({ 0x10, 0x0C, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x3C, 0x00, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Connect3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_TRUE(packet.getCleanSession());
}

TEST(Connect3, Decode_WithCredentials)
{
    const auto data = toVec({ 0x10, 0x18, 0x00, 0x04, 'M', 'Q', 'T', 'T',  0x04, 0xC0, 0x00, 0x3C, 0x00,
                              0x00, 0x00, 0x04, 'u',  's', 'e', 'r', 0x00, 0x04, 'p',  'a',  's',  's' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Connect3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getUsername(), "user");
    EXPECT_EQ(packet.getPassword(), "pass");
}

TEST(Connect3, Decode_WithWill)
{
    const auto data = toVec({ 0x10, 0x21, 0x00, 0x04, 'M', 'Q', 'T',  'T',  0x04, 0x0C, 0x00, 0x3C, 0x00, 0x00, 0x00,
                              0x05, 't',  'o',  'p',  'i', 'c', 0x00, 0x07, 'm',  'e',  's',  's',  'a',  'g',  'e' });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Connect3 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getWillTopic(), "topic");
    EXPECT_EQ(packet.getWillMessage(), "message");
    EXPECT_EQ(packet.getWillQos(), QualityOfService::AtLeastOnce);
}

TEST(Connect3, RoundTrip_MinimalPacket)
{
    Connect3 packet1("TestClient", 120);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Connect3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getClientId(), packet1.getClientId());
    EXPECT_EQ(packet2.getKeepAliveSeconds(), packet1.getKeepAliveSeconds());
    EXPECT_EQ(packet2.getCleanSession(), packet1.getCleanSession());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Connect3, RoundTrip_WithAllFeatures)
{
    Connect3 packet1("FullClient", 90, "admin", "secret", true, true, "lwt/topic", "offline", QualityOfService::ExactlyOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Connect3 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getClientId(), packet1.getClientId());
    EXPECT_EQ(packet2.getKeepAliveSeconds(), packet1.getKeepAliveSeconds());
    EXPECT_EQ(packet2.getUsername(), packet1.getUsername());
    EXPECT_EQ(packet2.getPassword(), packet1.getPassword());
    EXPECT_EQ(packet2.getCleanSession(), packet1.getCleanSession());
    EXPECT_EQ(packet2.getWillTopic(), packet1.getWillTopic());
    EXPECT_EQ(packet2.getWillMessage(), packet1.getWillMessage());
    EXPECT_EQ(packet2.getRetainWill(), packet1.getRetainWill());
    EXPECT_EQ(packet2.getWillQos(), packet1.getWillQos());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Connect5, Constructor_DefaultParameters)
{
    const Connect5 packet;

    EXPECT_EQ(packet.getPacketType(), PacketType::Connect);
    EXPECT_EQ(packet.getClientId(), "");
    EXPECT_EQ(packet.getKeepAliveSeconds(), 60);
    EXPECT_FALSE(packet.getCleanSession());
    EXPECT_TRUE(packet.isValid());
}

TEST(Connect5, Constructor_WithProperties)
{
    const Properties props;
    const Properties willProps;

    const Connect5 packet("Client5", 60, "", "", false, false, "", "", QualityOfService::AtMostOnce, willProps, props);

    EXPECT_EQ(packet.getClientId(), "Client5");
    EXPECT_EQ(packet.getProperties(), props);
    EXPECT_EQ(packet.getWillProperties(), willProps);
}

TEST(Connect5, GetLength_MinimalPacket)
{
    const Connect5 packet("", 60);

    EXPECT_EQ(packet.getLength(), 13u);
}

TEST(Connect5, GetLength_WithClientId)
{
    const Connect5 packet("Test", 60);

    EXPECT_EQ(packet.getLength(), 17u);
}

TEST(Connect5, Encode_MinimalPacket)
{
    Connect5 packet("", 60);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ASSERT_GE(buffer.size(), 15u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x0D);

    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x04);
    EXPECT_EQ(static_cast<char>(buffer[4]), 'M');
    EXPECT_EQ(static_cast<char>(buffer[5]), 'Q');
    EXPECT_EQ(static_cast<char>(buffer[6]), 'T');
    EXPECT_EQ(static_cast<char>(buffer[7]), 'T');

    EXPECT_EQ(static_cast<uint8_t>(buffer[8]), 0x05);

    EXPECT_EQ(static_cast<uint8_t>(buffer[9]), 0x00);

    EXPECT_EQ(static_cast<uint8_t>(buffer[10]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[11]), 0x3C);

    EXPECT_EQ(static_cast<uint8_t>(buffer[12]), 0x00);

    EXPECT_EQ(static_cast<uint8_t>(buffer[13]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[14]), 0x00);
}

TEST(Connect5, Decode_MinimalPacket)
{
    const auto data = toVec({ 0x10, 0x0D, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x05, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00 });

    ByteReader headerReader(data.data(), 2);
    const FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(data.data() + 2, data.size() - 2);
    const Connect5 packet(payloadReader, header);

    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getClientId(), "");
    EXPECT_EQ(packet.getKeepAliveSeconds(), 60);
}

TEST(Connect5, RoundTrip_MinimalPacket)
{
    Connect5 packet1("Client5", 90);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Connect5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getClientId(), packet1.getClientId());
    EXPECT_EQ(packet2.getKeepAliveSeconds(), packet1.getKeepAliveSeconds());
    EXPECT_TRUE(packet2.isValid());
}

TEST(Connect5, RoundTrip_WithCredentials)
{
    Connect5 packet1("Client5", 120, "user5", "pass5", true, false, "", "", QualityOfService::AtMostOnce);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet1.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    size_t headerSize = 2;
    ByteReader payloadReader(buffer.data() + headerSize, buffer.size() - headerSize);
    Connect5 packet2(payloadReader, header);

    EXPECT_EQ(packet2.getClientId(), packet1.getClientId());
    EXPECT_EQ(packet2.getKeepAliveSeconds(), packet1.getKeepAliveSeconds());
    EXPECT_EQ(packet2.getUsername(), packet1.getUsername());
    EXPECT_EQ(packet2.getPassword(), packet1.getPassword());
    EXPECT_EQ(packet2.getCleanSession(), packet1.getCleanSession());
    EXPECT_TRUE(packet2.isValid());
}

// Ensures MQTT 5 CONNECT encodes/decodes AuthenticationMethod and AuthenticationData properties.
TEST(Connect5AuthProps, EncodeDecode_AuthenticationMethodAndData)
{
    const std::string clientId = "client-1";
    constexpr uint16_t keepAlive = 30;
    const std::string username = "user";
    const std::string password = "pass";

    std::vector<Property> propList;
    propList.emplace_back(Property::create<PropertyIdentifier::AuthenticationMethod, std::string>("SCRAM-SHA-1"));
    propList
        .emplace_back(Property::create<PropertyIdentifier::AuthenticationData, std::vector<uint8_t>>(std::vector<uint8_t>{ 1, 2, 3, 4 }));
    Properties connectProps(propList);

    Connect5 packet(
        clientId,
        keepAlive,
        username,
        password,
        false,
        false,
        std::string{},
        std::string{},
        QualityOfService::AtMostOnce,
        Properties{},
        connectProps);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    packet.encode(writer);

    ByteReader headerReader(buffer.data(), buffer.size());
    FixedHeader header = FixedHeader::create(headerReader);

    ByteReader payloadReader(buffer.data() + 2, buffer.size() - 2);
    Connect5 decoded(payloadReader, header);

    ASSERT_TRUE(decoded.isValid());

    const Properties& decodedProps = decoded.getProperties();
    const std::vector<Property> props = decodedProps.getProperties();

    bool foundMethod = false;
    bool foundData = false;

    for (const Property& p : props)
    {
        if (p.getIdentifier() == PropertyIdentifier::AuthenticationMethod)
        {
            std::string value;
            EXPECT_TRUE(p.tryGetValue(value));
            EXPECT_EQ(value, std::string("SCRAM-SHA-1"));
            foundMethod = true;
        }
        else if (p.getIdentifier() == PropertyIdentifier::AuthenticationData)
        {
            std::vector<uint8_t> value;
            EXPECT_TRUE(p.tryGetValue(value));
            ASSERT_EQ(value.size(), 4u);
            EXPECT_EQ(value[0], 1);
            EXPECT_EQ(value[1], 2);
            EXPECT_EQ(value[2], 3);
            EXPECT_EQ(value[3], 4);
            foundData = true;
        }
    }

    EXPECT_TRUE(foundMethod);
    EXPECT_TRUE(foundData);
}
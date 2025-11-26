//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "serialize/mqtt_codec.h"

#include <gtest/gtest.h>
#include <vector>

using namespace reactormq::serialize;

// Variable Byte Integer Tests
TEST(MqttCodec, VariableByteIntegerSize)
{
    EXPECT_EQ(variableByteIntegerSize(0), 1);
    EXPECT_EQ(variableByteIntegerSize(127), 1);
    EXPECT_EQ(variableByteIntegerSize(128), 2);
    EXPECT_EQ(variableByteIntegerSize(16383), 2);
    EXPECT_EQ(variableByteIntegerSize(16384), 3);
    EXPECT_EQ(variableByteIntegerSize(2097151), 3);
    EXPECT_EQ(variableByteIntegerSize(2097152), 4);
    EXPECT_EQ(variableByteIntegerSize(268435455), 4);
    EXPECT_EQ(variableByteIntegerSize(268435456), 0);
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_SingleByte)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(0, writer);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x00);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 0);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_127)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(127, writer);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x7F);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 127);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_128)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(128, writer);
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x01);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 128);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_16383)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(16383, writer);
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xFF);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x7F);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 16383);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_16384)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(16384, writer);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 16384);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_2097151)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(2097151, writer);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xFF);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0xFF);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x7F);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 2097151);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_2097152)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(2097152, writer);
    EXPECT_EQ(buffer.size(), 4);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x01);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 2097152);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeVariableByteInteger_MaxValue)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    encodeVariableByteInteger(268435455, writer);
    EXPECT_EQ(buffer.size(), 4);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0xFF);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0xFF);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0xFF);
    EXPECT_EQ(static_cast<uint8_t>(buffer[3]), 0x7F);

    ByteReader reader(buffer.data(), buffer.size());
    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 268435455);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, DecodeVariableByteInteger_EOF)
{
    const std::vector buffer = { std::byte{ 0x80 } };
    ByteReader reader(buffer.data(), buffer.size());

    const uint32_t decoded = decodeVariableByteInteger(reader);
    EXPECT_EQ(decoded, 0);
}

// String Encoding Tests
TEST(MqttCodec, EncodeDecodeString_Empty)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    bool result = encodeString("", writer);
    EXPECT_TRUE(result);
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x00);

    ByteReader reader(buffer.data(), buffer.size());
    std::string decoded;
    result = decodeString(reader, decoded);
    EXPECT_TRUE(result);
    EXPECT_EQ(decoded, "");
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeString_Simple)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    bool result = encodeString("hello", writer);
    EXPECT_TRUE(result);
    EXPECT_EQ(buffer.size(), 7);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x05);
    EXPECT_EQ(static_cast<char>(buffer[2]), 'h');
    EXPECT_EQ(static_cast<char>(buffer[3]), 'e');
    EXPECT_EQ(static_cast<char>(buffer[4]), 'l');
    EXPECT_EQ(static_cast<char>(buffer[5]), 'l');
    EXPECT_EQ(static_cast<char>(buffer[6]), 'o');

    ByteReader reader(buffer.data(), buffer.size());
    std::string decoded;
    result = decodeString(reader, decoded);
    EXPECT_TRUE(result);
    EXPECT_EQ(decoded, "hello");
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodeString_UTF8)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    const std::string utf8str = "hÃ©llo";
    bool result = encodeString(utf8str, writer);
    EXPECT_TRUE(result);

    ByteReader reader(buffer.data(), buffer.size());
    std::string decoded;
    result = decodeString(reader, decoded);
    EXPECT_TRUE(result);
    EXPECT_EQ(decoded, utf8str);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, DecodeString_InsufficientData)
{
    const std::vector buffer = { std::byte{ 0x00 }, std::byte{ 0x05 }, std::byte{ 'h' }, std::byte{ 'i' } };
    ByteReader reader(buffer.data(), buffer.size());

    std::string decoded;
    const bool result = decodeString(reader, decoded);
    EXPECT_FALSE(result);
}

// Payload Encoding Tests
TEST(MqttCodec, EncodeDecodePayload_Empty)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    const std::vector<uint8_t> payload;
    encodePayload(payload.data(), payload.size(), writer);
    EXPECT_EQ(buffer.size(), 0);

    ByteReader reader(buffer.data(), buffer.size());
    std::vector<uint8_t> decoded;
    const bool result = decodePayload(reader, 0, decoded);
    EXPECT_TRUE(result);
    EXPECT_EQ(decoded.size(), 0);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, EncodeDecodePayload_NonEmpty)
{
    std::vector<std::byte> buffer;
    const ByteWriter writer(buffer);

    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03, 0xFF };
    encodePayload(payload.data(), payload.size(), writer);
    EXPECT_EQ(buffer.size(), 4);

    ByteReader reader(buffer.data(), buffer.size());
    std::vector<uint8_t> decoded;
    const bool result = decodePayload(reader, 4, decoded);
    EXPECT_TRUE(result);
    EXPECT_EQ(decoded, payload);
    EXPECT_TRUE(reader.isEof());
}

TEST(MqttCodec, DecodePayload_InsufficientData)
{
    const std::vector buffer = { std::byte{ 0x01 }, std::byte{ 0x02 } };
    ByteReader reader(buffer.data(), buffer.size());

    std::vector<uint8_t> decoded;
    const bool result = decodePayload(reader, 10, decoded);
    EXPECT_FALSE(result);
}